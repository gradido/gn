#ifndef BLOCKCHAIN_GRADIDO_H
#define BLOCKCHAIN_GRADIDO_H

#include "gradido_interfaces.h"
#include "blockchain.h"
#include "blockchain_gradido_def.h"
#include "blockchain_gradido_translate.h"
#include "gradido_messages.h"
#include "utils.h"
#include <queue>

namespace gradido {

class GradidoGroupBlockchain : public IBlockchain {
private:
    IGradidoFacade* gf;
    HederaTopicID topic_id;    
    Blockchain<Transaction> blockchain;

    // takes into account indexes built by Validator
    void calc_dependent_fields(Transaction& tr);
    void update_indexes(Transaction& tr);

    // TODO: different levels of validation; when validating existing
    // chain it would be enough to just check hashes
    // builds data indexes while traversing blockchain
    class Validator : public Blockchain<Transaction>::BlockchainRecordValidator {
    public:
        Validator();
        virtual void validate(uint64_t rec_num, const Transaction& rec);
        Transaction prepare();
    };

    Validator validator;


    class TransactionCompare {
    public:
        bool operator()(const Transaction& lhs, 
                        const Transaction& rhs) const;
    };
    pthread_mutex_t queue_lock;
    std::priority_queue<Transaction, std::vector<Transaction>, TransactionCompare> inbound;

    // idea is to allow only one thread to write to blockchain
    pthread_mutex_t blockchain_lock;
    pthread_t write_owner;
    bool is_busy;

    class BusyGuard final {
    private:
        bool started;
        GradidoGroupBlockchain& ggb;
    public:
        BusyGuard(GradidoGroupBlockchain& ggb) : started(false), 
            ggb(ggb) {}
        ~BusyGuard() {
            if (started) {
                ggb.is_busy = false;
                started = false;
            }
        }
        void start() {
            started = true;
        }
    };
    friend class BusyGuard;

    bool is_sequence_number_ok(const Transaction& tr);
    void append(const Transaction& tr);

public:
    GradidoGroupBlockchain(GroupInfo gi, std::string root_folder,
                           IGradidoFacade* gf);
    virtual ~GradidoGroupBlockchain();
    HederaTopicID get_topic_id();
    virtual void init();


    virtual IBlockchain::AddTransactionResult add_transaction(const Transaction& tr);
    virtual IBlockchain::AddTransactionResult continue_with_transactions();

};

}

#endif
