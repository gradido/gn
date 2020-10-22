#include "gradido_facade.h"
#include <Poco/Path.h>
#include <math.h>
#include <set>
#include "utils.h"
#include "blockchain_gradido.h"
#include "group_register.h"

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
                if (r.group().size() >= GROUP_ALIAS_LENGTH - 1) {
                    res.set_success(false);
                } else {
                    GroupInfo gi;
                    HederaTopicID tid;
                    /* TODO: acquire from group list
                    tid.shardNum = r.topic_id().shardnum();
                    tid.realmNum = r.topic_id().realmnum();
                    tid.topicNum = r.topic_id().topicnum();
                    */
                    gi.topic_id = tid;
                    memcpy(gi.alias, r.group().c_str(), 
                           r.group().size());
                    res.set_success(gf->add_group(gi));
                }
            } else if (r.action() == 1) {
                res.set_success(gf->remove_group(r.group()));
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
            IBlockchain* b = gf->get_group_blockchain(brd.group());
            if (b) {
                uint64_t first = brd.first_record();
                uint64_t cnt = fmin(brd.record_count(), 
                                    gf->get_conf()->get_block_record_outbound_batch_size());
                uint64_t last = brd.first_record() + cnt;

                uint64_t i = 0;
                for (i = first; i <= last; i++) {
                    grpr::BlockRecord br;
                    if (i < b->get_transaction_count()) {
                        // TODO: transaction_count in case when offset
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

    class ContinueFacadeInit : public ITask {
    private:
        GradidoFacade* gf;
    public:
        ContinueFacadeInit(GradidoFacade* gf) : gf(gf) {}
        virtual void run() {
            gf->continue_init_after_group_register_done();
        }
    };

    class StartGroupRegisterTask : public ITask {
    private:
        GradidoFacade* gf;
        HederaTopicID topic_id;
    public:
        StartGroupRegisterTask(GradidoFacade* gf, 
                               HederaTopicID topic_id) : 
            gf(gf), topic_id(topic_id) {
        }
        virtual void run() {
            IGroupRegisterBlockchain* gr = gf->get_group_register();
            gr->init();
        }
    };

    GradidoFacade::GradidoFacade() : communication_layer(this), 
                                     group_register(0) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    GradidoFacade::~GradidoFacade() {
        pthread_mutex_destroy(&main_lock);

        // TODO: delete blockchains
        // TODO: delete group_register
    }

    void GradidoFacade::init(const std::vector<std::string>& params) {
        config.init(params);
        worker_pool.init(config.get_worker_count());
        communication_layer.init(config.get_io_worker_count());

        group_register = new GroupRegister(
                         config.get_data_root_folder(),
                         this,
                         config.get_group_register_topic_id());
        group_register->exec_once_validated(new ContinueFacadeInit(this));

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

    IBlockchain* GradidoFacade::get_group_blockchain(std::string group) {
        MLock lock(main_lock);
        auto ii = blockchains.find(group);
        if (ii == blockchains.end()) 
            return 0;
        return ii->second;
    }

    IBlockchain* GradidoFacade::create_group_blockchain(GroupInfo gi) {
        MLock lock(main_lock);
        std::string alias(gi.alias);
        auto ii = blockchains.find(alias);
        if (ii != blockchains.end()) {
            LOG("group already exists");
            exit(1); // should not happen
        }
        Poco::Path data_root(config.get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());
        IBlockchain* b = new GradidoGroupBlockchain(gi, bp, this);
        blockchains.insert({alias, b});
        return b;
    }

    IBlockchain* GradidoFacade::create_or_get_group_blockchain(std::string group) {
        MLock lock(main_lock);
        auto ii = blockchains.find(group);
        if (ii != blockchains.end())
            return ii->second;
        // TODO: finish; this has to be acquired from group blockchain
        GroupInfo gi;
        Poco::Path data_root(config.get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());
        IBlockchain* b = new GradidoGroupBlockchain(gi, bp, this);
        blockchains.insert({group, b});
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
        std::string alias(gi.alias);
        if (get_group_blockchain(alias) != 0) {
            LOG("group already exists");
            return false;
        }
        ITask* task = new StartBlockchainTask(this, gi);
        worker_pool.push(task);
        return true;
    }

    bool GradidoFacade::remove_group(std::string group) {
        // TODO: finish here
        return false;
    }
    
    ICommunicationLayer* GradidoFacade::get_communications() {
        return &communication_layer;
    }

    void GradidoFacade::exit(int ret_val) {
        exit(ret_val);
    }

    void GradidoFacade::reload_config() {
        // following things are possible to alter for dynamic reload:
        // - adding new blockchain folder (removing not supported)
        // - updating sibling endpoint file (adding, removing lines)
        config.reload_sibling_file();
        std::vector<GroupInfo> gi = config.get_group_info_list();
        std::set<GroupInfo> gi_set;
        std::copy(gi_set.begin(), gi_set.end(), std::back_inserter(gi));

        config.reload_group_infos();
        std::vector<GroupInfo> gi2 = config.get_group_info_list();

        for (auto i : gi2) {
            if (gi_set.find(i) != gi_set.end()) {
                ITask* task = new StartBlockchainTask(this, i);
                worker_pool.push(task);
            }
        }
    }

    IGroupRegisterBlockchain* GradidoFacade::get_group_register() {
        return group_register;
    }

    void GradidoFacade::continue_init_after_group_register_done() {
        for (auto i : config.get_group_info_list()) {
            ITask* task = new StartBlockchainTask(this, i);
            worker_pool.push(task);
        }
    }


    
}


