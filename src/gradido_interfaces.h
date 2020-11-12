#ifndef GRADIDO_INTERFACES_H
#define GRADIDO_INTERFACES_H

#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "blockchain_gradido_def.h"
#include "gradido_messages.h"

/*
  some important invariances:
  - single facade, single comm layer, two pools of workers (io, events),
    many blockchains
  - hedera message is ultimate authority; if it sends different hash than
    expected, blockchain data is considered wrong and has to be obtained
    from (correct data yielding) nodes
  - each hedera message is validated in several steps, namely:
    - line checksum: for locally stored messages
    - structurally: for local, hedera or other node originating messages
      - signature verification, buffer sizes, correct byte character
        check is here
    - calculated-fields-wise: for local, hedera or other node 
      originating messages
      - this also includes checks against current local state, such as if
        user has enough gradidos to perform transaction
    - for hedera checksum correctness in the past
    - for hedera checksum correctness with latest message, originating
      from hedera server directly
  - only those messages inside local blockchain which are validated
    against message received from hedera are considered completely
    valid; single such received message may validate entire blockchain,
    if its hash is correct
    - if not, then either local blockchain (or its tail) or hedera
      is wrong; we choose to believe in hedera
  - methods of system component interface are by no means callable in
    arbitrary sequence; for example, any init() has to be called before
    any other method, and single time
*/

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

// TODO: should expand to include possible long and wrong transactions
struct MultipartTransaction {
    MultipartTransaction() : rec_count(0), rec{0} {}
    GradidoRecord rec[MAX_RECORD_PARTS];
    int rec_count;
    // is set only if there is a structural problem with the message
    std::shared_ptr<std::string> raw_message;
};

struct HashedMultipartTransaction : public MultipartTransaction {
    char hashes[MAX_RECORD_PARTS * BLOCKCHAIN_HASH_SIZE];
};

struct BlockInfo {
    uint32_t size;
    uint8_t checksum[BLOCKCHAIN_CHECKSUM_SIZE];
};

class IAbstractBlockchain {
public:
    virtual ~IAbstractBlockchain() {}
    virtual void init() = 0;

    // blockchain is either validating, or able to add new transactions
    // to it; while validating, incoming transactions are buffered;
    // this method proceeds with transactions inside buffer; should
    // be called only when validation is over
    virtual void continue_with_transactions() = 0;

    // data download from other nodes is considered part of validation;
    // calls should be done serially
    virtual void continue_validation() = 0;

    virtual uint64_t get_transaction_count() = 0;

    // TODO: remove
    virtual void require_blocks(std::vector<std::string> endpoints) = 0;

    virtual uint32_t get_block_count() = 0;
    virtual void get_checksums(std::vector<BlockInfo>& res) = 0;
    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::BlockRecord& res) = 0;
};

struct GroupInfo {
    HederaTopicID topic_id;
    // used as id and to name blockchain folder
    char alias[GROUP_ALIAS_LENGTH];

    std::string get_directory_name() {
        std::stringstream ss;
        ss << alias << "." << topic_id.shardNum << 
            "." << topic_id.realmNum << "." << topic_id.topicNum << ".bc";
        return ss.str();
    }

    bool operator<(const GroupInfo& gi) const {
        return strncmp(alias, gi.alias, GROUP_ALIAS_LENGTH);
    }
};

// group blockchain
class IBlockchain : public IAbstractBlockchain {
public:
    virtual bool get_paired_transaction(HederaTimestamp hti, 
                                        Transaction tt) = 0;
    virtual void add_transaction(const MultipartTransaction& tr) = 0;
    virtual void add_transaction(const HashedMultipartTransaction& tr) = 0;
};

class IGroupRegisterBlockchain : public IAbstractBlockchain {
public:

    virtual void add_transaction(const GroupRecord& rec) = 0;
    virtual void add_multipart_transaction(
                 const GroupRegisterRecord* rec, 
                 size_t rec_count) = 0;



    // just for debugging purposes
    virtual void add_record(std::string alias, HederaTopicID tid) = 0;
    virtual std::vector<GroupInfo> get_groups() = 0;
};


class IGradidoConfig {
 public:
    // essentially, key-value store for user-configurable data
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
    virtual std::string get_grpc_endpoint() = 0;
    virtual void reload_sibling_file() = 0;
    virtual void reload_group_infos() = 0;
    virtual HederaTopicID get_group_register_topic_id() = 0;
    virtual bool is_topic_reset_allowed() = 0;
};

// communication layer threads should not be delayed much; use tasks
class ICommunicationLayer {
public:
    virtual ~ICommunicationLayer() {}
    class TransactionListener {
    public:
        virtual void on_transaction(ConsensusTopicResponse& transaction) = 0;
    };

    class HandlerContext {
    public:
        virtual ~HandlerContext() {}
    };

    class HandlerCb {
    public:
        virtual ~HandlerCb() {}
        // signals that write is ready and related handler can be deleted
        virtual void write_ready() = 0;

        virtual HandlerContext* get_ctx() = 0;
        virtual void set_ctx(HandlerContext* ctx) = 0;
        virtual uint64_t get_writes_done() = 0;
    };

    class HandlerFactory {
    public:
        virtual ~HandlerFactory() {}
        virtual ITask* subscribe_to_blocks(grpr::BlockDescriptor* req,
                                           grpr::BlockRecord* reply, 
                                           HandlerCb* cb) = 0;

        virtual ITask* subscribe_to_block_checksums(
                       grpr::GroupDescriptor* req, 
                       grpr::BlockChecksum* reply, 
                       HandlerCb* cb) = 0;
        virtual ITask* manage_group(grpr::ManageGroupRequest* req, 
                                    grpr::ManageGroupResponse* reply, 
                                    HandlerCb* cb) = 0;
        virtual ITask* manage_node_network(
                              grpr::ManageNodeNetwork* req,
                              grpr::ManageNodeNetworkResponse* reply, 
                              HandlerCb* cb) = 0;
        virtual ITask* get_outbound_transaction(
                           grpr::OutboundTransactionDescriptor* req, 
                           grpr::OutboundTransaction* reply, 
                           HandlerCb* cb) = 0;

        virtual ITask* get_balance(grpr::UserDescriptor* req, 
                                   grpr::UserBalance* reply, 
                                   HandlerCb* cb) = 0;
        virtual ITask* get_transactions(
                       grpr::TransactionRangeDescriptor* req, 
                       grpr::TransactionData* reply, 
                       HandlerCb* cb) = 0;
        virtual ITask* get_creation_sum(
                       grpr::CreationSumRangeDescriptor* req, 
                       grpr::CreationSumData* reply, 
                       HandlerCb* cb) = 0;
        virtual ITask* get_users(grpr::GroupDescriptor* req, 
                                 grpr::UserData* reply, 
                                 HandlerCb* cb) = 0;
    };

    // TODO: consider moving to tasks
    class BlockRecordReceiver {
    public:
        virtual void on_block_record(grpr::BlockRecord br) = 0;
    };

    class BlockChecksumReceiver {
    public:
        virtual void on_block_checksum(grpr::BlockChecksum br) = 0;
    };

 public:
    // hf ownership is not taken by communication layer
    virtual void init(int worker_count, 
                      std::string rpcs_endpoint,
                      HandlerFactory* hf) = 0;
    
    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             std::shared_ptr<TransactionListener> tl) = 0;
    // not possible to have more than one listener for a topic (reason is
    // that single blockchain won't be kept in more than one 
    // representtion locally)
    virtual void stop_receiving_gradido_transactions(
                         HederaTopicID topic_id) = 0;

    virtual void require_block_data(std::string endpoint,
                                    grpr::BlockDescriptor bd, 
                                    BlockRecordReceiver* rr) = 0;
    virtual void require_block_checksums(std::string endpoint,
                                         grpr::GroupDescriptor gd, 
                                         BlockChecksumReceiver* rr) = 0;
};

class IGradidoFacade {
 public:

    // terminology: group == blockchain 

    virtual ~IGradidoFacade() {}

    // init(), join() is assumed to be done by main thread, when
    // starting app
    virtual void init(const std::vector<std::string>& params) = 0;
    virtual void join() = 0;

    // only once, after group register is ready
    virtual void continue_init_after_group_register_done() = 0;

    // list of groups is mutable within lifetime of facade
    virtual IBlockchain* get_group_blockchain(std::string group) = 0;
    virtual IBlockchain* create_group_blockchain(GroupInfo gi) = 0;
    virtual IBlockchain* create_or_get_group_blockchain(std::string group) = 0;

    virtual IAbstractBlockchain* get_any_blockchain(std::string name) = 0;

    
    // two step deletion: 1) excluding (blockchain still exists, but not
    // accessible from outside); 2) after some time blockchain can be
    // removed (idea is to avoid race condition when an even has a
    // reference to a blockchain about to be deleted)
    //
    // if blockchain is to be removed and then request comes to add it 
    // while it is still removing, blockchain is removed and re-added 
    // afterwards; adding new group anyway is a lengthy process, where
    // user has to wait
    virtual void exclude_blockchain(std::string group) = 0;

    // should be called only after exclude_blockchain() + some time
    // has passed
    virtual void remove_blockchain(std::string group) = 0;

    virtual IGroupRegisterBlockchain* get_group_register() = 0;

    virtual void push_task(ITask* task) = 0;
    virtual void push_task(ITask* task, uint32_t after_seconds) = 0;

    // available after init(), always
    virtual IGradidoConfig* get_conf() = 0;

    virtual bool add_group(GroupInfo gi) = 0;
    virtual bool remove_group(std::string group) = 0;

    // available after init(), always
    virtual ICommunicationLayer* get_communications() = 0;

    // should be called only after init(), if ever
    virtual void exit(int ret_val) = 0;

    // only blockchain folder structure and siblings.txt is reload;
    // not possible to delete blockchain by deleting its folder while
    // system is running; use exclude_blockchain() instead
    virtual void reload_config() = 0;

    // will check local / remote blockchains and call when data is
    // ready
    virtual void exec_once_paired_transaction_done(
                           std::string group,
                           ITask* task, 
                           HederaTimestamp hti) = 0;

    virtual bool get_random_sibling_endpoint(std::string& res) = 0;
};

}

#endif
