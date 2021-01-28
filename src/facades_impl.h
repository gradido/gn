#ifndef FACADES_IMPL_H
#define FACADES_IMPL_H

#include "gradido_interfaces.h"
#include <stack>
#include <pthread.h>
#include <map>
#include <Poco/Random.h>
#include "config.h"
#include "worker_pool.h"
#include "communications.h"
#include "gradido_events.h"
#include "handshake_handler.h"
#include "versioned.h"


namespace gradido {

class EmptyFacade : public IGradidoFacade {
public:
    virtual void init(const std::vector<std::string>& params) {NOT_SUPPORTED;}
    virtual void init(const std::vector<std::string>& params,
                      IConfigFactory* config_factory) {NOT_SUPPORTED;}
    virtual void init(const std::vector<std::string>& params,
                      ICommunicationLayer::HandlerFactory* hf,
                      IConfigFactory* config_factory) {NOT_SUPPORTED;}
    virtual void join() {NOT_SUPPORTED;}
    virtual void push_task(ITask* task) {NOT_SUPPORTED;}
    virtual void push_task(ITask* task, uint32_t after_seconds) {NOT_SUPPORTED;}
    virtual void push_task_and_join(ITask* task) {NOT_SUPPORTED;}
    virtual void set_task_logging(bool enabled) {NOT_SUPPORTED;}
    virtual IGradidoConfig* get_conf() {NOT_SUPPORTED;}
    virtual ICommunicationLayer* get_communications() {NOT_SUPPORTED;}
    virtual void exit(int ret_val) {NOT_SUPPORTED;}
    virtual void exit(std::string msg) {NOT_SUPPORTED;}
    virtual void reload_config() {NOT_SUPPORTED;}
    virtual std::string get_env_var(std::string name) {NOT_SUPPORTED;}
    virtual void continue_init_after_sb_done() {NOT_SUPPORTED;}
    virtual void continue_init_after_handshake_done() {NOT_SUPPORTED;}
    virtual std::string get_sb_ordering_node_endpoint() {NOT_SUPPORTED;}
    virtual std::string get_node_type_str() {NOT_SUPPORTED;}
    virtual bool ordering_broadcast(IVersioned* ve, grpr::OrderingRequest* ore) {NOT_SUPPORTED;}
    virtual ISubclusterBlockchain* get_subcluster_blockchain() {NOT_SUPPORTED;}
    virtual void global_log(std::string message) {NOT_SUPPORTED;}
    virtual IHandshakeHandler* get_handshake_handler(bool force) {NOT_SUPPORTED;}
    virtual IVersioned* get_versioned(int version_number) {NOT_SUPPORTED;}
    virtual IVersioned* get_current_versioned() {NOT_SUPPORTED;}
    virtual void continue_init_after_group_register_done() {NOT_SUPPORTED;}
    virtual IGroupRegisterBlockchain* get_group_register() {NOT_SUPPORTED;}
    virtual IBlockchain* get_group_blockchain(std::string group) {NOT_SUPPORTED;}
    virtual IBlockchain* create_group_blockchain(GroupInfo gi) {NOT_SUPPORTED;}
    virtual IBlockchain* create_or_get_group_blockchain(std::string group) {NOT_SUPPORTED;}
    virtual IAbstractBlockchain* get_any_blockchain(std::string name) {NOT_SUPPORTED;}
    virtual void exclude_blockchain(std::string group) {NOT_SUPPORTED;}
    virtual void remove_blockchain(std::string group) {NOT_SUPPORTED;}
    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req) {NOT_SUPPORTED;}
    virtual void exec_once_paired_transaction_done(
                           std::string group,
                           IGradidoFacade::PairedTransactionListener* ptl,
                           HederaTimestamp hti) {NOT_SUPPORTED;}
    virtual void exec_once_paired_transaction_done(
                           HederaTimestamp hti) {NOT_SUPPORTED;}
    virtual bool get_random_sibling_endpoint(std::string& res) {NOT_SUPPORTED;}
};

class AbstractFacade : public IAbstractFacade {
private:
    IGradidoConfig* config;
    WorkerPool worker_pool;
    CommunicationLayer communication_layer;
    bool task_logging_enabled;
    pthread_mutex_t main_lock;
public:
    AbstractFacade(IGradidoFacade* gf);
    virtual ~AbstractFacade();
    virtual void init(const std::vector<std::string>& params,
                      ICommunicationLayer::HandlerFactory* hf,
                      IConfigFactory* config_factory);
    virtual void join();
    virtual void push_task(ITask* task);
    virtual void push_task(ITask* task, uint32_t after_seconds);
    virtual void push_task_and_join(ITask* task);
    virtual void set_task_logging(bool enabled);
    virtual IGradidoConfig* get_conf();
    virtual ICommunicationLayer* get_communications();
    virtual void exit(int ret_val);
    virtual void exit(std::string msg);
    virtual void reload_config();
    virtual std::string get_env_var(std::string name);
};

// deprecated
class GroupRegisterFacade : public IGroupRegisterFacade {
private:
    IGradidoFacade* gf;
    IGroupRegisterBlockchain* group_register;
public:
    GroupRegisterFacade(IGradidoFacade* gf) : gf(gf), 
        group_register(0) {}
    virtual ~GroupRegisterFacade();
    virtual void init();
    virtual void continue_init_after_group_register_done();
    virtual IGroupRegisterBlockchain* get_group_register();
};

class GroupBlockchainFacade : public IGroupBlockchainFacade,
    ICommunicationLayer::PairedTransactionReceiver {
public:
    virtual void on_block_record(grpr::OutboundTransaction br,
                                 grpr::OutboundTransactionDescriptor req);

private:
    IGradidoFacade* gf;
    pthread_mutex_t main_lock;

    std::map<std::string, IBlockchain*> blockchains;

    struct PairedTransactionData {
        IGradidoFacade::PairedTransactionListener* ptl;
        grpr::OutboundTransactionDescriptor otd;
    };

    std::map<HederaTimestamp, PairedTransactionData> waiting_for_paired;
    Poco::Random rng;
    
public:
    GroupBlockchainFacade(IGradidoFacade* gf) : gf(gf) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    virtual ~GroupBlockchainFacade() {
        pthread_mutex_destroy(&main_lock);
    }

    virtual void init();

    // terminology: group == blockchain 

    // list of groups is mutable within lifetime of facade
    virtual IBlockchain* get_group_blockchain(std::string group);
    virtual IBlockchain* create_group_blockchain(GroupInfo gi);
    virtual IBlockchain* create_or_get_group_blockchain(std::string group);

    // two step deletion: 1) excluding (blockchain still exists, but not
    // accessible from outside); 2) after some time blockchain can be
    // removed (idea is to avoid race condition when an even has a
    // reference to a blockchain about to be deleted)
    //
    // if blockchain is to be removed and then request comes to add it 
    // while it is still removing, blockchain is removed and re-added 
    // afterwards; adding new group anyway is a lengthy process, where
    // user has to wait
    virtual void exclude_blockchain(std::string group);
    // should be called only after exclude_blockchain() + some time
    // has passed
    virtual void remove_blockchain(std::string group);

    class PairedTransactionListener {
    public:
        virtual void on_paired_transaction_done(Transaction t) = 0;
    };
    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req);

    // will check local / remote blockchains and call when data is
    // ready
    virtual void exec_once_paired_transaction_done(
                 std::string group,
                 IGroupBlockchainFacade::PairedTransactionListener* ptl,
                 HederaTimestamp hti);

    // for repeated requests to get paired transaction
    virtual void exec_once_paired_transaction_done(
                           HederaTimestamp hti);


    virtual bool get_random_sibling_endpoint(std::string& res);
};

// almost any executable needs this, extracting as a separate class
// to reduce repetition
class Executable : public EmptyFacade {
private:
    AbstractFacade af;
public:
    Executable() : af(this) {}
    virtual void init(const std::vector<std::string>& params,
                      ICommunicationLayer::HandlerFactory* hf,
                      IConfigFactory* config_factory);
    using EmptyFacade::init;
    virtual void join();
    virtual void push_task(ITask* task);
    virtual void push_task(ITask* task, uint32_t after_seconds);
    virtual void push_task_and_join(ITask* task);
    virtual void set_task_logging(bool enabled);
    virtual IGradidoConfig* get_conf();
    virtual ICommunicationLayer* get_communications();
    virtual void exit(int ret_val);
    virtual void exit(std::string msg);
    virtual void reload_config();
    virtual std::string get_env_var(std::string name);
};

class EmptyHandlerFactoryImpl : 
    public ICommunicationLayer::HandlerFactory {
protected:
    IGradidoFacade* gf;
public:
    EmptyHandlerFactoryImpl(IGradidoFacade* gf) : gf(gf) {}
    virtual ITask* subscribe_to_blocks(
                   grpr::BlockDescriptor* req,
                   grpr::BlockRecord* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* subscribe_to_block_checksums(
                   grpr::BlockchainId* req, 
                   grpr::BlockChecksum* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* manage_group(
                   grpr::ManageGroupRequest* req, 
                   grpr::Ack* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* manage_node_network(
                   grpr::ManageNodeNetwork* req,
                   grpr::Ack* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* get_outbound_transaction(
                   grpr::OutboundTransactionDescriptor* req, 
                   grpr::OutboundTransaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* get_balance(
                   grpr::UserDescriptor* req, 
                   grpr::UserBalance* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* get_transactions(
                   grpr::TransactionRangeDescriptor* req, 
                   grpr::TransactionData* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* get_creation_sum(
                   grpr::CreationSumRangeDescriptor* req, 
                   grpr::CreationSumData* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* get_users(
                   grpr::BlockchainId* req, 
                   grpr::UserData* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* create_node_handshake0(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* create_node_handshake2(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* create_ordering_node_handshake2(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* submit_message(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* subscribe_to_blockchain(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* submit_log_message(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
    virtual ITask* ping(
                   grpr::Transaction* req, 
                   grpr::Transaction* reply, 
                   ICommunicationLayer::HandlerCb* cb) {return 0;}
};

class NodeHandlerFactoryDeprecated : public EmptyHandlerFactoryImpl {
public:
    NodeHandlerFactoryDeprecated(IGradidoFacade* gf) : 
        EmptyHandlerFactoryImpl(gf) {}
    virtual ITask* subscribe_to_blocks(grpr::BlockDescriptor* req,
                                       grpr::BlockRecord* reply, 
                                       ICommunicationLayer::HandlerCb* cb);

    virtual ITask* subscribe_to_block_checksums(
                                                grpr::BlockchainId* req, 
                                                grpr::BlockChecksum* reply, 
                                                ICommunicationLayer::HandlerCb* cb);
    virtual ITask* manage_group(grpr::ManageGroupRequest* req, 
                                grpr::Ack* reply, 
                                ICommunicationLayer::HandlerCb* cb);
    virtual ITask* get_outbound_transaction(
                                            grpr::OutboundTransactionDescriptor* req, 
                                            grpr::OutboundTransaction* reply, 
                                            ICommunicationLayer::HandlerCb* cb);
    virtual ITask* get_users(grpr::BlockchainId* req, 
                             grpr::UserData* reply, 
                             ICommunicationLayer::HandlerCb* cb);
};

class GradidoNodeDeprecated : public Executable {
private:
    GroupRegisterFacade grf;
    GroupBlockchainFacade gbf;
    NodeHandlerFactoryDeprecated handler_factory_impl;
    class ConfigFactory : public IConfigFactory {
    public:
        virtual IGradidoConfig* create() { 
            return new GradidoNodeConfigDeprecated(); 
        }
    };
    ConfigFactory config_factory;

public:
    GradidoNodeDeprecated() : grf(this), gbf(this), 
        handler_factory_impl(this) {}
    virtual void init(const std::vector<std::string>& params);
    virtual void init(const std::vector<std::string>& params,
                      IConfigFactory* config_factory) {NOT_SUPPORTED;}
    virtual void continue_init_after_group_register_done();
    virtual IGroupRegisterBlockchain* get_group_register();
    virtual IBlockchain* get_group_blockchain(std::string group);
    virtual IBlockchain* create_group_blockchain(GroupInfo gi);
    virtual IBlockchain* create_or_get_group_blockchain(std::string group);
    virtual IAbstractBlockchain* get_any_blockchain(std::string name);
    virtual void exclude_blockchain(std::string group);
    virtual void remove_blockchain(std::string group);
    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req);
    virtual void exec_once_paired_transaction_done(
                           std::string group,
                           IGradidoFacade::PairedTransactionListener* ptl,
                           HederaTimestamp hti);
    virtual void exec_once_paired_transaction_done(
                           HederaTimestamp hti);
    virtual bool get_random_sibling_endpoint(std::string& res);

};

// implements just INodeFacade; note, that it needs IGradidoFacade
class NodeFacade : public INodeFacade {
private:
    IGradidoFacade* gf;
    ISubclusterBlockchain* sb;
    HandshakeHandler hh;
    bool handshake_ongoing;
    pthread_mutex_t main_lock;
    std::map<int, IVersioned*> versioneds;
    int current_version_number;

    void start_sb();
public:
    class HandlerFactory : public EmptyHandlerFactoryImpl {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            EmptyHandlerFactoryImpl(gf) {}
        virtual ITask* create_node_handshake0(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 ICommunicationLayer::HandlerCb* cb) {
            return new Handshake0Receiver(gf, req, reply, cb);
        }
    };    
public:
    NodeFacade(IGradidoFacade* gf) : gf(gf), sb(0), hh(gf), 
        handshake_ongoing(false), 
        current_version_number(DEFAULT_VERSION_NUMBER) {
            SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    virtual ~NodeFacade() {
        pthread_mutex_destroy(&main_lock);
        for (auto i : versioneds)
            delete i.second;
    }

    void init();

    virtual void continue_init_after_handshake_done();
    virtual void global_log(std::string message);
    virtual IVersioned* get_versioned(int version_number);
    virtual IVersioned* get_current_versioned();
    virtual IHandshakeHandler* get_handshake_handler(bool force);
    virtual ISubclusterBlockchain* get_subcluster_blockchain();
    virtual void continue_init_after_sb_done();
    virtual std::string get_sb_ordering_node_endpoint();
    virtual std::string get_node_type_str() {NOT_SUPPORTED;}
    virtual bool ordering_broadcast(IVersioned* ve, grpr::OrderingRequest* ore) {NOT_SUPPORTED;}
};

// nodes can inherit this 
class NodeBase : public Executable {
protected:
    NodeFacade nf;
    class HandlerFactory : public NodeFacade::HandlerFactory {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            NodeFacade::HandlerFactory(gf) {}
        virtual ITask* ping(
                       grpr::Transaction* req, 
                       grpr::Transaction* reply, 
                       ICommunicationLayer::HandlerCb* cb) {
            return new PingReceiver(gf, req, reply, cb);
        }
    };

    virtual ICommunicationLayer::HandlerFactory* 
        get_handler_factory() = 0;
    virtual IConfigFactory* get_config_factory() = 0;
public:
    NodeBase() : nf(this) {}
    virtual void init(const std::vector<std::string>& params);
    virtual void init(const std::vector<std::string>& params,
                      IConfigFactory* config_factory) {NOT_SUPPORTED;}

    virtual void continue_init_after_sb_done();
    virtual IVersioned* get_versioned(int version_number);
    virtual IVersioned* get_current_versioned();
    virtual void global_log(std::string message);

    virtual void continue_init_after_handshake_done();
    virtual ISubclusterBlockchain* get_subcluster_blockchain();
    virtual std::string get_sb_ordering_node_endpoint();
    virtual IHandshakeHandler* get_handshake_handler(bool force);
};


class LoggerNode : public NodeBase, public IConfigFactory {
public:
    virtual IGradidoConfig* create();
    
protected:
    class HandlerFactory : public NodeBase::HandlerFactory {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            NodeBase::HandlerFactory(gf) {}
        virtual ITask* submit_log_message(
                       grpr::Transaction* req, 
                       grpr::Transaction* reply, 
                       ICommunicationLayer::HandlerCb* cb) {
            return new LogMessageReceiver(gf, req, reply, cb);
        }
    };
    virtual ICommunicationLayer::HandlerFactory* get_handler_factory();
    virtual IConfigFactory* get_config_factory();
private:
    HandlerFactory handler_factory;
public:
    LoggerNode() : handler_factory(this) {}
    virtual std::string get_node_type_str() { return get_as_str(SbNodeType::LOGGER); }
};

class OrderingNode : public NodeBase, public IConfigFactory {
public:
    virtual IGradidoConfig* create();
    
protected:
    class HandlerFactory : public NodeBase::HandlerFactory {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            NodeBase::HandlerFactory(gf) {}
        virtual ITask* subscribe_to_blockchain(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 ICommunicationLayer::HandlerCb* cb) {
            return new SubscribeToBlockchainReceiver(gf, req, reply, cb);
        }
        virtual ITask* submit_message(
                       grpr::Transaction* req, 
                       grpr::Transaction* reply, 
                       ICommunicationLayer::HandlerCb* cb) {
            return new OrderingMessageReceiver(gf, req, reply, cb);
        }
    };
    virtual ICommunicationLayer::HandlerFactory* get_handler_factory();
    virtual IConfigFactory* get_config_factory();
private:
    HandlerFactory handler_factory;

    class ConnectedClient {
    public:
        void send(grpr::OrderedBlockchainEvent& obe) {}
    };

    class BlockchainContext {
    public:
        uint64_t curr_seq_id;
        std::string current_hash;
        BlockchainContext() : curr_seq_id(0) {}
    };

    std::map<std::string, BlockchainContext*> blockchains_served;
    std::map<std::string, ConnectedClient*> connected_clients;

public:
    OrderingNode() : handler_factory(this) {}
    virtual void continue_init_after_sb_done();

    virtual std::string get_node_type_str() { return get_as_str(SbNodeType::ORDERING); }

    virtual bool ordering_broadcast(IVersioned* ve, 
                                    grpr::OrderingRequest ore);
};

class PingerNode : public NodeBase, public IConfigFactory {
public:
    virtual IGradidoConfig* create();
    
protected:
    class HandlerFactory : public NodeBase::HandlerFactory {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            NodeBase::HandlerFactory(gf) {}
    };
    virtual ICommunicationLayer::HandlerFactory* get_handler_factory();
    virtual IConfigFactory* get_config_factory();
private:
    HandlerFactory handler_factory;
public:
    PingerNode() : handler_factory(this) {}
    
};

class NodeLauncher : public Executable, public IConfigFactory {
public:
    virtual IGradidoConfig* create();
    
protected:
    class HandlerFactory : public EmptyHandlerFactoryImpl {
    public:
        HandlerFactory(IGradidoFacade* gf) : 
            EmptyHandlerFactoryImpl(gf) {}
        virtual ITask* create_node_handshake2(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 ICommunicationLayer::HandlerCb* cb) {
            return new Handshake2Receiver(gf, req, reply, cb);
        }
    };
private:
    HandlerFactory handler_factory;
    StarterHandshakeHandler hh;
    IVersioned* versioned;
public:
    NodeLauncher() : handler_factory(this), hh(this), 
        versioned(new Versioned_1()) {}
    virtual ~NodeLauncher();
    virtual void init(const std::vector<std::string>& params);
    virtual IVersioned* get_versioned(int version_number);
    virtual IHandshakeHandler* get_handshake_handler(bool force);
};

class DemoLoginNode : public EmptyFacade {

};


}

#endif
