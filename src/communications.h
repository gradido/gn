#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include "worker_pool.h"
#include <vector>
#include <string>
#include <memory>
#include "blockchain_gradido_def.h"
#include "gradido_messages.h"
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

namespace gradido {

class CommunicationLayer final {
public:
    class TransactionListener {
    public:
        virtual void on_transaction(ConsensusTopicResponse& transaction) = 0;
    };
    
private:

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

public:
    CommunicationLayer();
    ~CommunicationLayer();

    void init(int worker_count);
    
    void receive_gradido_transactions(std::string endpoint,
                                      HederaTopicID topic_id,
                                      std::shared_ptr<TransactionListener> tl);
    void stop_receiving_gradido_transactions(HederaTopicID topic_id);
};

}

#endif
