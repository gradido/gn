#include "facades_impl.h"
#include <Poco/Path.h>
#include <math.h>
#include <set>
#include "utils.h"
#include "blockchain_gradido.h"
#include "group_register.h"
#include "timer_pool.h"
#include "subcluster_blockchain.h"

namespace gradido {

    class TimerListenerImpl : public timer_pool::TimerListener {
    private:
        IAbstractFacade* gf;
        ITask* task;
        timer_pool::Timer* timer;
    public:
        TimerListenerImpl(IAbstractFacade* gf, ITask* task,
                          uint32_t after_seconds) : 
            gf(gf), task(task), 
            timer(timer_pool::TimerPool::getInstance()->createTimer()) {
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

    AbstractFacade::~AbstractFacade() {
        if (config) {
            delete config;
            config = 0;
        }
    }

    void AbstractFacade::init(const std::vector<std::string>& params,
                              ICommunicationLayer::HandlerFactory* hf,
                              IConfigFactory* config_factory) {
        config = config_factory->create();
        config->init(params);
        worker_pool.init(config->get_worker_count());
        communication_layer.init(config->get_io_worker_count(), 
                                 config->get_grpc_endpoint(),
                                 config->get_json_rpc_port(),
                                 hf);
    }

    void AbstractFacade::join() {
        worker_pool.join();
    }

    void AbstractFacade::push_task(ITask* task) {
        worker_pool.push(task);
    }

    void AbstractFacade::push_task(ITask* task, uint32_t after_seconds) {
        new TimerListenerImpl(this, task, after_seconds);
    }

    void AbstractFacade::push_task_and_join(ITask* task) {
        worker_pool.push_and_join(task);
    }

    IGradidoConfig* AbstractFacade::get_conf() {
        return config;
    }

    ICommunicationLayer* AbstractFacade::get_communications() {
        return &communication_layer;
    }

    void AbstractFacade::exit(int ret_val) {
        LOG("exiting with " << ret_val);
        raise(SIGTERM);
    }

    void AbstractFacade::exit(std::string msg) {
        LOG(msg);
        raise(SIGTERM);
    }

    void AbstractFacade::reload_config() {
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

    void NodeFacade::continue_init_after_sb_done() {
        // TODO
    }

    void NodeFacade::start_sb() {
        if (sb)
            return;
        LOG("preparing subcluster blockchain");
        IGradidoConfig* conf = gf->get_conf();
        Poco::Path root_folder(conf->get_data_root_folder());
        if (conf->is_sb_host())
            sb = new SubclusterBlockchain(
                         conf->get_sb_pub_key(),
                         conf->get_grpc_endpoint(),
                         conf->kp_get_pub_key(),
                         root_folder,
                         gf);
        else
            sb = new SubclusterBlockchain(
                         conf->get_sb_pub_key(),
                         conf->get_sb_ordering_node_endpoint(),
                         conf->get_sb_ordering_node_pub_key(),
                         root_folder,
                         gf);
        sb->init();
    }

    void NodeFacade::init() {
        MLock lock(main_lock);
        versioneds.insert({1, new Versioned_1(gf)});
        IGradidoConfig* conf = gf->get_conf();
        if (conf->launch_token_is_present()) {
            if (conf->kp_identity_has())
                throw std::runtime_error("key pair identity already exists; delete both key files manually and redeploy launch token");
            std::string priv;
            std::string pub;
            if (!create_kp_identity(priv, pub))
                throw std::runtime_error("cannot create kp identity");
            conf->kp_store(priv, pub);
            handshake_ongoing = true;
            LOG("waiting for handshake");
        } else {
            if (!conf->kp_identity_has())
                throw std::runtime_error("no identity; provide launch token");
            start_sb();
        }
    }

    std::string NodeFacade::get_sb_ordering_node_endpoint() {
        IGradidoConfig* conf = gf->get_conf();
        if (conf->is_sb_host())
            return conf->get_grpc_endpoint();
        else return conf->get_sb_ordering_node_endpoint();
    }

    void NodeFacade::continue_init_after_handshake_done() {
        MLock lock(main_lock);
        handshake_ongoing = false;
        start_sb();
    }

    void NodeFacade::global_log(std::string message) {
        ICommunicationLayer* c = gf->get_communications();
        std::vector<std::string> ln = sb->get_logger_nodes();
        for (auto i : ln)
            c->send_global_log_message(i, message);
    }

    IVersioned* NodeFacade::get_versioned(int version_number) {
        MLock lock(main_lock);
        auto z = versioneds.find(version_number);
        if (z == versioneds.end())
            return 0;
        return z->second;
    }

    IVersioned* NodeFacade::get_current_versioned() {
        return get_versioned(current_version_number);
    }

    IHandshakeHandler* NodeFacade::get_handshake_handler() {
        MLock lock(main_lock);
        return handshake_ongoing ? &hh : 0;
    }

    ISubclusterBlockchain* NodeFacade::get_subcluster_blockchain() {
        return sb;
    }

    void GroupRegisterFacade::init() {
        LOG("starting group register");
        IGradidoConfig* config = gf->get_conf();
        group_register = new GroupRegister(
                         config->get_data_root_folder(),
                         gf,
                         config->get_group_register_topic_id());
        gf->push_task(new InitGroupRegister(gf));
    }

    GroupRegisterFacade::~GroupRegisterFacade() {
        if (group_register) {
            delete group_register;
            group_register = 0;
        }
    }

    void GroupRegisterFacade::continue_init_after_group_register_done() {
        IGradidoConfig* config = gf->get_conf();
        for (auto i : config->get_group_info_list()) {
            ITask* task = new StartBlockchainTask(gf, i);
            gf->push_task(task);
        }
    }

    IGroupRegisterBlockchain* GroupRegisterFacade::get_group_register() {
        return group_register;
    }

    void GroupBlockchainFacade::on_block_record(
                           grpr::OutboundTransaction br,
                           grpr::OutboundTransactionDescriptor req) {
        if (br.success())
            gf->push_task(new AddPairedTask(gf, br, req));
        else 
            gf->push_task(new TryPairedAgainTask(gf, br, req), 1);
    }

    void GroupBlockchainFacade::init() {}

    IBlockchain* GroupBlockchainFacade::get_group_blockchain(std::string group) {
        MLock lock(main_lock);
        auto ii = blockchains.find(group);
        if (ii == blockchains.end()) 
            return 0;
        return ii->second;
    }

    IBlockchain* GroupBlockchainFacade::create_group_blockchain(GroupInfo gi) {
        MLock lock(main_lock);
        std::string alias(gi.alias);
        auto ii = blockchains.find(alias);
        if (ii != blockchains.end()) {
            LOG("group already exists");
            return 0;
        }
        IGradidoConfig* config = gf->get_conf();
        Poco::Path data_root(config->get_data_root_folder());
        IBlockchain* b = new BlockchainGradido(alias, data_root,
                                               gf, gi.topic_id);
        blockchains.insert({alias, b});
        gf->push_task(new InitGroupBlockchain(gf, b));

        LOG("attached to blockchain " + alias);
        return b;
    }

    IBlockchain* GroupBlockchainFacade::create_or_get_group_blockchain(std::string group) {
        IGradidoConfig* config = gf->get_conf();

        MLock lock(main_lock);
        auto ii = blockchains.find(group);
        if (ii != blockchains.end())
            return ii->second;
        // TODO: finish; this has to be acquired from group blockchain
        GroupInfo gi;
        Poco::Path data_root(config->get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());

        
        //IBlockchain* b = new GradidoGroupBlockchain(gi, bp, this);
        //blockchains.insert({group, b});

        IBlockchain* b = 0;
        return b;
    }

    void GroupBlockchainFacade::exclude_blockchain(std::string group) {
    }

    void GroupBlockchainFacade::remove_blockchain(std::string group) {
    }

    void GroupBlockchainFacade::on_paired_transaction(
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

    void GroupBlockchainFacade::exec_once_paired_transaction_done(HederaTimestamp hti) {
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
        ICommunicationLayer* communication_layer = 
            gf->get_communications();
        communication_layer->require_outbound_transaction(se, 
                                                          ptd.otd, 
                                                          this);
    }

    void GroupBlockchainFacade::exec_once_paired_transaction_done(
                        std::string group,
                        IGroupBlockchainFacade::PairedTransactionListener* ptl,
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
        ICommunicationLayer* communication_layer = 
            gf->get_communications();
        communication_layer->require_outbound_transaction(se, otd, this);
    }

    bool GroupBlockchainFacade::get_random_sibling_endpoint(std::string& res) {
        IGradidoConfig* config = gf->get_conf();
        MLock lock(main_lock);
        std::vector<std::string> s = config->get_sibling_nodes();
        if (!s.size())
            return false;
        res = s[rng.next(s.size())];
        return true;
    }


    void Executable::init(const std::vector<std::string>& params,
                            ICommunicationLayer::HandlerFactory* hf,
                            IConfigFactory* config_factory) {
        af.init(params, hf, config_factory);
    }
    void Executable::join() {
        af.join();
    }
    void Executable::push_task(ITask* task) {
        af.push_task(task);
    }
    void Executable::push_task(ITask* task, uint32_t after_seconds) {
        af.push_task(task, after_seconds);
    }
    void Executable::push_task_and_join(ITask* task) {
        af.push_task_and_join(task);
    }
    IGradidoConfig* Executable::get_conf() {
        return af.get_conf();
    }
    ICommunicationLayer* Executable::get_communications() {
        return af.get_communications();
    }
    void Executable::exit(int ret_val) {
        af.exit(ret_val);
    }
    void Executable::exit(std::string msg) {
        af.exit(msg);
    }
    void Executable::reload_config() {
        af.reload_config();
    }

    ITask* NodeHandlerFactoryDeprecated::subscribe_to_blocks(
                               grpr::BlockDescriptor* req,
                               grpr::BlockRecord* reply, 
                               ICommunicationLayer::HandlerCb* cb) { 
        return new BlockProvider(gf, req, reply, cb);
    }

    ITask* NodeHandlerFactoryDeprecated::
    subscribe_to_block_checksums(grpr::BlockchainId* req, 
                                 grpr::BlockChecksum* reply, 
                                 ICommunicationLayer::HandlerCb* cb) { 
        return new ChecksumProvider(gf, req, reply, cb);
    }

    ITask* NodeHandlerFactoryDeprecated::manage_group(grpr::ManageGroupRequest* req, 
                        grpr::Ack* reply, 
                        ICommunicationLayer::HandlerCb* cb) { 
        return new ManageGroupProvider(gf, req, reply, cb); 
    }

    ITask* NodeHandlerFactoryDeprecated::get_outbound_transaction(
                                    grpr::OutboundTransactionDescriptor* req, 
                                    grpr::OutboundTransaction* reply, 
                                    ICommunicationLayer::HandlerCb* cb) {
        return new OutboundProvider(gf, req, reply, cb); 
    }

    ITask* NodeHandlerFactoryDeprecated::get_users(grpr::BlockchainId* req, 
                     grpr::UserData* reply, 
                     ICommunicationLayer::HandlerCb* cb) { 
        return new UsersProviderDeprecated(gf, req, reply, cb); 
    }


    void GradidoNodeDeprecated::init(
         const std::vector<std::string>& params) {
        Executable::init(params, &handler_factory_impl, 
                           &config_factory);
        grf.init();
        gbf.init();
    }

    void GradidoNodeDeprecated::continue_init_after_group_register_done() {
        grf.continue_init_after_group_register_done();
    }
    IGroupRegisterBlockchain* GradidoNodeDeprecated::get_group_register() {
        grf.get_group_register();
    }
    IBlockchain* GradidoNodeDeprecated::get_group_blockchain(
                                        std::string group) {
        return gbf.get_group_blockchain(group);
    }
    IBlockchain* GradidoNodeDeprecated::create_group_blockchain(
                                        GroupInfo gi) {
        return gbf.create_group_blockchain(gi);
    }
    IBlockchain* GradidoNodeDeprecated::create_or_get_group_blockchain(
                              std::string group) {
        return gbf.create_or_get_group_blockchain(group);
    }
    IAbstractBlockchain* GradidoNodeDeprecated::get_any_blockchain(
                                      std::string name) {
        if (name.compare(GROUP_REGISTER_NAME) == 0) {
            return grf.get_group_register();
        } else return gbf.get_group_blockchain(name);
    }
    void GradidoNodeDeprecated::exclude_blockchain(std::string group) {
        gbf.exclude_blockchain(group);
    }
    void GradidoNodeDeprecated::remove_blockchain(std::string group) {
        gbf.remove_blockchain(group);
    }
    void GradidoNodeDeprecated::on_paired_transaction(
                      grpr::OutboundTransaction br,
                      grpr::OutboundTransactionDescriptor req) {
        gbf.on_paired_transaction(br, req);
    }
    void GradidoNodeDeprecated::exec_once_paired_transaction_done(
                           std::string group,
                           IGradidoFacade::PairedTransactionListener* ptl,
                           HederaTimestamp hti) {
        gbf.exec_once_paired_transaction_done(group, ptl, hti);
    }
    void GradidoNodeDeprecated::exec_once_paired_transaction_done(
                           HederaTimestamp hti) {
        gbf.exec_once_paired_transaction_done(hti);
    }
    bool GradidoNodeDeprecated::get_random_sibling_endpoint(
                                std::string& res) {
        gbf.get_random_sibling_endpoint(res);
    }

    void NodeBase::init(const std::vector<std::string>& params) {
        Executable::init(params, get_handler_factory(), 
                           get_config_factory());
        nf.init();
    }
    void NodeBase::continue_init_after_sb_done() {
        nf.continue_init_after_sb_done();
    }
    void NodeBase::continue_init_after_handshake_done() {
        nf.continue_init_after_handshake_done();
    }

    IVersioned* NodeBase::get_versioned(int version_number) {
        return nf.get_versioned(version_number);
    }

    IVersioned* NodeBase::get_current_versioned() {
        return nf.get_current_versioned();
    }

    void NodeBase::global_log(std::string message) {
        // ignoring; although could send to other nodes
    }

    ISubclusterBlockchain* NodeBase::get_subcluster_blockchain() {
        return nf.get_subcluster_blockchain();
    }
    
    std::string NodeBase::get_sb_ordering_node_endpoint() {
        return nf.get_sb_ordering_node_endpoint();
    }

    IHandshakeHandler* NodeBase::get_handshake_handler() {
        return nf.get_handshake_handler();
    }


    ICommunicationLayer::HandlerFactory* 
    LoggerNode::get_handler_factory() {
        return &handler_factory;
    }


    IConfigFactory* LoggerNode::get_config_factory() {
        return this;
    }

    IGradidoConfig* LoggerNode::create() {
        return new LoggerConfig();
    }

    ICommunicationLayer::HandlerFactory* 
    OrderingNode::get_handler_factory() {
        return &handler_factory;
    }

    IConfigFactory* OrderingNode::get_config_factory() {
        return this;
    }

    IGradidoConfig* OrderingNode::create() {
        return new OrderingConfig();
    }

    ICommunicationLayer::HandlerFactory* 
    PingerNode::get_handler_factory() {
        return &handler_factory;
    }

    IConfigFactory* PingerNode::get_config_factory() {
        return this;
    }

    IGradidoConfig* PingerNode::create() {
        return new PingerConfig();
    }

    IGradidoConfig* NodeLauncher::create() {
        return new NodeLauncherConfig();
    }

    void NodeLauncher::init(const std::vector<std::string>& params) {
        Executable::init(params, &handler_factory, this);
        push_task(new SendH0(this, 
                             get_conf()->get_launch_node_endpoint(), 
                             &hh));
    }

    IVersioned* NodeLauncher::get_versioned(int version_number) {
        if (version_number != DEFAULT_VERSION_NUMBER)
            exit("wrong version number " + 
                 std::to_string(version_number));
        return versioned;
    }

    NodeLauncher::~NodeLauncher() {
        if (versioned) {
            delete versioned;
            versioned = 0;
        }
    }

    IHandshakeHandler* NodeLauncher::get_handshake_handler() {
        return &hh;
    }


    
}
