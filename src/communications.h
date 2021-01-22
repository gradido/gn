#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <vector>
#include <string>
#include <memory>
#include "grpcpp/grpcpp.h"
#include "grpc/support/log.h"
#include "grpc/grpc.h"
#include "grpc++/channel.h"
#include "grpc++/client_context.h"
#include "grpc++/create_channel.h"
#include "grpc++/security/credentials.h"
#include <grpc/grpc.h>
#include <grpc/support/cpu.h>
#include <grpc/support/log.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/generic/generic_stub.h>
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"

#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "worker_pool.h"
#include "json_rpc/JsonRequestHandlerFactory.h"

#define GCOMM_CALL_DATA_CLASS(Name, ReqType, ReplType, RpcName) \
    class Name :                                                \
    public TemplatedCallData<ReqType,                           \
        ReplType, Name> {                                       \
    public:                                                     \
        using Parent = TemplatedCallData<ReqType,               \
            ReplType, Name>;                                    \
    Name(grpr::GradidoNodeService::AsyncService* service,       \
         grpc::ServerCompletionQueue* cq,                       \
         IGradidoFacade* gf,                                    \
         ICommunicationLayer::HandlerFactory* hf) :             \
        Parent(service, cq, gf, hf) {                           \
            proceed();                                          \
        }                                                       \
                                                                \
        virtual void finish_create() {                          \
            service_->Request##RpcName(&ctx_,                   \
                                       &request_,               \
                                       &responder_,             \
                                       cq_, cq_,                \
                                       this);                   \
        }                                                       \
        virtual ITask* get_handler() {                          \
            return hf->RpcName(&request_,                       \
                               &reply_,                         \
                               this);                           \
        }                                                       \
    };

#define GCOMM_SUBCLUSTER_CALL_DATA_CLASS(Name, RpcName) GCOMM_CALL_DATA_CLASS(Name, grpr::Transaction, grpr::Transaction, RpcName)


namespace gradido {

class EmptyCommunications : public ICommunicationLayer {
 public:
    virtual void init(int worker_count, 
                      std::string rpcs_endpoint,
                      int json_rpc_port,
                      HandlerFactory* hf) {NOT_SUPPORTED;}
    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             TransactionListener* tl) {NOT_SUPPORTED;}
    virtual void receive_grpr_transactions(
                              std::string endpoint,
                              std::string blockchain_name,
                              GrprTransactionListener* tl) {NOT_SUPPORTED;}
    virtual void stop_receiving_gradido_transactions(
                         HederaTopicID topic_id) {NOT_SUPPORTED;}
    virtual void require_block_data(std::string endpoint,
                                    grpr::BlockDescriptor bd, 
                                    BlockRecordReceiver* rr) {NOT_SUPPORTED;}
    virtual void require_block_checksums(std::string endpoint,
                                         grpr::BlockchainId gd, 
                                         BlockChecksumReceiver* rr) {NOT_SUPPORTED;}
    virtual void require_outbound_transaction(
                         std::string endpoint,
                         grpr::OutboundTransactionDescriptor otd,
                         PairedTransactionReceiver* rr) {NOT_SUPPORTED;}
    virtual void send_global_log_message(std::string endpoint,
                                         std::string msg) {NOT_SUPPORTED;}
    virtual void send_handshake0(std::string endpoint,
                                 grpr::Transaction req,
                                 GrprTransactionListener* listener) {NOT_SUPPORTED;}
    virtual void send_handshake2(std::string endpoint,
                                 grpr::Transaction req,
                                 GrprTransactionListener* listener) {NOT_SUPPORTED;}
    virtual bool submit_to_blockchain(std::string endpoint,
                                      grpr::Transaction req,
                                      GrprTransactionListener* listener) {NOT_SUPPORTED;}


};

class CommunicationLayer : public ICommunicationLayer {
private:

    IGradidoFacade* gf;
    pthread_mutex_t main_lock;
    ICommunicationLayer::HandlerFactory* hf;

    WorkerPool worker_pool;
    int round_robin_distribute_counter;

    class PollClient {
    public:
        virtual bool got_data() = 0;
        virtual void init(grpc::CompletionQueue& cq) = 0;
    };

    class PollService : public ITask {
    private:
        grpc::CompletionQueue cq;
        std::vector<PollClient*> clients;
        bool shutdown;
    public:
        PollService();
        ~PollService();

        // takes pointer ownership
        void add_client(PollClient* pc);
        virtual void run();
        void stop();

    };
    std::vector<PollService*> poll_services;

    PollService* get_poll_service();

    class AbstractSubscriber : public PollClient {
    protected:
        std::string endpoint;
        std::shared_ptr<::grpc::Channel> channel;
        grpc::ClientContext ctx;
        bool done;
        bool first_message;
    public:
        AbstractSubscriber(std::string endpoint);
    };

    class TopicSubscriber : public AbstractSubscriber {
    private:
        HederaTopicID topic_id;        
        TransactionListener* tl;
        std::unique_ptr< ::grpc::ClientAsyncReader<ConsensusTopicResponse>> stream;
        std::unique_ptr<ConsensusService::Stub> stub;
        ConsensusTopicQuery query;
        ConsensusTopicResponse ctr;

    public:
        TopicSubscriber(std::string endpoint,
                        HederaTopicID topic_id,
                        TransactionListener* tl);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    class BlockSubscriber : public AbstractSubscriber {
    private:
        BlockRecordReceiver* brr;
        std::unique_ptr< ::grpc::ClientAsyncReader<grpr::BlockRecord>> stream;
        std::unique_ptr<grpr::GradidoNodeService::Stub> stub;
        grpr::BlockDescriptor bd;
        grpr::BlockRecord br;
    public:
        BlockSubscriber(std::string endpoint,
                        grpr::BlockDescriptor bd,
                        BlockRecordReceiver* brr);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    class BlockChecksumSubscriber : public AbstractSubscriber {
    private:
        BlockChecksumReceiver* bcr;
        std::unique_ptr< ::grpc::ClientAsyncReader<grpr::BlockChecksum>> stream;
        std::unique_ptr<grpr::GradidoNodeService::Stub> stub;
        grpr::BlockchainId gd;
        grpr::BlockChecksum bc;
    public:
        BlockChecksumSubscriber(std::string endpoint,
                                grpr::BlockchainId gd,
                                BlockChecksumReceiver* bcr);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    class OutboundSubscriber : public AbstractSubscriber {
    private:
        PairedTransactionReceiver* brr;
        std::unique_ptr< ::grpc::ClientAsyncReader<grpr::OutboundTransaction>> stream;
        std::unique_ptr<grpr::GradidoNodeService::Stub> stub;
        grpr::OutboundTransactionDescriptor otd;
        grpr::OutboundTransaction br;
    public:
        OutboundSubscriber(std::string endpoint,
                           grpr::OutboundTransactionDescriptor otd,
                           PairedTransactionReceiver* brr);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    class TransactionSubscriber : public AbstractSubscriber {
    protected:
        GrprTransactionListener* gtl;
        std::unique_ptr< ::grpc::ClientAsyncReader<grpr::Transaction>> stream;
        std::unique_ptr<grpr::GradidoNodeService::Stub> stub;
        
        grpr::Transaction req;
        grpr::Transaction reply;

        virtual void prepare_stream(grpc::CompletionQueue& cq) = 0;
    public:
        TransactionSubscriber(std::string endpoint,
                              grpr::Transaction req,
                              GrprTransactionListener* gtl);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    class Handshake0Subscriber : public TransactionSubscriber {
    protected:
        virtual void prepare_stream(grpc::CompletionQueue& cq);

    public:
        Handshake0Subscriber(std::string endpoint,
                             grpr::Transaction req,
                             GrprTransactionListener* gtl);
    };

    class Handshake2Subscriber : public TransactionSubscriber {
    protected:
        virtual void prepare_stream(grpc::CompletionQueue& cq);

    public:
        Handshake2Subscriber(std::string endpoint,
                             grpr::Transaction req,
                             GrprTransactionListener* gtl);
    };

    class AbstractCallData : public ICommunicationLayer::HandlerCb {
    protected:
        grpr::GradidoNodeService::AsyncService* service_;
        grpc::ServerCompletionQueue* cq_;
        grpc::ServerContext ctx_;
        bool first_call;
        bool done;
        IGradidoFacade* gf;
        ICommunicationLayer::HandlerFactory* hf;
        HandlerContext* handler_ctx;
        uint64_t writes_done;

        enum CallStatus {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStatus status_; // The current serving state.

        virtual void finish_create() = 0;
        virtual void add_clone_to_cq() = 0;
        virtual bool do_process() = 0;
    public:
        AbstractCallData(
              grpr::GradidoNodeService::AsyncService* service, 
              grpc::ServerCompletionQueue* cq,
              IGradidoFacade* gf,
              ICommunicationLayer::HandlerFactory* hf);
        virtual ~AbstractCallData();
        void proceed();

        virtual HandlerContext* get_ctx();
        virtual void set_ctx(HandlerContext* ctx);
        virtual uint64_t get_writes_done();
    };

    template<typename Req, typename Repl, typename Child>
    class TemplatedCallData : public AbstractCallData {
    protected:
        Req request_;
        Repl reply_;
        grpc::ServerAsyncWriter<Repl> responder_;
        
    protected:
        virtual void add_clone_to_cq() {
            new Child(service_, cq_, gf, hf);
        }
        virtual ITask* get_handler() = 0;
        virtual bool do_process() {
            if (done)
                return false;
            ITask* handler = get_handler();
            if (handler) {
                gf->push_task(handler);
                return true;
            } else
                return false;
        }


    public:
        TemplatedCallData(
                      grpr::GradidoNodeService::AsyncService* service, 
                      grpc::ServerCompletionQueue* cq,
                      IGradidoFacade* gf,
                      ICommunicationLayer::HandlerFactory* hf) :
        AbstractCallData(service, cq, gf, hf), responder_(&ctx_) {}

        void write_ready() {
            writes_done++;
            grpc::WriteOptions options;
            if (!reply_.success()) {
                responder_.WriteAndFinish(reply_, options, 
                                          grpc::Status::OK, this);
                done = true;
            } else {
                responder_.Write(reply_, options, this);
            }
        }
    };

    GCOMM_CALL_DATA_CLASS(BlockCallData, grpr::BlockDescriptor,
                          grpr::BlockRecord, subscribe_to_blocks);
    GCOMM_CALL_DATA_CLASS(ChecksumCallData, grpr::BlockchainId,
                          grpr::BlockChecksum, 
                          subscribe_to_block_checksums);
    GCOMM_CALL_DATA_CLASS(ManageGroupCallData, grpr::ManageGroupRequest,
                          grpr::Ack, manage_group);
    GCOMM_CALL_DATA_CLASS(ManageNodeCallData, grpr::ManageNodeNetwork,
                          grpr::Ack, manage_node_network);
    GCOMM_CALL_DATA_CLASS(OutboundTransactionCallData, 
                          grpr::OutboundTransactionDescriptor,
                          grpr::OutboundTransaction, 
                          get_outbound_transaction);
    GCOMM_CALL_DATA_CLASS(GetBalanceCallData, grpr::UserDescriptor,
                          grpr::UserBalance, get_balance);
    GCOMM_CALL_DATA_CLASS(GetTransactionsCallData, 
                          grpr::TransactionRangeDescriptor,
                          grpr::TransactionData, get_transactions);
    GCOMM_CALL_DATA_CLASS(GetCreationSumCallData, 
                          grpr::CreationSumRangeDescriptor,
                          grpr::CreationSumData, get_creation_sum);
    GCOMM_CALL_DATA_CLASS(GetUserCallData, grpr::BlockchainId,
                          grpr::UserData, get_users);

    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(CreateNodeHandshake0CallData, 
                                     create_node_handshake0);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(CreateNodeHandshake2CallData, 
                                     create_node_handshake2);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(CreateOrderingNodeHandshake2CallData, 
                                     create_ordering_node_handshake2);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(SubmitMessageCallData, 
                                     submit_message);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(SubscribeToBlockchainCallData, 
                                     subscribe_to_blockchain);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(SubmitLogMessageCallData, 
                                     submit_log_message);
    GCOMM_SUBCLUSTER_CALL_DATA_CLASS(PingCallData, 
                                     ping);

    class RpcsService : public ITask {
    private:
        std::string endpoint;
        bool shutdown;
        std::unique_ptr<grpc::ServerCompletionQueue> cq_;
        grpr::GradidoNodeService::AsyncService service_;
        std::unique_ptr<grpc::Server> server_;

    public:
        RpcsService(std::string endpoint);
        virtual void run();
        void stop();
        grpr::GradidoNodeService::AsyncService* get_service();
        grpc::ServerCompletionQueue* get_cq();

        // void set_block_request_handler_factory(Factory f);
    };
    // owned by worker pool; TODO: consider unlikely race condition
    // because of destructor called in a different thread
    RpcsService* rpcs_service;

    template<typename T>
    void start_rpc() {
        new T(rpcs_service->get_service(),
              rpcs_service->get_cq(),
              gf, hf);
    }

    Poco::Net::ServerSocket* jsonrpc_svs;
    Poco::Net::HTTPServer* jsonrpc_srv;

public:
    CommunicationLayer(IGradidoFacade* gf);
    ~CommunicationLayer();

    virtual void init(int worker_count, 
                      std::string rpcs_endpoint,
                      int json_rpc_port,
                      ICommunicationLayer::HandlerFactory* hf);

    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             TransactionListener* tl);
    virtual void receive_grpr_transactions(
                              std::string endpoint,
                              std::string blockchain_name,
                              GrprTransactionListener* tl);

    virtual void stop_receiving_gradido_transactions(HederaTopicID topic_id);

    virtual void require_block_data(std::string endpoint,
                                    grpr::BlockDescriptor brd, 
                                    BlockRecordReceiver* rr);
    virtual void require_block_checksums(std::string endpoint,
                                         grpr::BlockchainId brd, 
                                         BlockChecksumReceiver* rr);
    virtual void require_outbound_transaction(
                         std::string endpoint,
                         grpr::OutboundTransactionDescriptor otd,
                         PairedTransactionReceiver* rr);
    virtual void send_global_log_message(std::string endpoint,
                                         std::string msg);
    virtual void send_handshake0(std::string endpoint,
                                 grpr::Transaction req,
                                 ICommunicationLayer::GrprTransactionListener* listener);
    virtual void send_handshake2(std::string endpoint,
                                 grpr::Transaction req,
                                 ICommunicationLayer::GrprTransactionListener* listener);

    virtual bool submit_to_blockchain(std::string endpoint,
                                      grpr::Transaction req,
                                      ICommunicationLayer::GrprTransactionListener* listener);

};

}

#endif
