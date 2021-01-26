#include "test_common.h"

#include "communications.h"
#include "facades_impl.h"

class AlmostEmptyFacade : public EmptyFacade {
private:
    WorkerPool worker_pool;
 public:
    AlmostEmptyFacade() : worker_pool(this) {}
    virtual void init(const std::vector<std::string>& params) {
        worker_pool.init(3);
    }
    virtual void push_task(ITask* task) {
        LOG("push task");

        worker_pool.push(task);
    }
};


class BlockRecordReceiverImpl : public ICommunicationLayer::BlockRecordReceiver {
public:
    virtual void on_block_record(grpr::BlockRecord br) {
        LOG("on_block_record() " << br.success() << "; "<< br.record());
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

class TestHandlerFactory : public EmptyHandlerFactoryImpl {
public:
    TestHandlerFactory() : EmptyHandlerFactoryImpl(0) {}
    virtual ITask* subscribe_to_blocks(grpr::BlockDescriptor* req,
                                       grpr::BlockRecord* reply, 
                                       ICommunicationLayer::HandlerCb* cb) {
        return new BlockHandleTask(req, reply, cb);
    }
};

TEST(GradidoCommunications, smoke) {

    AlmostEmptyFacade ef;
    std::vector<std::string> params;
    ef.init(params);
    std::string endpoint0 = "0.0.0.0:17171";
    int json_rpc_port0 = 17172;
    std::string endpoint1 = "0.0.0.0:17181";
    int json_rpc_port1 = 17182;
    TestHandlerFactory thf;
    

    CommunicationLayer comm0(&ef);
    CommunicationLayer comm1(&ef);


    comm0.init(1, endpoint0, json_rpc_port0, &thf);
    comm1.init(1, endpoint1, json_rpc_port1, &thf);

    sleep(1);

    grpr::BlockDescriptor bd;
    auto gg = bd.mutable_blockchain_id();
    gg->set_pub_key("tttgroup");
    bd.set_block_id(10);

    ICommunicationLayer::BlockRecordReceiver* brr = 
        new BlockRecordReceiverImpl();

    comm1.require_block_data(endpoint0, bd, brr);

    sleep(2);

}
