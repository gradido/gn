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
#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "worker_pool.h"


namespace gradido {

class CommunicationLayer : public ICommunicationLayer {
private:

    IGradidoFacade* gf;
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
        std::shared_ptr<TransactionListener> tl;
        std::unique_ptr< ::grpc::ClientAsyncReader<ConsensusTopicResponse>> stream;
        std::unique_ptr<ConsensusService::Stub> stub;
        ConsensusTopicQuery query;
        ConsensusTopicResponse ctr;

    public:
        TopicSubscriber(std::string endpoint,
                        HederaTopicID topic_id,
                        std::shared_ptr<TransactionListener> tl);

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
        grpr::GroupDescriptor gd;
        grpr::BlockChecksum bc;
    public:
        BlockChecksumSubscriber(std::string endpoint,
                                grpr::GroupDescriptor gd,
                                BlockChecksumReceiver* bcr);

        virtual bool got_data();
        virtual void init(grpc::CompletionQueue& cq);
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
            gf->push_task(handler);
            return true;
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

    class BlockCallData : public TemplatedCallData<grpr::BlockDescriptor,
        grpr::BlockRecord, BlockCallData> {
    public:
        using Parent = TemplatedCallData<grpr::BlockDescriptor,
            grpr::BlockRecord, BlockCallData>;
        BlockCallData(grpr::GradidoNodeService::AsyncService* service, 
                      grpc::ServerCompletionQueue* cq,
                      IGradidoFacade* gf,
                      ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestsubscribe_to_blocks(&ctx_, 
                                                 &request_, 
                                                 &responder_, 
                                                 cq_, cq_,
                                                 this);
        }
        virtual ITask* get_handler() {
            return hf->subscribe_to_blocks(&request_, 
                                           &reply_,
                                           this);
        }
    };

    class ChecksumCallData : 
        public TemplatedCallData<grpr::GroupDescriptor,
        grpr::BlockChecksum, ChecksumCallData> {
    public:
        using Parent = TemplatedCallData<grpr::GroupDescriptor,
            grpr::BlockChecksum, ChecksumCallData>;
        ChecksumCallData(grpr::GradidoNodeService::AsyncService* service, 
                         grpc::ServerCompletionQueue* cq,
                         IGradidoFacade* gf,
                         ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestsubscribe_to_block_checksums(&ctx_, 
                                                          &request_, 
                                                          &responder_, 
                                                          cq_, cq_,
                                                          this);
        }
        virtual ITask* get_handler() {
            return hf->subscribe_to_block_checksums(&request_, 
                                                    &reply_,
                                                    this);
        }
    };

    class ManageGroupCallData : 
        public TemplatedCallData<grpr::ManageGroupRequest,
        grpr::ManageGroupResponse, ManageGroupCallData> {
    public:
        using Parent = TemplatedCallData<grpr::ManageGroupRequest,
            grpr::ManageGroupResponse, ManageGroupCallData>;
        ManageGroupCallData(grpr::GradidoNodeService::AsyncService* service, 
                            grpc::ServerCompletionQueue* cq,
                            IGradidoFacade* gf,
                            ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestmanage_group(&ctx_, 
                                          &request_, 
                                          &responder_, 
                                          cq_, cq_,
                                          this);
        }
        virtual ITask* get_handler() {
            return hf->manage_group(&request_, 
                                    &reply_,
                                    this);
        }
    };

    class ManageNodeCallData : 
        public TemplatedCallData<grpr::ManageNodeNetwork,
        grpr::ManageNodeNetworkResponse, ManageNodeCallData> {
    public:
        using Parent = TemplatedCallData<grpr::ManageNodeNetwork,
            grpr::ManageNodeNetworkResponse, ManageNodeCallData>;
        ManageNodeCallData(grpr::GradidoNodeService::AsyncService* service, 
                           grpc::ServerCompletionQueue* cq,
                           IGradidoFacade* gf,
                           ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestmanage_node_network(&ctx_, 
                                                 &request_, 
                                                 &responder_, 
                                                 cq_, cq_,
                                                 this);
        }
        virtual ITask* get_handler() {
            return hf->manage_node_network(&request_, 
                                           &reply_,
                                           this);
        }
    };

    class OutboundTransactionCallData : 
        public TemplatedCallData<grpr::OutboundTransactionDescriptor,
        grpr::OutboundTransaction, OutboundTransactionCallData> {
    public:
        using Parent = TemplatedCallData<grpr::OutboundTransactionDescriptor,
            grpr::OutboundTransaction, OutboundTransactionCallData>;
        OutboundTransactionCallData(grpr::GradidoNodeService::AsyncService* service, 
                                    grpc::ServerCompletionQueue* cq,
                                    IGradidoFacade* gf,
                                    ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestget_outbound_transaction(&ctx_, 
                                                      &request_, 
                                                      &responder_, 
                                                      cq_, cq_,
                                                      this);
        }
        virtual ITask* get_handler() {
            return hf->get_outbound_transaction(&request_, 
                                                &reply_,
                                                this);
        }
    };

    class GetBalanceCallData : 
        public TemplatedCallData<grpr::UserDescriptor,
        grpr::UserBalance, GetBalanceCallData> {
    public:
        using Parent = TemplatedCallData<grpr::UserDescriptor,
            grpr::UserBalance, GetBalanceCallData>;
        GetBalanceCallData(grpr::GradidoNodeService::AsyncService* service, 
                                    grpc::ServerCompletionQueue* cq,
                                    IGradidoFacade* gf,
                                    ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestget_balance(&ctx_, 
                                         &request_, 
                                         &responder_, 
                                         cq_, cq_,
                                         this);
        }
        virtual ITask* get_handler() {
            return hf->get_balance(&request_, 
                                   &reply_,
                                   this);
        }
    };


    class GetTransactionsCallData : 
        public TemplatedCallData<grpr::TransactionRangeDescriptor,
        grpr::TransactionData, GetTransactionsCallData> {
    public:
        using Parent = TemplatedCallData<grpr::TransactionRangeDescriptor,
            grpr::TransactionData, GetTransactionsCallData>;
        GetTransactionsCallData(grpr::GradidoNodeService::AsyncService* service, 
                                    grpc::ServerCompletionQueue* cq,
                                    IGradidoFacade* gf,
                                    ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestget_transactions(&ctx_, 
                                              &request_, 
                                              &responder_, 
                                              cq_, cq_,
                                              this);
        }
        virtual ITask* get_handler() {
            return hf->get_transactions(&request_, 
                                        &reply_,
                                        this);
        }
    };

    class GetCreationSumCallData : 
        public TemplatedCallData<grpr::CreationSumRangeDescriptor,
        grpr::CreationSumData, GetCreationSumCallData> {
    public:
        using Parent = TemplatedCallData<grpr::CreationSumRangeDescriptor,
            grpr::CreationSumData, GetCreationSumCallData>;
        GetCreationSumCallData(grpr::GradidoNodeService::AsyncService* service, 
                                    grpc::ServerCompletionQueue* cq,
                                    IGradidoFacade* gf,
                                    ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestget_creation_sum(&ctx_, 
                                              &request_, 
                                              &responder_, 
                                              cq_, cq_,
                                              this);
        }
        virtual ITask* get_handler() {
            return hf->get_creation_sum(&request_, 
                                        &reply_,
                                        this);
        }
    };

    class GetUserCallData : 
        public TemplatedCallData<grpr::GroupDescriptor,
        grpr::UserData, GetUserCallData> {
    public:
        using Parent = TemplatedCallData<grpr::GroupDescriptor,
            grpr::UserData, GetUserCallData>;
        GetUserCallData(grpr::GradidoNodeService::AsyncService* service, 
                        grpc::ServerCompletionQueue* cq,
                        IGradidoFacade* gf,
                        ICommunicationLayer::HandlerFactory* hf) :
        Parent(service, cq, gf, hf) {
            proceed();
        }

        virtual void finish_create() {
            service_->Requestget_users(&ctx_, 
                                       &request_, 
                                       &responder_, 
                                       cq_, cq_,
                                       this);
        }
        virtual ITask* get_handler() {
            return hf->get_users(&request_, 
                                 &reply_,
                                 this);
        }
    };

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

public:
    CommunicationLayer(IGradidoFacade* gf);
    ~CommunicationLayer();

    virtual void init(int worker_count, std::string rpcs_endpoint,
                      ICommunicationLayer::HandlerFactory* hf);

    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             std::shared_ptr<TransactionListener> tl);
    virtual void stop_receiving_gradido_transactions(HederaTopicID topic_id);

    virtual void require_block_data(std::string endpoint,
                                    grpr::BlockDescriptor brd, 
                                    BlockRecordReceiver* rr);
    virtual void require_block_checksums(std::string endpoint,
                                         grpr::GroupDescriptor brd, 
                                         BlockChecksumReceiver* rr);

};

}

#endif
