#include "test_common.h"

#include "communications.h"

class EmptyFacade : public IGradidoFacade {
private:
    WorkerPool worker_pool;
 public:
    EmptyFacade() : worker_pool(this) {}
    virtual void init(const std::vector<std::string>& params) {
        worker_pool.init(3);
    }
    virtual void join() {}
    // TODO: add const
    virtual IBlockchain* get_group_blockchain(std::string group) {
        return 0;
    }
    virtual IBlockchain* create_group_blockchain(GroupInfo gi) {
        return 0;
    }
    virtual IBlockchain* create_or_get_group_blockchain(std::string group) {
        return 0;
    }

    virtual IGroupRegisterBlockchain* get_group_register() {
        return 0;
    }

    virtual void push_task(ITask* task) {
        std::cerr << "push task" << std::endl;

        worker_pool.push(task);
    }
    virtual IGradidoConfig* get_conf() {
        return 0;
    }
    virtual bool add_group(GroupInfo gi) {}
    virtual bool remove_group(std::string group) {}
    virtual ICommunicationLayer* get_communications() {
        return 0;
    }
    virtual void exit(int ret_val) {}

    virtual void reload_config() {}
    virtual void exclude_blockchain(std::string group) {}
    virtual void remove_blockchain(std::string group) {}
    virtual void push_task(ITask* task, uint32_t after_seconds) {}
    virtual void exec_once_paired_transaction_done(
                           std::string group,
                           ITask* task, 
                           HederaTimestamp hti) {}
    virtual void continue_init_after_group_register_done() {}
    virtual bool get_random_sibling_endpoint(std::string& res) { return false; }

    virtual IAbstractBlockchain* get_any_blockchain(std::string name) {}
    virtual void exec_once_paired_transaction_done(
                      std::string group,
                      IGradidoFacade::PairedTransactionListener* ptl,
                      HederaTimestamp hti) {}
    virtual void exec_once_paired_transaction_done(
                                                   HederaTimestamp hti) {}

    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req) {}

};


class BlockRecordReceiverImpl : public ICommunicationLayer::BlockRecordReceiver {
public:
    virtual void on_block_record(grpr::BlockRecord br) {

        std::cerr << "on_block_record() " << br.success() << "; "<< br.record()<< std::endl;

    }

};

class BlockHandleTask : public ITask {
private:
    grpr::BlockDescriptor* req;
    grpr::BlockRecord* reply;
    ICommunicationLayer::HandlerCb* cb;
public:
    BlockHandleTask(grpr::BlockDescriptor* req,
                    grpr::BlockRecord* reply, 
                    ICommunicationLayer::HandlerCb* cb) : 
        req(req), reply(reply), cb(cb) {}
    void run() {
        if (cb->get_writes_done() >= 1) {
            reply->set_success(false);
        } else {
            reply->set_success(true);
            reply->set_record(std::to_string(17));
        }
        cb->write_ready();
    }
};

class TestHandlerFactory : public ICommunicationLayer::HandlerFactory {
public:
    virtual ITask* subscribe_to_blocks(grpr::BlockDescriptor* req,
                                       grpr::BlockRecord* reply, 
                                       ICommunicationLayer::HandlerCb* cb) {
        return new BlockHandleTask(req, reply, cb);
    }
        virtual ITask* subscribe_to_block_checksums(
                       grpr::GroupDescriptor* req, 
                       grpr::BlockChecksum* reply, 
                       ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* manage_group(grpr::ManageGroupRequest* req, 
                                    grpr::ManageGroupResponse* reply, 
                                    ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* manage_node_network(
                              grpr::ManageNodeNetwork* req,
                              grpr::ManageNodeNetworkResponse* reply, 
                              ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* get_outbound_transaction(
                           grpr::OutboundTransactionDescriptor* req, 
                           grpr::OutboundTransaction* reply, 
                           ICommunicationLayer::HandlerCb* cb) { return 0; }

        virtual ITask* get_balance(grpr::UserDescriptor* req, 
                                   grpr::UserBalance* reply, 
                                   ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* get_transactions(
                       grpr::TransactionRangeDescriptor* req, 
                       grpr::TransactionData* reply, 
                       ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* get_creation_sum(
                       grpr::CreationSumRangeDescriptor* req, 
                       grpr::CreationSumData* reply, 
                       ICommunicationLayer::HandlerCb* cb) { return 0; }
        virtual ITask* get_users(grpr::GroupDescriptor* req, 
                                 grpr::UserData* reply, 
                                 ICommunicationLayer::HandlerCb* cb) { return 0; }

};

TEST(GradidoCommunications, smoke) {

    EmptyFacade ef;
    std::vector<std::string> params;
    ef.init(params);
    std::string endpoint0 = "0.0.0.0:17171";
    std::string endpoint1 = "0.0.0.0:17172";
    TestHandlerFactory thf;
    

    CommunicationLayer comm0(&ef);
    CommunicationLayer comm1(&ef);


    comm0.init(1, endpoint0, &thf);
    comm1.init(1, endpoint1, &thf);

    sleep(1);

    grpr::BlockDescriptor bd;
    auto gg = bd.mutable_group();
    gg->set_group("tttgroup");
    bd.set_block_id(10);

    ICommunicationLayer::BlockRecordReceiver* brr = 
        new BlockRecordReceiverImpl();

    comm1.require_block_data(endpoint0, bd, brr);

    sleep(2);

}
