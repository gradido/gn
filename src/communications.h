#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include "worker_pool.h"
#include <vector>
#include <string>
#include <memory>
#include "blockchain_gradido_def.h"
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

namespace gradido {

class CommunicationLayer : public ICommunicationLayer {
private:

    // TODO: consider removal
    IGradidoFacade* gf;

    WorkerPool worker_pool;
    int round_robin_distribute_counter;

    class PollClient {
    public:
        virtual void got_data() = 0;
        virtual void init(grpc::CompletionQueue& cq) = 0;
    };

    class PollService : public ITask {
    private:
        grpc::CompletionQueue cq;
        std::vector<PollClient*> clients;
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

    class TopicSubscriber : public PollClient {
    private:
        std::string endpoint;
        HederaTopicID topic_id;        
        std::shared_ptr<TransactionListener> tl;
        std::unique_ptr< ::grpc::ClientAsyncReader<ConsensusTopicResponse>> stream;
        std::unique_ptr<ConsensusService::Stub> stub;
        std::shared_ptr<::grpc::Channel> channel;
        grpc::ClientContext ctx;
        ConsensusTopicQuery query;
        ConsensusTopicResponse ctr;

    public:
        TopicSubscriber(std::string endpoint,
                        HederaTopicID topic_id,
                        std::shared_ptr<TransactionListener> tl);

        virtual void got_data();
        virtual void init(grpc::CompletionQueue& cq);
    };

    std::shared_ptr<ManageGroupListener> mgl;

public:
    CommunicationLayer(IGradidoFacade* gf);
    ~CommunicationLayer();

    virtual void init(int worker_count);
    
    virtual void receive_gradido_transactions(std::string endpoint,
                                      HederaTopicID topic_id,
                                      std::shared_ptr<TransactionListener> tl);
    virtual void stop_receiving_gradido_transactions(HederaTopicID topic_id);

    virtual void receive_manage_group_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageGroupListener> mgl);

    virtual void receive_record_requests(
                 std::string endpoint, 
                 std::shared_ptr<RecordRequestListener> rrl);
    virtual void require_records(std::string endpoint,
                                 grpr::BlockRangeDescriptor brd, 
                                 std::shared_ptr<RecordReceiver> rr);
    virtual void receive_manage_network_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageNetworkReceiver> mnr);
};

}

#endif
