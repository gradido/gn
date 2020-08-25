#ifndef GRADIDO_INTERFACES_H
#define GRADIDO_INTERFACES_H

#include <iostream>
#include <vector>
#include <unistd.h>
#include "blockchain_gradido_def.h"
#include <string>
#include <sstream>
#include "gradido_messages.h"

// TODO: thread id, process id, timestamp
#define LOG(msg) \
    std::cerr << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl

#define SAFE_PT(expr) if (expr != 0) throw std::runtime_error("couldn't " #expr)

#define BLOCKCHAIN_HASH_SIZE 32

namespace gradido {

class ITask {
public:
    virtual ~ITask() {}
    virtual void run() = 0;
};

struct MultipartTransaction {
    MultipartTransaction() : rec_count(0), rec{0} {}
    GradidoRecord rec[MAX_RECORD_PARTS];
    int rec_count;
};

struct HashedMultipartTransaction : public MultipartTransaction {
    char hashes[MAX_RECORD_PARTS * BLOCKCHAIN_HASH_SIZE];
};

class IBlockchain {
 public:
    virtual ~IBlockchain() {}
    virtual void init() = 0;
    virtual void add_transaction(const MultipartTransaction& tr) = 0;
    virtual void add_transaction(const HashedMultipartTransaction& tr) = 0;

    virtual void continue_with_transactions() = 0;
    virtual void continue_validation() = 0;

    virtual grpr::BlockRecord get_block_record(uint64_t seq_num) = 0;
    virtual void exec_once_validated(ITask* task) = 0;
    virtual void exec_once_paired_transaction_done(
                           ITask* task, 
                           HederaTimestamp hti) = 0;
    virtual uint64_t get_transaction_count() = 0;
    virtual void require_transactions(std::vector<std::string> endpoints) = 0;
};

struct GroupInfo {
    uint64_t group_id;
    HederaTopicID topic_id;
    std::string alias;  // used to name blockchain folder

    std::string get_directory_name() {
        std::stringstream ss;
        ss << alias << "." << group_id << "." << topic_id.shardNum << 
            "." << topic_id.realmNum << "." << topic_id.topicNum << ".bc";
        return ss.str();
    }
};

class IGradidoConfig {
 public:
    virtual ~IGradidoConfig() {}
    virtual void init(const std::vector<std::string>& params) = 0;
    virtual std::vector<GroupInfo> get_group_info_list() = 0;
    virtual int get_worker_count() = 0;
    virtual int get_io_worker_count() = 0;
    virtual std::string get_data_root_folder() = 0;
    virtual std::string get_hedera_mirror_endpoint() = 0;
    virtual int get_blockchain_append_batch_size() = 0;
    virtual int get_blockchain_init_batch_size() = 0;
    virtual int get_block_record_outbound_batch_size() = 0;
    virtual void add_blockchain(GroupInfo gi) = 0;
    virtual bool add_sibling_node(std::string endpoint) = 0;
    virtual bool remove_sibling_node(std::string endpoint) = 0;
    virtual std::vector<std::string> get_sibling_nodes() = 0;
    virtual std::string get_group_requests_endpoint() = 0;
    virtual std::string get_record_requests_endpoint() = 0;
    virtual std::string get_manage_network_requests_endpoint() = 0;
};

// communication layer threads should not be delayed much; use tasks
class ICommunicationLayer {
public:
    virtual ~ICommunicationLayer() {}
    class TransactionListener {
    public:
        virtual void on_transaction(ConsensusTopicResponse& transaction) = 0;
        // is called if reconnect occured; this may imply some blocks 
        // have to be obtained from sibling nodes
        virtual void on_reconnect() = 0;
    };

    class ManageGroupListener {
    public:
        virtual grpr::ManageGroupResponse on_request(grpr::ManageGroupRequest) = 0;
    };

    class BlockRecordSink {
    public:
        // closes after br.success == false
        virtual void on_record(grpr::BlockRecord br) = 0;
    };

    class RecordRequestListener {
    public:
        virtual void on_request(grpr::BlockRangeDescriptor brd,
                                std::shared_ptr<BlockRecordSink> brs) = 0;
    };

    class RecordReceiver {
    public:
        virtual void on_record(grpr::BlockRecord br) = 0;
    };

    class ManageNetworkReceiver {
    public:
        virtual grpr::ManageNodeNetworkResponse on_request(grpr::ManageNodeNetwork mnn) = 0;
    };

 public:
    virtual void init(int worker_count) = 0;
    
    virtual void receive_gradido_transactions(std::string endpoint,
                                              HederaTopicID topic_id,
                                              std::shared_ptr<TransactionListener> tl) = 0;
    virtual void stop_receiving_gradido_transactions(
                         HederaTopicID topic_id) = 0;

    // always single listener
    virtual void receive_manage_group_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageGroupListener> mgl) = 0;

    // always single listener
    virtual void receive_record_requests(
                 std::string endpoint, 
                 std::shared_ptr<RecordRequestListener> rrl) = 0;

    virtual void require_transactions(std::string endpoint,
                                      grpr::BlockRangeDescriptor brd, 
                                      std::shared_ptr<RecordReceiver> rr) = 0;

    // always single listener
    virtual void receive_manage_network_requests(
                 std::string endpoint, 
                 std::shared_ptr<ManageNetworkReceiver> mnr) = 0;
};

class IGradidoFacade {
 public:
    virtual ~IGradidoFacade() {}
    virtual void init(const std::vector<std::string>& params) = 0;
    virtual void join() = 0;
    virtual IBlockchain* get_group_blockchain(uint64_t group_id) = 0;
    virtual IBlockchain* create_group_blockchain(GroupInfo gi) = 0;
    virtual void push_task(ITask* task) = 0;
    virtual IGradidoConfig* get_conf() = 0;
    virtual bool add_group(GroupInfo gi) = 0;
    virtual bool remove_group(uint64_t group_id) = 0;
    virtual ICommunicationLayer* get_communications() = 0;
    virtual void exit(int ret_val) = 0;
    
};

}

#endif
