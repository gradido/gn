#ifndef GRADIDO_FACADE_H
#define GRADIDO_FACADE_H

#include "gradido_interfaces.h"
#include <stack>
#include <pthread.h>
#include <map>
#include <Poco/Random.h>
#include "config.h"
#include "worker_pool.h"
#include "communications.h"


namespace gradido {

class GradidoFacade : public IGradidoFacade, 
    ICommunicationLayer::PairedTransactionReceiver {
 public:
    virtual void on_block_record(grpr::OutboundTransaction br,
                                 grpr::OutboundTransactionDescriptor req);

 private:
    pthread_mutex_t main_lock;
    Config config;
    WorkerPool worker_pool;
    CommunicationLayer communication_layer;
    std::map<std::string, IBlockchain*> blockchains;

    IGroupRegisterBlockchain* group_register;

    Poco::Random rng;

    struct PairedTransactionData {
        IGradidoFacade::PairedTransactionListener* ptl;
        grpr::OutboundTransactionDescriptor otd;
    };

    std::map<HederaTimestamp, PairedTransactionData> waiting_for_paired;


    class HandlerFactoryImpl : public ICommunicationLayer::HandlerFactory {
    private:
        IGradidoFacade* gf;
    public:
        HandlerFactoryImpl(IGradidoFacade* gf);
        virtual ITask* subscribe_to_blocks(grpr::BlockDescriptor* req,
                                           grpr::BlockRecord* reply, 
                                           ICommunicationLayer::HandlerCb* cb);

        virtual ITask* subscribe_to_block_checksums(
                       grpr::GroupDescriptor* req, 
                       grpr::BlockChecksum* reply, 
                       ICommunicationLayer::HandlerCb* cb);
        virtual ITask* manage_group(grpr::ManageGroupRequest* req, 
                                    grpr::ManageGroupResponse* reply, 
                                    ICommunicationLayer::HandlerCb* cb);
        virtual ITask* manage_node_network(
                              grpr::ManageNodeNetwork* req,
                              grpr::ManageNodeNetworkResponse* reply, 
                              ICommunicationLayer::HandlerCb* cb);
        virtual ITask* get_outbound_transaction(
                           grpr::OutboundTransactionDescriptor* req, 
                           grpr::OutboundTransaction* reply, 
                           ICommunicationLayer::HandlerCb* cb);

        virtual ITask* get_balance(grpr::UserDescriptor* req, 
                                   grpr::UserBalance* reply, 
                                   ICommunicationLayer::HandlerCb* cb);
        virtual ITask* get_transactions(
                       grpr::TransactionRangeDescriptor* req, 
                       grpr::TransactionData* reply, 
                       ICommunicationLayer::HandlerCb* cb);
        virtual ITask* get_creation_sum(
                       grpr::CreationSumRangeDescriptor* req, 
                       grpr::CreationSumData* reply, 
                       ICommunicationLayer::HandlerCb* cb);
        virtual ITask* get_users(grpr::GroupDescriptor* req, 
                                 grpr::UserData* reply, 
                                 ICommunicationLayer::HandlerCb* cb);
    };
    HandlerFactoryImpl rpc_handler_factory;
    
 public:
    GradidoFacade();
    virtual ~GradidoFacade();
    virtual void init(const std::vector<std::string>& params);
    virtual void join();
    virtual IBlockchain* get_group_blockchain(std::string group);
    virtual IBlockchain* create_group_blockchain(GroupInfo gi);
    virtual IBlockchain* create_or_get_group_blockchain(std::string group);

    virtual IAbstractBlockchain* get_any_blockchain(std::string name);

    virtual void exclude_blockchain(std::string group);
    virtual void remove_blockchain(std::string group);


    virtual IGroupRegisterBlockchain* get_group_register();

    virtual void push_task(ITask* task);
    virtual void push_task(ITask* task, uint32_t after_seconds);
    virtual void push_task_and_join(ITask* task);

    virtual IGradidoConfig* get_conf();
    virtual ICommunicationLayer* get_communications();
    virtual void exit(int ret_val);
    virtual void reload_config();
    virtual void exec_once_paired_transaction_done(
                      std::string group,
                      IGradidoFacade::PairedTransactionListener* ptl,
                      HederaTimestamp hti);
    virtual void exec_once_paired_transaction_done(
                           HederaTimestamp hti);

    virtual void continue_init_after_group_register_done();
    virtual bool get_random_sibling_endpoint(std::string& res);
    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req);

};
 
}

#endif
