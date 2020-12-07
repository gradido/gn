#include "gradido_facade.h"
#include <Poco/Path.h>
#include <math.h>
#include <set>
#include "utils.h"
#include "blockchain_gradido.h"
#include "group_register.h"
#include "gradido_events.h"
#include "timer_pool.h"

namespace gradido {


    GradidoFacade::GradidoFacade() : worker_pool(this, "event-queue"),
                                     communication_layer(this), 
                                     group_register(0),
                                     rpc_handler_factory(this) {
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

        communication_layer.init(config.get_io_worker_count(), 
                                 config.get_grpc_endpoint(),
                                 config.get_json_rpc_port(),
                                 &rpc_handler_factory);
        group_register = new GroupRegister(
                         config.get_data_root_folder(),
                         this,
                         config.get_group_register_topic_id());
        push_task(new InitGroupRegister(this));
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
            return 0;
        }
        Poco::Path data_root(config.get_data_root_folder());
        IBlockchain* b = new BlockchainGradido(alias, data_root,
                                               this, gi.topic_id);
        blockchains.insert({alias, b});
        push_task(new InitGroupBlockchain(this, b));

        LOG("attached to blockchain " + alias);
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

        
        //IBlockchain* b = new GradidoGroupBlockchain(gi, bp, this);
        //blockchains.insert({group, b});

        IBlockchain* b = 0;
        return b;
    }

    void GradidoFacade::push_task(ITask* task) {
        worker_pool.push(task);
    }

    void GradidoFacade::push_task_and_join(ITask* task) {
        worker_pool.push_and_join(task);
    }

    IGradidoConfig* GradidoFacade::get_conf() {
        return &config;
    }
    
    ICommunicationLayer* GradidoFacade::get_communications() {
        return &communication_layer;
    }

    void GradidoFacade::exit(int ret_val) {
        // TODO: ret_val
        raise(SIGTERM);
    }

    void GradidoFacade::reload_config() {
        /* TODO: maybe not needed
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
        */
    }

    IGroupRegisterBlockchain* GradidoFacade::get_group_register() {
        return group_register;
    }

    void GradidoFacade::exclude_blockchain(std::string group) {
    }

    void GradidoFacade::remove_blockchain(std::string group) {
    }

    class TimerListenerImpl : public timer_pool::TimerListener {
    private:
        IGradidoFacade* gf;
        ITask* task;
        timer_pool::Timer* timer;
    public:
        TimerListenerImpl(IGradidoFacade* gf, ITask* task,
                          uint32_t after_seconds) : 
            gf(gf), task(task), timer(timer_pool::TimerPool::getInstance()->createTimer()) {
            timer->setListener(this);
            timer->start(after_seconds);
        }
        virtual ~TimerListenerImpl() {
            delete timer;
        }
        virtual void timerEvent(timer_pool::Timer* timer) {
            gf->push_task(task);
            delete this;
        }
    };

    void GradidoFacade::push_task(ITask* task, uint32_t after_seconds) {
        new TimerListenerImpl(this, task, after_seconds);
    }

    void GradidoFacade::continue_init_after_group_register_done() {
        for (auto i : config.get_group_info_list()) {
            ITask* task = new StartBlockchainTask(this, i);
            worker_pool.push(task);
        }
    }

    void GradidoFacade::exec_once_paired_transaction_done(HederaTimestamp hti) {
        std::string se;
        if (!get_random_sibling_endpoint(se))
            throw std::runtime_error("need sibling to get paired transaction");
        PairedTransactionData ptd;
        {
            MLock lock(main_lock);
            auto zz = waiting_for_paired.find(hti);
            if (zz == waiting_for_paired.end())
                throw std::runtime_error("cannot continue getting paired transaction");
            ptd = zz->second;
        }
        communication_layer.require_outbound_transaction(se, 
                                                         ptd.otd, 
                                                         this);
    }


    void GradidoFacade::exec_once_paired_transaction_done(
                        std::string group,
                        IGradidoFacade::PairedTransactionListener* ptl,
                        HederaTimestamp hti) {
        std::string se;
        if (!get_random_sibling_endpoint(se))
            throw std::runtime_error("need sibling to get paired transaction2");
        grpr::OutboundTransactionDescriptor otd;
        {
            MLock lock(main_lock);
            auto zz = waiting_for_paired.find(hti);
            if (zz != waiting_for_paired.end())
                // not more than one blockchain can request it
                return;

            otd.set_group(group);
            ::proto::Timestamp* ts = otd.mutable_paired_transaction_id();
            ts->set_seconds(hti.seconds);
            ts->set_nanos(hti.nanos);

            PairedTransactionData ptd;
            ptd.ptl = ptl;
            ptd.otd = otd;
            waiting_for_paired.insert({hti, ptd});
        }
        communication_layer.require_outbound_transaction(se, otd, this);
    }

    bool GradidoFacade::get_random_sibling_endpoint(std::string& res) {
        MLock lock(main_lock);
        std::vector<std::string> s = config.get_sibling_nodes();
        if (!s.size())
            return false;
        res = s[rng.next(s.size())];
        return true;
    }


    ITask* GradidoFacade::HandlerFactoryImpl::subscribe_to_blocks(
                               grpr::BlockDescriptor* req,
                               grpr::BlockRecord* reply, 
                               ICommunicationLayer::HandlerCb* cb) { 
        return new BlockProvider(gf, req, reply, cb);
    }

    ITask* GradidoFacade::HandlerFactoryImpl::
    subscribe_to_block_checksums(grpr::GroupDescriptor* req, 
                                 grpr::BlockChecksum* reply, 
                                 ICommunicationLayer::HandlerCb* cb) { 
        return new ChecksumProvider(gf, req, reply, cb);
    }

    ITask* GradidoFacade::HandlerFactoryImpl::manage_group(grpr::ManageGroupRequest* req, 
                        grpr::ManageGroupResponse* reply, 
                        ICommunicationLayer::HandlerCb* cb) { 
        return new ManageGroupProvider(gf, req, reply, cb); 
    }

    ITask* GradidoFacade::HandlerFactoryImpl::manage_node_network(
                               grpr::ManageNodeNetwork* req,
                               grpr::ManageNodeNetworkResponse* reply, 
                               ICommunicationLayer::HandlerCb* cb) { return 0; }

    ITask* GradidoFacade::HandlerFactoryImpl::get_outbound_transaction(
                                    grpr::OutboundTransactionDescriptor* req, 
                                    grpr::OutboundTransaction* reply, 
                                    ICommunicationLayer::HandlerCb* cb) {
        return new OutboundProvider(gf, req, reply, cb); 
    }

    ITask* GradidoFacade::HandlerFactoryImpl::get_balance(grpr::UserDescriptor* req, 
                       grpr::UserBalance* reply, 
                       ICommunicationLayer::HandlerCb* cb) { return 0; }
    ITask* GradidoFacade::HandlerFactoryImpl::get_transactions(
                            grpr::TransactionRangeDescriptor* req, 
                            grpr::TransactionData* reply, 
                            ICommunicationLayer::HandlerCb* cb) { return 0; }
    ITask* GradidoFacade::HandlerFactoryImpl::get_creation_sum(
                            grpr::CreationSumRangeDescriptor* req, 
                            grpr::CreationSumData* reply, 
                            ICommunicationLayer::HandlerCb* cb) { return 0; }
    ITask* GradidoFacade::HandlerFactoryImpl::get_users(grpr::GroupDescriptor* req, 
                     grpr::UserData* reply, 
                     ICommunicationLayer::HandlerCb* cb) { 
        return new UsersProvider(gf, req, reply, cb); 
    }

    IAbstractBlockchain* GradidoFacade::get_any_blockchain(
                                        std::string name) {
        
        if (name.compare(GROUP_REGISTER_NAME) == 0) {
            return group_register;
        } else return get_group_blockchain(name);
    }

    GradidoFacade::HandlerFactoryImpl::HandlerFactoryImpl(
                                       IGradidoFacade* gf) :
        gf(gf) {}

    void GradidoFacade::on_block_record(
                           grpr::OutboundTransaction br,
                           grpr::OutboundTransactionDescriptor req) {
        if (br.success())
            push_task(new AddPairedTask(this, br, req));
        else 
            push_task(new TryPairedAgainTask(this, br, req), 1);
    }

    void GradidoFacade::on_paired_transaction(
                        grpr::OutboundTransaction br,
                        grpr::OutboundTransactionDescriptor req) {
        MLock lock(main_lock);

        HederaTimestamp hti = TransactionUtils::translate_Timestamp_from_pb(req.paired_transaction_id());

        auto wfp = waiting_for_paired.find(hti);        
        if (wfp == waiting_for_paired.end())
            throw std::runtime_error("unexpected incoming pair");
        waiting_for_paired.erase(hti);

        // TODO: should not be here
        typedef ValidatedMultipartBlockchain<GradidoRecord, BlockchainGradido, GRADIDO_BLOCK_SIZE> StorageType; 
        StorageType::Record rec;
        memcpy((void*)&rec, (void*)br.data().c_str(), 
               sizeof(StorageType::Record));
        
        Transaction& t = rec.payload.transaction;

        wfp->second.ptl->on_paired_transaction_done(t);
    }
    
    
}


