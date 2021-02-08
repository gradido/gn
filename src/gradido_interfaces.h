#ifndef GRADIDO_INTERFACES_H
#define GRADIDO_INTERFACES_H

#include "gradido_core_utils.h"

#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <pthread.h>
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

namespace gradido {

#define BLOCKCHAIN_HASH_SIZE 32

proto::Timestamp get_current_time();

template<typename T>
class BlockchainTypes {
public:
    enum RecordType {
        EMPTY=0,
        PAYLOAD,
        CHECKSUM
    };

    class Record {
    public:
        uint8_t type; // RecordType
        union {
            T payload;
            uint8_t checksum[BLOCKCHAIN_CHECKSUM_SIZE];
        };
        void operator=(const Record& from) {
            memcpy(this, &from, sizeof(Record));
        }

        Record() {
            ZERO_THIS;
        }
        ~Record() {}
    };

};

class ITask {
public:
    virtual ~ITask() {}
    virtual void run() = 0;

    // for debugging purposes; subclasses may decide to override to 
    // provide more details; by default it is using typeid().name()
    virtual std::string get_task_info();
};

struct BlockInfo {
    uint32_t size;
    uint8_t checksum[BLOCKCHAIN_CHECKSUM_SIZE];
};

template<typename T>
struct Batch {
    // not owned by this object
    T* buff;
    uint32_t size;
    bool reset_blockchain; // deprecated; TODO: remove
    Batch() : buff(0), size(0), reset_blockchain(false) {}

};

typedef Batch<GroupRegisterRecord> GroupRegisterRecordBatch;
typedef Batch<GradidoRecord> GradidoRecordBatch;

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

    virtual uint32_t get_block_count() = 0;
    virtual void get_checksums(std::vector<BlockInfo>& res) = 0;
    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::BlockRecord& res) = 0;

    // TODO: get rid of
    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::OutboundTransaction& res) = 0;
};

template<typename T>
class ITypedBlockchain : public IAbstractBlockchain {
 public:
    virtual void add_transaction(Batch<T> rec) = 0;

    using RecordType = typename BlockchainTypes<T>::RecordType;
    using Record = typename BlockchainTypes<T>::Record;
    virtual bool get_transaction2(uint64_t seq_num, Record& t) = 0;
};

// TODO: deprecated
struct GroupInfo {
    HederaTopicID topic_id;
    // used as id and to name blockchain folder
    char alias[GROUP_ALIAS_LENGTH];

    GroupInfo() { ZERO_THIS; }

    std::string get_directory_name() {
        std::stringstream ss;
        ss << alias << "." << topic_id.shardNum << 
            "." << topic_id.realmNum << "." << topic_id.topicNum << ".bc";
        return ss.str();
    }

    static GroupInfo create(std::string alias, 
                            uint32_t shard_num,
                            uint32_t realm_num,
                            uint32_t topic_num) {
        HederaTopicID tid;
        tid.shardNum = shard_num;
        tid.realmNum = realm_num;
        tid.topicNum = topic_num;
        return create(alias, tid);
    }

    static GroupInfo create(std::string alias, 
                            HederaTopicID topic_id) {
        GroupInfo gi;
        gi.topic_id = topic_id;

        memset(gi.alias, 0, GROUP_ALIAS_LENGTH);
        strncpy(gi.alias, alias.c_str(), min((size_t)GROUP_ALIAS_LENGTH, 
                                             alias.size()));
        return gi;
    }

    static std::string get_directory_name(std::string alias, 
                                          HederaTopicID topic_id) {
        GroupInfo gi;
        gi.topic_id = topic_id;
        memset(gi.alias, 0, GROUP_ALIAS_LENGTH);
        strncpy(gi.alias, alias.c_str(), min((size_t)GROUP_ALIAS_LENGTH, 
                                             alias.size()));
        return gi.get_directory_name();
    }

    bool operator<(const GroupInfo& gi) const {
        return strncmp(alias, gi.alias, GROUP_ALIAS_LENGTH);
    }
};

// group blockchain
class IBlockchain : public ITypedBlockchain<GradidoRecord> {
public:
    virtual bool get_paired_transaction(HederaTimestamp hti, 
                                        uint64_t& seq_num) = 0;
    virtual bool get_transaction(uint64_t seq_num, Transaction& t) = 0;
    virtual bool get_transaction(uint64_t seq_num, GradidoRecord& t) = 0;


    virtual std::vector<std::string> get_users() = 0;
    /*
    virtual void add_transaction(const MultipartTransaction& tr) = 0;
    virtual void add_transaction(const HashedMultipartTransaction& tr) = 0;
    */
};

// TODO: deprecated
class IGroupRegisterBlockchain : 
    public ITypedBlockchain<GroupRegisterRecord> {
public:

    virtual void add_transaction(GroupRegisterRecordBatch rec) = 0;
    virtual bool get_topic_id(std::string alias, HederaTopicID& res) = 0;

    // just for debugging purposes
    virtual void add_record(std::string alias, HederaTopicID tid) = 0;
    virtual std::vector<GroupInfo> get_groups() = 0;
};

class ISubclusterBlockchain : 
    public ITypedBlockchain<SbRecord> {
public:
    virtual std::vector<std::string> get_logger_nodes() = 0;

    class INotifier {
    public:
        virtual ITask* create() = 0;
    };
    // doesn't take ownership
    virtual void on_succ_append(INotifier* n) = 0;

    virtual std::vector<std::string> get_all_endpoints() = 0;
};


class IGradidoConfig {
 public:
    // keys are in hex

    // essentially, key-value store for user-configurable data
    virtual ~IGradidoConfig() {}
    virtual void init(const std::vector<std::string>& params) = 0;
    virtual std::vector<GroupInfo> get_group_info_list() = 0;
    virtual int get_worker_count() = 0;
    virtual int get_io_worker_count() = 0;
    virtual std::string get_data_root_folder() = 0;
    virtual std::string get_hedera_mirror_endpoint() = 0;
    virtual int get_blockchain_append_batch_size() = 0;
    virtual void add_blockchain(GroupInfo gi) = 0;
    virtual bool add_sibling_node(std::string endpoint) = 0;
    virtual bool remove_sibling_node(std::string endpoint) = 0;
    virtual std::string get_sibling_node_file() = 0;
    virtual std::vector<std::string> get_sibling_nodes() = 0;
    virtual std::string get_grpc_endpoint() = 0;
    virtual void reload_sibling_file() = 0;
    virtual void reload_group_infos() = 0;
    virtual std::string get_group_register_topic_id_as_str() = 0;
    virtual HederaTopicID get_group_register_topic_id() = 0;
    virtual bool is_topic_reset_allowed() = 0;
    virtual int get_json_rpc_port() = 0;
    virtual int get_general_batch_size() = 0;
    virtual std::string get_sb_ordering_node_endpoint() = 0;
    virtual std::string get_sb_pub_key() = 0;
    virtual std::string get_sb_ordering_node_pub_key() = 0;
    virtual bool is_sb_host() = 0;

    // launch token is fetched into RAM and file is removed immediately
    virtual bool launch_token_is_present() = 0;
    virtual std::string launch_token_get_admin_key() = 0;

    virtual bool kp_identity_has() = 0;
    virtual std::string kp_get_priv_key() = 0;
    virtual std::string kp_get_pub_key() = 0;
    virtual void kp_store(std::string priv_key, std::string pub_key) = 0;

    virtual std::string get_launch_node_endpoint() = 0;
};

class IConfigFactory {
public:
    virtual IGradidoConfig* create() = 0;
};


// communication layer threads should not be delayed much; use tasks
class ICommunicationLayer {
public:

    // handlers / listeners are supposed to create and start an ITask 
    // subclass instance, to not block IO threads

    virtual ~ICommunicationLayer() {}
    class TransactionListener {
    public:
        virtual void on_transaction(ConsensusTopicResponse& transaction) = 0;
    };

    class GrprTransactionListener {
    public:
        virtual void on_transaction(grpr::Transaction& t) = 0;
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
                       grpr::BlockchainId* req, 
                       grpr::BlockChecksum* reply, 
                       HandlerCb* cb) = 0;
        virtual ITask* manage_group(grpr::ManageGroupRequest* req, 
                                    grpr::Ack* reply, 
                                    HandlerCb* cb) = 0;
        virtual ITask* manage_node_network(
                              grpr::ManageNodeNetwork* req,
                              grpr::Ack* reply, 
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
        virtual ITask* get_users(grpr::BlockchainId* req, 
                                 grpr::UserData* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* create_node_handshake0(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* create_node_handshake2(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* create_ordering_node_handshake2(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* submit_message(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* subscribe_to_blockchain(
                                 grpr::Transaction* req, 
                                 grpr::Transaction* reply, 
                                 HandlerCb* cb) = 0;
        virtual ITask* submit_log_message(grpr::Transaction* req, 
                                          grpr::Transaction* reply, 
                                          HandlerCb* cb) = 0;
        virtual ITask* ping(grpr::Transaction* req, 
                            grpr::Transaction* reply, 
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

    class PairedTransactionReceiver {
    public:
        virtual void on_block_record(grpr::OutboundTransaction br,
                                     grpr::OutboundTransactionDescriptor req) = 0;
    };

 public:
    // hf ownership is not taken by communication layer
    virtual void init(int worker_count, 
                      std::string rpcs_endpoint,
                      int json_rpc_port,
                      HandlerFactory* hf) = 0;
    
    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             TransactionListener* tl) = 0;

    virtual void receive_grpr_transactions(
                              std::string endpoint,
                              std::string blockchain_name,
                              GrprTransactionListener* tl) = 0;

    // not possible to have more than one listener for a topic (reason is
    // that single blockchain won't be kept in more than one 
    // representtion locally)
    virtual void stop_receiving_gradido_transactions(
                         HederaTopicID topic_id) = 0;

    virtual void require_block_data(std::string endpoint,
                                    grpr::BlockDescriptor bd, 
                                    BlockRecordReceiver* rr) = 0;
    virtual void require_block_checksums(std::string endpoint,
                                         grpr::BlockchainId gd, 
                                         BlockChecksumReceiver* rr) = 0;
    virtual void require_outbound_transaction(
                         std::string endpoint,
                         grpr::OutboundTransactionDescriptor otd,
                         PairedTransactionReceiver* rr) = 0;

    virtual void send_global_log_message(std::string endpoint,
                                         std::string msg) = 0;

    virtual void send_handshake0(std::string endpoint,
                                 grpr::Transaction req,
                                 GrprTransactionListener* listener) = 0;
    virtual void send_handshake2(std::string endpoint,
                                 grpr::Transaction req,
                                 GrprTransactionListener* listener) = 0;
    virtual bool submit_to_blockchain(std::string endpoint,
                                      grpr::Transaction req,
                                      GrprTransactionListener* listener) = 0;


};

// some things are considered versioned: their interpretation being
// dependent on grpr::Transaction::version_number
//
// typically, it is signature validation and translation of grpr
// messages, but there can be more to it

// checks signatures on transaction; it is supposed to happen with help
// of subcluster blockchain
class IVersionedGrpc {
public:
    // functions validate t; rpc name is reused as it is
    virtual bool create_node_handshake0(grpr::Transaction t) = 0;
    virtual bool create_node_handshake2(grpr::Transaction t) = 0;
    virtual bool create_ordering_node_handshake2(grpr::Transaction t) = 0;
    virtual bool submit_message(grpr::Transaction t) = 0;
    virtual bool subscribe_to_blockchain(grpr::Transaction t) = 0;
    virtual bool submit_log_message(grpr::Transaction t) = 0;
    virtual bool ping(grpr::Transaction t) = 0;
};

class ITransactionFactoryBase {
public:
    // all identifiers are expected in hex
    enum class ExitCode {
        OK=0,
        ID_BAD_LENGTH,
        ID_NOT_A_HEX,
        MEMO_TOO_LONG,
        MEMO_CONTAINS_ZERO_CHAR
    };
};

class ITransactionFactory : public ITransactionFactoryBase {
public:
    virtual ExitCode create_gradido_creation(std::string user,
                                             uint64_t amount,
                                             std::string memo,
                                             uint8_t** out, 
                                             uint32_t* out_length) = 0;

    virtual ExitCode create_local_transfer(std::string user0,
                                           std::string user1,
                                           uint64_t amount,
                                           std::string memo,
                                           uint8_t** out, 
                                           uint32_t* out_length) = 0;
    virtual ExitCode create_inbound_transfer(std::string user0,
                                             std::string user1,
                                             std::string other_group,
                                             uint64_t amount,
                                             std::string memo,
                                             uint8_t** out, 
                                             uint32_t* out_length) = 0;
    virtual ExitCode create_outbound_transfer(std::string user0,
                                              std::string user1,
                                              std::string other_group,
                                              uint64_t amount,
                                              std::string memo,
                                              uint8_t** out, 
                                              uint32_t* out_length) = 0;
    virtual ExitCode add_group_friend(std::string id,
                                      std::string memo,
                                      uint8_t** out, 
                                      uint32_t* out_length) = 0;
    virtual ExitCode remove_group_friend(std::string id,
                                         std::string memo,
                                         uint8_t** out, 
                                         uint32_t* out_length) = 0;
    
    virtual ExitCode add_user(std::string id,
                              std::string memo,
                              uint8_t** out, 
                              uint32_t* out_length) = 0;
    virtual ExitCode move_user_inbound(std::string id,
                                       std::string other_group,
                                       std::string memo,
                                       uint8_t** out, 
                                       uint32_t* out_length) = 0;
    virtual ExitCode move_user_outbound(std::string id,
                                        std::string other_group,
                                        std::string memo,
                                        uint8_t** out, 
                                        uint32_t* out_length) = 0;
    
    
};

class ISbTransactionFactory : public ITransactionFactoryBase {
public:
};

class IVersioned {
public:
    virtual ~IVersioned() {}
    virtual IVersionedGrpc* get_request_sig_validator() = 0;
    virtual IVersionedGrpc* get_response_sig_validator() = 0;

    virtual void sign(grpr::Transaction* t) = 0;

    virtual bool translate_log_message(grpr::Transaction t, std::string& out) = 0;
    virtual bool translate(grpr::Transaction t, grpr::Handshake0& out) = 0;
    virtual bool translate(grpr::Transaction t, grpr::Handshake1& out) = 0;
    virtual bool translate(grpr::Transaction t, grpr::Handshake2& out) = 0;
    virtual bool translate(grpr::Transaction t, grpr::Handshake3& out) = 0;
    virtual bool translate(grpr::Transaction t, 
                           Batch<SbRecord>& out) = 0;

    virtual bool translate(grpr::Transaction t,
                           grpr::OrderingRequest& ore) = 0;
    
    virtual bool translate_sb_transaction(std::string bytes,
                                          grpr::Transaction& out) = 0;


    virtual bool prepare_h0(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string endpoint) = 0;
    virtual bool prepare_h1(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string kp_pub_key) = 0;
    virtual bool prepare_h2(proto::Timestamp ts, 
                            std::string type,
                            grpr::Transaction& out,
                            std::string kp_pub_key) = 0;
    virtual bool prepare_h3(proto::Timestamp ts, 
                            grpr::Transaction sb,
                            grpr::Transaction& out) = 0;
    virtual bool prepare_add_node(proto::Timestamp ts, 
                                  std::string type,
                                  grpr::Transaction& out) = 0;

    virtual ITransactionFactory* get_transaction_factory() = 0;
    virtual ISbTransactionFactory* get_sb_transaction_factory() = 0;

};

class IOrderer {
public:
    virtual ~IOrderer() {}
    
};


// facade is the central part of a component, which is usually a system
// process (in other words, program)
//
// as such, it provides basic services, like initialization from 
// command line parameters, access to configuration (file or other),
// access to task queue (expecting program logic to be controlled via
// events), access to communications
class IAbstractFacade {
public:
    virtual ~IAbstractFacade() {}

    // init(), join() is assumed to be done by main thread, when
    // starting app
    virtual void init(const std::vector<std::string>& params,
                      ICommunicationLayer::HandlerFactory* hf,
                      IConfigFactory* config_factory) = 0;
    virtual void join() = 0;

    // takes ownership
    virtual void push_task(ITask* task) = 0;
    // takes ownership
    virtual void push_task(ITask* task, uint32_t after_seconds) = 0;

    // does not take ownership
    virtual void push_task_and_join(ITask* task) = 0;

    // enables logging of pushed event types 
    virtual void set_task_logging(bool enabled) = 0;

    // available after init(), always
    virtual IGradidoConfig* get_conf() = 0;

    // available after init(), always
    virtual ICommunicationLayer* get_communications() = 0;

    // should be called only after init(), if ever
    virtual void exit(int ret_val) = 0;
    virtual void exit(std::string msg) = 0;

    // only blockchain folder structure and siblings.txt is reload;
    // not possible to delete blockchain by deleting its folder while
    // system is running; use exclude_blockchain() instead
    virtual void reload_config() = 0;

    // accesses OS env
    virtual std::string get_env_var(std::string name) = 0;
};


class IOrderingNodeSubscription {
public:
    virtual grpr::Transaction get_next() = 0;
    virtual void channel_closed() = 0;
};

class IOrderingNode {
public:
    virtual IOrderingNodeSubscription* create_subscription_for(
                               grpr::Transaction req) = 0;
};

class IHandshakeHandler {
public:
    virtual grpr::Transaction get_response_h0(grpr::Transaction req,
                                              IVersioned* ve) = 0;
    virtual grpr::Transaction get_response_h2(grpr::Transaction req,
                                              IVersioned* ve) = 0;
    virtual grpr::Transaction get_h3_signed_contents() = 0;
};

// nodes have some things in common, such as subcluster blockchain they
// maintain, always; also, each node has private/public key pair as
// its identificator
class INodeFacade {
public:
    virtual ~INodeFacade() {}
    virtual void continue_init_after_sb_done() = 0;
    // doesn't give away ownership
    virtual IVersioned* get_versioned(int version_number) = 0;
    virtual IVersioned* get_current_versioned() = 0;
    virtual void global_log(std::string message) = 0;

    // if handshake is not ongoing returns 0; if force, always return
    // pointer
    virtual IHandshakeHandler* get_handshake_handler(bool force) = 0;
    virtual ISubclusterBlockchain* get_subcluster_blockchain() = 0;
    virtual void continue_init_after_handshake_done() = 0;

    virtual std::string get_sb_ordering_node_endpoint() = 0;
    virtual std::string get_node_type_str() = 0;

    virtual bool ordering_broadcast(IVersioned* ve,
                                    grpr::OrderingRequest* ore) = 0;
};

// this is to support deprecated group register blockchain
class IGroupRegisterFacade {
 public:
    virtual ~IGroupRegisterFacade() {}
    // only once, after group register is ready
    virtual void continue_init_after_group_register_done() = 0;
    virtual IGroupRegisterBlockchain* get_group_register() = 0;
};

// this is to support gradido blockchains, associated with the facade
class IGroupBlockchainFacade {
 public:
    virtual ~IGroupBlockchainFacade() {}

    // terminology: group == blockchain 

    // list of groups is mutable within lifetime of facade
    virtual IBlockchain* get_group_blockchain(std::string group) = 0;
    virtual IBlockchain* create_group_blockchain(GroupInfo gi) = 0;
    virtual IBlockchain* create_or_get_group_blockchain(std::string group) = 0;

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

    class PairedTransactionListener {
    public:
        virtual void on_paired_transaction_done(Transaction t) = 0;
    };
    virtual void on_paired_transaction(grpr::OutboundTransaction br,
                                       grpr::OutboundTransactionDescriptor req) = 0;

    // will check local / remote blockchains and call when data is
    // ready
    virtual void exec_once_paired_transaction_done(
                           std::string group,
                           PairedTransactionListener* ptl,
                           HederaTimestamp hti) = 0;

    // for repeated requests to get paired transaction
    virtual void exec_once_paired_transaction_done(
                           HederaTimestamp hti) = 0;


    virtual bool get_random_sibling_endpoint(std::string& res) = 0;
};

// each and every facade (executable program) implements this; virtual
// functions may throw "unsupported" exception; reason of having such
// class layout is to have single type, which can be passed around, thus
// reducing necessary refactoring when things change (for example, event
// handler initially may require just event queue, later adding some more
// features)
//
// only way to have this verified in compile time would involve using
// templates all over the place (inconvenient and complex)
//
// other possibilities to check things in runtime would involve factory
// methods, type casts (both adds to complexity and essentially work the
// same way as current solution)
//
// keeping it simple here
class IGradidoFacade : 
    public IAbstractFacade, 
    public INodeFacade,
    public IGroupRegisterFacade, 
    public IGroupBlockchainFacade,
    public IOrderingNode {
public:
    virtual void init(const std::vector<std::string>& params,
                      IConfigFactory* config_factory) = 0;
    virtual IAbstractBlockchain* get_any_blockchain(std::string name) = 0;


};


}

#endif
