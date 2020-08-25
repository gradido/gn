#include "gradido_facade.h"
#include "utils.h"
#include "blockchain_gradido.h"
#include <Poco/Path.h>
#include <math.h>

namespace gradido {

    class ManageGroupListenerImpl : public ICommunicationLayer::ManageGroupListener {
    private:
        IGradidoFacade* gf;
    public:
        ManageGroupListenerImpl(GradidoFacade* gf) : gf(gf) {}
        virtual grpr::ManageGroupResponse on_request(grpr::ManageGroupRequest r) {
            // TODO: should be called from an event, and response
            // returned asynchronously
            grpr::ManageGroupResponse res;
            res.set_success(false);
            
            if (r.action() == 0) {
                GroupInfo gi;
                HederaTopicID tid;
                tid.shardNum = r.topic_id().shardnum();
                tid.realmNum = r.topic_id().realmnum();
                tid.topicNum = r.topic_id().topicnum();
                gi.group_id = r.group_id();
                gi.topic_id = tid;
                gi.alias = r.alias();
                res.set_success(gf->add_group(gi));
            } else if (r.action() == 1) {
                res.set_success(gf->remove_group(r.group_id()));
            }
            return res;
        }
    };

    class ProcessBlockRequest : public ITask {
    private:
        IGradidoFacade* gf;
        grpr::BlockRangeDescriptor brd;
        std::shared_ptr<ICommunicationLayer::BlockRecordSink> brs;
    public:
        ProcessBlockRequest(IGradidoFacade* gf,
                            grpr::BlockRangeDescriptor brd,
                            std::shared_ptr<ICommunicationLayer::BlockRecordSink> brs) : 
            gf(gf), brd(brd), brs(brs) {}
        virtual void run() {
            IBlockchain* b = gf->get_group_blockchain(brd.group_id());
            if (b) {
                uint64_t first = brd.first_record();
                uint64_t cnt = fmin(brd.record_count(), 
                                    gf->get_conf()->get_block_record_outbound_batch_size());
                uint64_t last = brd.first_record() + cnt;

                uint64_t i = 0;
                for (i = first; i <= last; i++) {
                    grpr::BlockRecord br;
                    if (i < b->get_transaction_count()) {
                        // TODO: try / catch
                        br = b->get_block_record(i);
                        br.set_success(true);
                        brs->on_record(br);
                    } else {
                        br.set_success(false);
                        brs->on_record(br);
                        break;
                    }
                }
                if (i < brd.record_count()) {
                    brd.set_first_record(first + i);
                    brd.set_record_count(brd.record_count() - i);
                    ITask* task = new ProcessBlockRequest(gf, brd, brs);
                    gf->push_task(task);
                }
            } else {
                grpr::BlockRecord br;
                br.set_success(false);
                brs->on_record(br);
            }
        }
    };

    class RecordRequestListenerImpl : public ICommunicationLayer::RecordRequestListener {
    private:
        IGradidoFacade* gf;
    public:
        RecordRequestListenerImpl(GradidoFacade* gf) : gf(gf) {}
        virtual void on_request(grpr::BlockRangeDescriptor brd,
                                std::shared_ptr<ICommunicationLayer::BlockRecordSink> brs) {
            ITask* task = new ProcessBlockRequest(gf, brd, brs);
            gf->push_task(task);
        }
    };

    class ManageNetworkReceiverImpl : public ICommunicationLayer::ManageNetworkReceiver {
    private:
        IGradidoFacade* gf;
    public:
        ManageNetworkReceiverImpl(IGradidoFacade* gf) : gf(gf) {}
        virtual grpr::ManageNodeNetworkResponse on_request(grpr::ManageNodeNetwork mnn) {
            // TODO: should be called from an event, and response
            // returned asynchronously
            grpr::ManageNodeNetworkResponse res;
            res.set_success(false);
            if (mnn.action() == 0) {
                gf->get_conf()->add_sibling_node(mnn.endpoint());
            } else if (mnn.action() == 1) {
                gf->get_conf()->remove_sibling_node(mnn.endpoint());
            }
            return res;
        }
    };

    class StartBlockchainTask : public ITask {
    private:
        IGradidoFacade* gf;
        GroupInfo gi;
    public:
        StartBlockchainTask(IGradidoFacade* gf, GroupInfo gi) : 
            gf(gf), gi(gi) {
        }
        virtual void run() {
            IBlockchain* b = gf->create_group_blockchain(gi);
            b->init();
        }
    };

    GradidoFacade::GradidoFacade() : communication_layer(this) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    GradidoFacade::~GradidoFacade() {
        pthread_mutex_destroy(&main_lock);
        // TODO: delete blockchains
    }

    void GradidoFacade::init(const std::vector<std::string>& params) {
        config.init(params);
        worker_pool.init(config.get_worker_count());
        communication_layer.init(config.get_io_worker_count());

        for (auto i : config.get_group_info_list()) {
            ITask* task = new StartBlockchainTask(this, i);
            worker_pool.push(task);
        }

        communication_layer.receive_manage_group_requests(
                            config.get_group_requests_endpoint(),
                            std::make_shared<ManageGroupListenerImpl>(this));
        communication_layer.receive_record_requests(
                            config.get_record_requests_endpoint(),
                            std::make_shared<RecordRequestListenerImpl>(this));

        communication_layer.receive_manage_network_requests(
                            config.get_manage_network_requests_endpoint(),
                            std::make_shared<ManageNetworkReceiverImpl>(this));
    }

    void GradidoFacade::join() {
        worker_pool.join();
    }

    IBlockchain* GradidoFacade::get_group_blockchain(uint64_t group_id) {
        MLock lock(main_lock);
        auto ii = blockchains.find(group_id);
        if (ii == blockchains.end()) 
            return 0;
        return ii->second;
    }

    IBlockchain* GradidoFacade::create_group_blockchain(GroupInfo gi) {
        MLock lock(main_lock);
        auto ii = blockchains.find(gi.group_id);
        if (ii != blockchains.end()) {
            LOG("group already exists");
            exit(1);
        }
        Poco::Path data_root(config.get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());
        IBlockchain* b = new GradidoGroupBlockchain(gi, bp, this);
        blockchains.insert({gi.group_id, b});
        return b;
    }

    void GradidoFacade::push_task(ITask* task) {
        worker_pool.push(task);
    }

    IGradidoConfig* GradidoFacade::get_conf() {
        return &config;
    }

    bool GradidoFacade::add_group(GroupInfo gi) {
        MLock lock(main_lock);
        if (get_group_blockchain(gi.group_id) != 0) {
            LOG("group already exists");
            return false;
        }
        ITask* task = new StartBlockchainTask(this, gi);
        worker_pool.push(task);
        return true;
    }

    bool GradidoFacade::remove_group(uint64_t group_id) {
        // TODO: finish here
        return false;
    }
    
    ICommunicationLayer* GradidoFacade::get_communications() {
        return &communication_layer;
    }

    void GradidoFacade::exit(int ret_val) {
        exit(ret_val);
    }

    
}


