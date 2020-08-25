#include "communications.h"

namespace gradido {

    CommunicationLayer::PollService::PollService() {}
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

        while (true) {
            void* got_tag;
            bool ok = false;
            cq.Next(&got_tag, &ok);
            PollClient* pc = (PollClient*)got_tag;
            pc->got_data();
        }
    }

    void CommunicationLayer::PollService::stop() {
        // TODO
    }

    CommunicationLayer::PollService* CommunicationLayer::get_poll_service() {
        int res = round_robin_distribute_counter;
        round_robin_distribute_counter++;
        round_robin_distribute_counter =
            round_robin_distribute_counter % poll_services.size();
        
        return poll_services[res];
    }

    CommunicationLayer::TopicSubscriber::TopicSubscriber(std::string endpoint,
                                                         HederaTopicID topic_id,
                                                         std::shared_ptr<TransactionListener> tl) :
        endpoint(endpoint), topic_id(topic_id), tl(tl) {}

    void CommunicationLayer::TopicSubscriber::got_data() {
        // could of consider hiding (void*)this from PollClient, but
        // lets keep it simple here
        stream->Read(&ctr, (void*)this);
        tl->on_transaction(ctr);
    }
    void CommunicationLayer::TopicSubscriber::init(grpc::CompletionQueue& cq) {
            
        channel = grpc::CreateChannel(endpoint,
                                      grpc::InsecureChannelCredentials());
        stub = std::unique_ptr<ConsensusService::Stub>(ConsensusService::NewStub(channel));
            
        proto::TopicID* topicId = query.mutable_topicid();
        topicId->set_shardnum(topic_id.shardNum);
        topicId->set_realmnum(topic_id.realmNum);
        topicId->set_topicnum(topic_id.topicNum);

        stream = stub->PrepareAsyncsubscribeTopic(&ctx, query, &cq);
        stream->StartCall((void*)this);
    }

    
    CommunicationLayer::CommunicationLayer(IGradidoFacade* gf) : gf(gf), round_robin_distribute_counter(0) {
    }

    void CommunicationLayer::init(int worker_count) {
        worker_pool.init(worker_count);
        for (int i = 0; i < worker_count; i++) {
            PollService* ps = new PollService();
            poll_services.push_back(ps);
            worker_pool.push(ps);
        }
    }

    CommunicationLayer::~CommunicationLayer() {
        for (auto* i : poll_services) {
            i->stop();
            delete i;
        }
        poll_services.clear();
    }
    
    void CommunicationLayer::receive_gradido_transactions(std::string endpoint,
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

    void CommunicationLayer::receive_manage_group_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageGroupListener> mgl) {
        this->mgl = mgl;
        // TODO
    }

    void CommunicationLayer::receive_record_requests(
                 std::string endpoint, 
                 std::shared_ptr<RecordRequestListener> rrl) {
        // TODO
    }
    
    void CommunicationLayer::require_transactions(std::string endpoint,
                                                  grpr::BlockRangeDescriptor brd, 
                                                  std::shared_ptr<RecordReceiver> rr) {
        // TODO
    }

    void CommunicationLayer::receive_manage_network_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageNetworkReceiver> mnr) {
        
    }
    

}



