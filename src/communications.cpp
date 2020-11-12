#include "communications.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerAsyncWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;


namespace gradido {

    CommunicationLayer::AbstractSubscriber::AbstractSubscriber(std::string endpoint) : endpoint(endpoint), done(false), first_message(true) {
        channel = grpc::CreateChannel(endpoint,
                                      grpc::InsecureChannelCredentials());
    }

    CommunicationLayer::PollService::PollService() : shutdown(false) {}
    CommunicationLayer::PollService::~PollService() {
        for (auto* i : clients) {
            delete i;
        }
        clients.clear();
    }

    // takes pointer ownership
    void CommunicationLayer::PollService::add_client(PollClient* pc) {
        clients.push_back(pc);
        pc->init(cq);
    }

    void CommunicationLayer::PollService::run() {
        while (!shutdown) {
            void* got_tag;
            bool ok = false;
            cq.Next(&got_tag, &ok);
            // TODO: check ok
            if (!shutdown) {
                PollClient* pc = (PollClient*)got_tag;
                if (!pc->got_data()) {
                    clients.erase(std::find(clients.begin(), 
                                            clients.end(), pc));
                    delete pc;
                }
            }
        }
    }

    void CommunicationLayer::PollService::stop() {
        // TODO: probably shut down streams as well
        shutdown = true;
        cq.Shutdown();
    }

    CommunicationLayer::PollService* CommunicationLayer::get_poll_service() {
        // TODO: lock guard
        int res = round_robin_distribute_counter;
        round_robin_distribute_counter++;
        round_robin_distribute_counter =
            round_robin_distribute_counter % poll_services.size();
        
        return poll_services[res];
    }

    CommunicationLayer::TopicSubscriber::TopicSubscriber(
                        std::string endpoint,
                        HederaTopicID topic_id,
                        std::shared_ptr<TransactionListener> tl) :
        AbstractSubscriber(endpoint), topic_id(topic_id), tl(tl) {}

    bool CommunicationLayer::TopicSubscriber::got_data() {
        // could of consider hiding (void*)this from PollClient, but
        // lets keep it simple here
        stream->Read(&ctr, (void*)this);
        tl->on_transaction(ctr);
        return true;
    }
    void CommunicationLayer::TopicSubscriber::init(grpc::CompletionQueue& cq) {
        stub = std::unique_ptr<ConsensusService::Stub>(ConsensusService::NewStub(channel));
            
        proto::TopicID* topicId = query.mutable_topicid();
        topicId->set_shardnum(topic_id.shardNum);
        topicId->set_realmnum(topic_id.realmNum);
        topicId->set_topicnum(topic_id.topicNum);

        stream = stub->PrepareAsyncsubscribeTopic(&ctx, query, &cq);
        stream->StartCall((void*)this);
    }

    
    CommunicationLayer::CommunicationLayer(IGradidoFacade* gf) : 
        gf(gf), worker_pool(gf), round_robin_distribute_counter(0), 
        rpcs_service(0) {
    }

    void CommunicationLayer::init(int worker_count,
                                  std::string rpcs_endpoint,
                                  HandlerFactory* hf) {
        this->hf = hf;

        // +1 for serving rpcs
        worker_pool.init(worker_count + 1);
        for (int i = 0; i < worker_count; i++) {
            PollService* ps = new PollService();
            poll_services.push_back(ps);
            worker_pool.push(ps);
        }

        rpcs_service = new RpcsService(rpcs_endpoint);
        worker_pool.push(rpcs_service);

        start_rpc<BlockCallData>();
        start_rpc<ChecksumCallData>();
        start_rpc<ManageGroupCallData>();
        start_rpc<ManageNodeCallData>();
        start_rpc<OutboundTransactionCallData>();
        start_rpc<GetBalanceCallData>();
        start_rpc<GetTransactionsCallData>();
        start_rpc<GetCreationSumCallData>();
        start_rpc<GetUserCallData>();
    }

    CommunicationLayer::~CommunicationLayer() {
        for (auto* i : poll_services)
            i->stop();
        if (rpcs_service)
            rpcs_service->stop();

        worker_pool.init_shutdown();
        worker_pool.join();
        
        // tasks (poll services) are owned by worker pool and deleted
        // from it

        poll_services.clear();
    }
    
    void CommunicationLayer::receive_hedera_transactions(std::string endpoint,
                                                         HederaTopicID topic_id,
                                                         std::shared_ptr<TransactionListener> tl) {
        // TODO: synchronize for multi-thread access
        PollService* ps = get_poll_service();
        TopicSubscriber* ts = new TopicSubscriber(endpoint, topic_id, tl);
        ps->add_client(ts);
    }
    void CommunicationLayer::stop_receiving_gradido_transactions(HederaTopicID topic_id) {
        // TODO
    }

    void CommunicationLayer::require_block_data(std::string endpoint,
                                                grpr::BlockDescriptor brd, 
                                                BlockRecordReceiver* rr) {
        PollService* ps = get_poll_service();
        BlockSubscriber* bs = new BlockSubscriber(endpoint, brd, rr);
        ps->add_client(bs);
    }
    void CommunicationLayer::require_block_checksums(std::string endpoint,
                                                     grpr::GroupDescriptor brd, 
                                                     BlockChecksumReceiver* rr) {
        PollService* ps = get_poll_service();
        BlockChecksumSubscriber* bs = 
            new BlockChecksumSubscriber(endpoint, brd, rr);
        ps->add_client(bs);
    }

    CommunicationLayer::AbstractCallData::AbstractCallData(
              grpr::GradidoNodeService::AsyncService* service, 
              ServerCompletionQueue* cq,
              IGradidoFacade* gf,
              ICommunicationLayer::HandlerFactory* hf) : 
        service_(service), cq_(cq),
        status_(CREATE), first_call(true), done(false), gf(gf), hf(hf),
        handler_ctx(0), writes_done(0) {}

    CommunicationLayer::AbstractCallData::~AbstractCallData() {
        if (handler_ctx) {
            delete handler_ctx;
            handler_ctx = 0;
        }
    }
    ICommunicationLayer::HandlerContext* 
    CommunicationLayer::AbstractCallData::get_ctx() {
        return handler_ctx;
    }
    void CommunicationLayer::AbstractCallData::set_ctx(
                             HandlerContext* ctx) {
        handler_ctx = ctx;
    }
    uint64_t CommunicationLayer::AbstractCallData::get_writes_done() {
        return writes_done;
    }

    void CommunicationLayer::AbstractCallData::proceed() {
        if (status_ == CREATE) {
            status_ = PROCESS;
            finish_create();
        } else if (status_ == PROCESS) {
            // Now that we go through this stage multiple times, 
            // we don't want to create a new instance every time.
            // Refer to gRPC's original example if you don't understand 
            // why we create a new instance of CallData here.
            if (first_call) {
                first_call = true;
                add_clone_to_cq();
            }
            if (!do_process())
                status_ = FINISH;
        } else {
            GPR_ASSERT(status_ == FINISH);
            delete this;
        }
    }

    void CommunicationLayer::RpcsService::run() {
        void* tag; // uniquely identifies a request.
        bool ok;
        while (true) {
            GPR_ASSERT(cq_->Next(&tag, &ok));
            // assert not good here; multiple methods served
            GPR_ASSERT(ok);
            ((AbstractCallData*)tag)->proceed();
        }
    }

    CommunicationLayer::RpcsService::RpcsService(std::string endpoint) : 
        endpoint(endpoint), shutdown(false) {
        ServerBuilder builder;
        builder.AddListeningPort(endpoint, 
                                 grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);

        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
    }
    
    void CommunicationLayer::RpcsService::stop() {
        shutdown = true;
        // TODO: not possible
        //cq_.Shutdown();
    }

    grpr::GradidoNodeService::AsyncService*
    CommunicationLayer::RpcsService::get_service() {
        return &service_;
    }

    ServerCompletionQueue*
    CommunicationLayer::RpcsService::get_cq() {
        return cq_.get();
    }

    CommunicationLayer::BlockSubscriber::BlockSubscriber(
                        std::string endpoint,
                        grpr::BlockDescriptor bd,
                        BlockRecordReceiver* brr) : 
        AbstractSubscriber(endpoint), brr(brr), bd(bd) {}

    bool CommunicationLayer::BlockSubscriber::got_data() {        
        if (done) {
            return false;
        }
        stream->Read(&br, (void*)this);

        if (first_message) {
            first_message = false;
        } else {
            brr->on_block_record(br);
            if (!br.success())
                done = true;
        }
        return true;
    }

    void CommunicationLayer::BlockSubscriber::init(
                             grpc::CompletionQueue& cq) {
        stub = std::unique_ptr<grpr::GradidoNodeService::Stub>(
                               grpr::GradidoNodeService::NewStub(channel));
            
        stream = stub->PrepareAsyncsubscribe_to_blocks(&ctx, bd, &cq);
        stream->StartCall((void*)this);
    }

    CommunicationLayer::BlockChecksumSubscriber::BlockChecksumSubscriber(
                        std::string endpoint,
                        grpr::GroupDescriptor gd,
                        BlockChecksumReceiver* bcr) : 
        AbstractSubscriber(endpoint), bcr(bcr), gd(gd) {}

    bool CommunicationLayer::BlockChecksumSubscriber::got_data() {                if (done)
            return false;
        stream->Read(&bc, (void*)this);

        if (first_message) {
            first_message = false;
        } else {
            bcr->on_block_checksum(bc);
            if (!bc.success())
                done = true;
        }
        return true;
    }

    void CommunicationLayer::BlockChecksumSubscriber::init(
                             grpc::CompletionQueue& cq) {

        stub = std::unique_ptr<grpr::GradidoNodeService::Stub>(
                               grpr::GradidoNodeService::NewStub(channel));
            
        stream = stub->PrepareAsyncsubscribe_to_block_checksums(&ctx, gd, &cq);
        stream->StartCall((void*)this);
    }

}
