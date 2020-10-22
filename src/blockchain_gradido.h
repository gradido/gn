#ifndef BLOCKCHAIN_GRADIDO_H
#define BLOCKCHAIN_GRADIDO_H

#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "blockchain_gradido_translate.h"
#include "gradido_messages.h"
#include "utils.h"
#include <queue>
#include <map>
#include <Poco/Path.h>

namespace gradido {

class GradidoGroupBlockchain : public IBlockchain {
public:

    class GroupRegisterValidator {
    };

    class GradidoRecordValidator {
        bool is_valid(const GradidoRecord* rec, uint32_t count) {
            return true;
        }

    };

private:
    GroupRegisterValidator group_validator;
    GradidoRecordValidator gradido_validator;

private:
    GroupInfo gi;
    IGradidoFacade* gf;
    HederaTopicID topic_id;    

    Blockchain<GradidoRecord> blockchain;

    struct UserInfo {
        GradidoValue current_balance;
        uint64_t last_record_with_balance;
    };

    struct FriendGroupInfo {
    };

    // both are guarded by blockchain_lock
    std::map<std::string, UserInfo> user_index;
    std::map<std::string, FriendGroupInfo> friend_group_index;

    // for testing purposes is set to true
    bool omit_previous_transactions;

    // takes into account indexes built by Validator
    bool prepare_rec(MultipartTransaction& tr);
    // set by prepare_rec(); cannot return directly, as may be used
    // by blockchain (via validate())
    bool prepare_rec_cannot_proceed;
    std::string other_group;
    HederaTimestamp paired_transaction;

    class TransactionCompare {
    public:
        bool operator()(const MultipartTransaction& lhs, 
                        const MultipartTransaction& rhs) const;
    };
    pthread_mutex_t queue_lock;
    std::priority_queue<MultipartTransaction, std::vector<MultipartTransaction>, TransactionCompare> inbound;


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

    void append(const MultipartTransaction& tr);

    uint64_t transaction_count;

    MultipartTransaction validation_buff;
    RVR validate_multipart(const MultipartTransaction& mtr);

public:
    GradidoGroupBlockchain(GroupInfo gi, Poco::Path root_folder,
                           IGradidoFacade* gf);
    virtual ~GradidoGroupBlockchain();
    HederaTopicID get_topic_id();
    virtual void init();

    virtual void add_transaction(const MultipartTransaction& tr);
    virtual void add_transaction(const HashedMultipartTransaction& tr);

    virtual void continue_with_transactions();
    virtual void continue_validation();

    virtual grpr::BlockRecord get_block_record(uint64_t seq_num);
    virtual bool get_paired_transaction(HederaTimestamp hti, 
                                        Transaction tt);
    virtual void exec_once_validated(ITask* task);
    virtual void exec_once_paired_transaction_done(
                           ITask* task, 
                           HederaTimestamp hti);
    virtual uint64_t get_transaction_count();

    virtual void require_transactions(std::vector<std::string> endpoints);
};

}

#endif
