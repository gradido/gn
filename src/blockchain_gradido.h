#ifndef BLOCKCHAIN_GRADIDO_H
#define BLOCKCHAIN_GRADIDO_H

#include "gradido_interfaces.h"
#include "blockchain.h"
#include "blockchain_gradido_def.h"
#include "blockchain_gradido_translate.h"
#include "gradido_messages.h"
#include "utils.h"
#include <queue>
#include <map>
#include <Poco/Path.h>

namespace gradido {

class GradidoGroupBlockchain : public IBlockchain, Blockchain<Transaction>::BlockchainRecordValidator {
public:
    // TODO: different levels of validation; when validating existing
    // chain it would be enough to just check hashes
    virtual bool validate(const Transaction& rec);

    // builds data indexes while traversing blockchain
    virtual void added_successfuly(const Transaction& rec);

private:
    GroupInfo gi;
    IGradidoFacade* gf;
    HederaTopicID topic_id;    
    Blockchain<Transaction> blockchain;

    struct UserInfo {
        uint64_t current_balance;
        char pub_key[64];
        uint64_t last_record_with_balance;
        bool is_disabled;
    };

    struct FriendGroupInfo {
        bool is_disabled;
    };

    std::map<uint64_t, UserInfo> user_index;
    std::map<uint64_t, FriendGroupInfo> friend_group_index;

    // for testing purposes is set to true
    bool omit_previous_transactions;

    // takes into account indexes built by Validator
    void prepare_rec_and_indexes(Transaction& tr);

    class TransactionCompare {
    public:
        bool operator()(const Transaction& lhs, 
                        const Transaction& rhs) const;
    };
    pthread_mutex_t queue_lock;
    std::priority_queue<Transaction, std::vector<Transaction>, TransactionCompare> inbound;


    // idea is to allow only one thread to write to blockchain
    pthread_mutex_t blockchain_lock;
    uint64_t write_owner;
    uint64_t write_counter;
    bool is_busy;

    uint64_t last_known_rec_seq_num;

    // this is needed when testing as it is useful to allow sequence
    // number be larger than 1 for first transaction
    uint64_t seq_num_offset;
    bool first_rec_came;

    class BusyGuard final {
    private:
        bool started;
        GradidoGroupBlockchain& ggb;
        uint64_t owner;
    public:
        BusyGuard(GradidoGroupBlockchain& ggb) : started(false), 
            ggb(ggb) {
                owner = ggb.write_counter++;
            }
        ~BusyGuard() {
            MLock lock(ggb.queue_lock);
            if (started && owner == ggb.write_owner) {
                ggb.is_busy = false;
                started = false;
            }
        }
        void start() {
            started = true;
            ggb.is_busy = true;
            ggb.write_owner = owner;
        }
        uint64_t get_owner() {
            return owner;
        }
    };
    friend class BusyGuard;

    void append(const Transaction& tr);

public:
    GradidoGroupBlockchain(GroupInfo gi, Poco::Path root_folder,
                           IGradidoFacade* gf);
    virtual ~GradidoGroupBlockchain();
    HederaTopicID get_topic_id();
    virtual void init();


    virtual void add_transaction(const Transaction& tr);
    virtual void add_transaction(const Transaction& tr, 
                                 std::string rec_hash_to_check);

    virtual void continue_with_transactions();
    virtual void continue_validation();

    virtual grpr::BlockRecord get_block_record(uint64_t seq_num);
    virtual void exec_once_validated(ITask* task);
    virtual void exec_once_paired_transaction_done(
                           ITask* task, 
                           HederaTimestamp hti);
    virtual uint64_t get_total_rec_count();

    virtual void require_records(std::vector<std::string> endpoints);

};

}

#endif
