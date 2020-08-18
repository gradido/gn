#include "blockchain_gradido.h"
#include <Poco/File.h>

namespace gradido {

    class RecordReceiverImpl : public ICommunicationLayer::RecordReceiver {
    private:
        IBlockchain* b;
        std::vector<std::string> endpoints;
    public:
        RecordReceiverImpl(IBlockchain* b, 
                           std::vector<std::string> endpoints) : 
            b(b), endpoints(endpoints) {
        }
        virtual void on_record(grpr::BlockRecord br) {
            if (br.success()) {
                Transaction t = TransactionUtils::translate_Transaction_from_pb(br.transaction());
                // TODO: implement br.hash() comparison with local hash
                b->add_transaction(t);
            } else {
                b->require_records(endpoints);
            }
        }
    };

    class TryGetRecords : public ITask {
    private:
        IBlockchain* b;
        IGradidoFacade* gf;
        grpr::BlockRangeDescriptor brd;
        std::vector<std::string> endpoints;
    public:
        TryGetRecords(IBlockchain* b, IGradidoFacade* gf,
                      grpr::BlockRangeDescriptor brd,
                      std::vector<std::string> endpoints) : 
            b(b), gf(gf), brd(brd),
            endpoints(endpoints) {
        }
        virtual void run() {
            std::string endpoint = endpoints.back();
            endpoints.pop_back();
            std::shared_ptr<ICommunicationLayer::RecordReceiver> rr = 
                std::make_shared<RecordReceiverImpl>(b, endpoints);
            gf->get_communications()->require_records(endpoint, brd, rr);
        }
    };

    class RequireRecordsTask : public ITask {
    private:
        IBlockchain* b;
        std::vector<std::string> endpoints;
    public:
        RequireRecordsTask(IBlockchain* b, 
                           std::vector<std::string> endpoints) : 
            b(b), endpoints(endpoints) {
        }
        virtual void run() {
            b->require_records(endpoints);
        }
    };

    class AttemptToContinueBlockchainTask : public ITask {
    private:
        IBlockchain* b;
    public:
        AttemptToContinueBlockchainTask(IBlockchain* b) : b(b) {
        }
        virtual void run() {
            b->continue_with_transactions();
        }
    };

    class ProcessTransactionTask : public ITask {
    private:
        IBlockchain* b;
        ConsensusTopicResponse transaction;
    public:
        ProcessTransactionTask(IBlockchain* b,
                               const ConsensusTopicResponse& transaction) :
            b(b), transaction(transaction) {
        }
        virtual void run() {
            Transaction t = TransactionUtils::translate_from_pb(transaction);
            b->add_transaction(t);
        }
    };

    class GradidoTransactionListener : public ICommunicationLayer::TransactionListener {
    private:
        IBlockchain* b;
        IGradidoFacade* gf;
        bool first_message;
    public:
        GradidoTransactionListener(IBlockchain* b, IGradidoFacade* gf) : 
            b(b), gf(gf), first_message(true) {
        }
        virtual void on_transaction(ConsensusTopicResponse& transaction) {
            if (first_message)
                // GRPC sends empty message as first message, 
                // apparently always
                first_message = false;
            else
                gf->push_task(new ProcessTransactionTask(
                                  b, transaction));
        }
        virtual void on_reconnect() {
            // TODO
        }
    };

    class BlockchainValidator : public ITask {
    private:
        IBlockchain* b;
    public:
        BlockchainValidator(IBlockchain* b) : b(b) {}
        virtual void run() {
            b->continue_validation();
        }
    };

    void GradidoGroupBlockchain::prepare_rec_and_indexes(Transaction& tr) {
        uint64_t seq_num = tr.hedera_transaction.sequenceNumber;
        switch (tr.transaction_type) {
        case GRADIDO_TRANSACTION: {
            GradidoTransaction& tr2 = tr.data.gradido_transaction;
            switch (tr2.gradido_transfer_type) {
            case LOCAL: {
                auto& u0 = tr2.data.local.user_from;
                auto& u1 = tr2.data.local.user_to;
                auto user_from = user_index.find(u0.user_id);
                if (user_from == user_index.end()) {
                    tr.result = BAD_USER_ID;
                    return;
                } 
                auto ufr = user_from->second;
                if (ufr.is_disabled) {
                    tr.result = USER_DISABLED;
                    return;
                }
                auto user_to = user_index.find(u1.user_id);
                if (user_to == user_index.end()) {
                    tr.result = BAD_USER_ID;
                    return;
                } 
                auto uto = user_to->second;
                if (uto.is_disabled) {
                    tr.result = USER_DISABLED;
                    return;
                }
                if (uto.current_balance < tr2.amount) {
                    // TODO: amount with deduction?
                    tr.result = NOT_ENOUGH_GRADIDOS;
                    return;
                }
                ufr.current_balance -= tr2.amount;
                uto.current_balance += tr2.amount;
                ufr.last_record_with_balance = seq_num;
                uto.last_record_with_balance = seq_num;
                user_from->second = ufr;
                user_to->second = uto;

                tr2.data.local.user_from.new_balance = ufr.current_balance;
                tr2.data.local.user_to.new_balance = uto.current_balance;
                tr2.data.local.user_from.prev_user_rec_num = seq_num;
                tr2.data.local.user_to.prev_user_rec_num = seq_num;
                break;
            }
            case INBOUND: {
                break;
            }
            case OUTBOUND: {
                break;
            }
            }

            break;
        }
        case GROUP_UPDATE: {
            // TODO
            break;
        }
        case GROUP_FRIENDS_UPDATE: {
            // TODO
            break;
        }
        case GROUP_MEMBER_UPDATE: {
            break;
        }
        }
        tr.result = SUCCESS;
    }

    bool GradidoGroupBlockchain::validate(const Transaction& rec) {
        return true;
    }

    void GradidoGroupBlockchain::added_successfuly(const Transaction& rec) {
        // TODO: build indexes, etc.
    }
    

    GradidoGroupBlockchain::GradidoGroupBlockchain(GroupInfo gi,
                                                   Poco::Path root_folder,
                                                   IGradidoFacade* gf) :
        gi(gi),
        gf(gf),
        topic_id(gi.topic_id),
        omit_previous_transactions(false),
        blockchain(gi.alias,
                   root_folder,
                   GRADIDO_BLOCK_SIZE,
                   this),
        write_owner(0),
        write_counter(1),
        is_busy(true),
        seq_num_offset(1),
        first_rec_came(false)
    {
        SAFE_PT(pthread_mutex_init(&queue_lock, 0));
        SAFE_PT(pthread_mutex_init(&blockchain_lock, 0));

        Poco::Path pp = root_folder;
        Poco::Path pp2 = pp.append(".is-first-seq-num-non-zero");
        Poco::File pf(pp2);
        if (pf.isFile())
            omit_previous_transactions = true;
    }

    GradidoGroupBlockchain::~GradidoGroupBlockchain() {
        pthread_mutex_destroy(&queue_lock);
        pthread_mutex_destroy(&blockchain_lock);
    }

    HederaTopicID GradidoGroupBlockchain::get_topic_id() {
        return topic_id;
    }

    void GradidoGroupBlockchain::init() {
        LOG(("initializing blockchain " + gi.get_directory_name()));
        std::string endpoint = gf->get_conf()->get_hedera_mirror_endpoint();
        gf->get_communications()->receive_gradido_transactions(
                                  endpoint,
                                  topic_id,
                                  std::make_shared<GradidoTransactionListener>(this, gf));
        gf->push_task(new BlockchainValidator(this));
    }

    void GradidoGroupBlockchain::add_transaction(const Transaction& tr) {
        {
            MLock lock(queue_lock); 
            inbound.push(tr);
        }

        continue_with_transactions();
    }

    void GradidoGroupBlockchain::continue_with_transactions() {
        int batch_size = gf->get_conf()->get_blockchain_append_batch_size();
        BusyGuard bd(*this);
        while (1) {
            Transaction tr;
            {
                MLock lock(queue_lock); 
                if (inbound.size() == 0) {
                    // TODO: add tasks waiting in task queues
                    return;
                } else {
                    if (is_busy) {
                        if (write_owner != bd.get_owner())
                            return;
                    } else {
                        bd.start();
                    }
                }
                if (batch_size <= 0) {
                    ITask* task = new AttemptToContinueBlockchainTask(this);
                    gf->push_task(task);
                    return;
                }
                tr = inbound.top();
                inbound.pop();
            }
            batch_size--;

            if (omit_previous_transactions && !first_rec_came) {
                first_rec_came = true;
                seq_num_offset = tr.hedera_transaction.sequenceNumber;
            } else {
                uint64_t expected_seq_num = 0;
                {
                    MLock lock(blockchain_lock);
                    expected_seq_num = blockchain.get_rec_count() + 
                        seq_num_offset;
                }
                if (tr.hedera_transaction.sequenceNumber < 
                    expected_seq_num)
                    continue; // just drop it
                if (tr.hedera_transaction.sequenceNumber > 
                    expected_seq_num) {
                    // no use to proceed further, as there is a hole
                    MLock lock0(blockchain_lock);
                    MLock lock(queue_lock); 
                    last_known_rec_seq_num = 
                        tr.hedera_transaction.sequenceNumber;
                    inbound.push(tr); // putting back
                    write_owner = 0; // disowning current write operation, 
                    // giving it to event chain which updates
                    // blockchain
                    require_records(gf->get_conf()->get_sibling_nodes());
                    return; 
                }
            }
            prepare_rec_and_indexes(tr);
            append(tr);
        }        
    }

    bool GradidoGroupBlockchain::TransactionCompare::operator()(
                    const Transaction& lhs, 
                    const Transaction& rhs) const {

        return lhs.hedera_transaction.sequenceNumber < 
            rhs.hedera_transaction.sequenceNumber;
    }

    void GradidoGroupBlockchain::append(const Transaction& tr) {
        MLock lock(blockchain_lock); 
        blockchain.append(tr);
    }

    void GradidoGroupBlockchain::continue_validation() {
        if (omit_previous_transactions) {
            MLock lock2(queue_lock);
            is_busy = false;
            ITask* task = new AttemptToContinueBlockchainTask(this);
            gf->push_task(task);

            LOG("OMMITTT!!!");
            return;
        }

        int batch_size = gf->get_conf()->get_blockchain_init_batch_size();
        MLock lock(blockchain_lock); 
        int num = blockchain.validate_next_batch(batch_size);
        if (num == batch_size) {
            ITask* task = new BlockchainValidator(this);
            gf->push_task(task);
        } else {
            MLock lock2(queue_lock);
            is_busy = false;
            ITask* task = new AttemptToContinueBlockchainTask(this);
            gf->push_task(task);
        }
    }

    grpr::BlockRecord GradidoGroupBlockchain::get_block_record(uint64_t seq_num) {
        grpr::BlockRecord res;
        res.set_success(false);
        grpr::Transaction* tr1 = res.mutable_transaction();
        MLock lock(blockchain_lock);
        if (seq_num < blockchain.get_rec_count()) {
            std::string hash;
            Transaction tr0 = blockchain.get_rec(seq_num, hash);
            *tr1 = TransactionUtils::translate_Transaction_to_pb(tr0);
            res.set_success(true);
            res.set_hash(hash);
        }
        return res;
    }
    void GradidoGroupBlockchain::exec_once_validated(ITask* task) {
        // TODO
    }
    void GradidoGroupBlockchain::exec_once_paired_transaction_done(
                                 ITask* task, 
                                 HederaTimestamp hti) {
        // TODO
    }
    
    uint64_t GradidoGroupBlockchain::get_total_rec_count() {
        MLock lock(blockchain_lock);
        return blockchain.get_rec_count();
    }

    void GradidoGroupBlockchain::require_records(std::vector<std::string> endpoints) {
        // TODO: shuffle endpoints
        MLock lock(blockchain_lock); 
        uint64_t from = blockchain.get_rec_count() + seq_num_offset;
        assert(last_known_rec_seq_num >= from);
        uint64_t count = last_known_rec_seq_num - from + 1;

        if (count > 0) {
            grpr::BlockRangeDescriptor brd;
            brd.set_group_id(gi.group_id);
            brd.set_first_record(from);
            brd.set_record_count(count);
            ITask* task = new TryGetRecords(this, gf, brd, endpoints);
            gf->push_task(task);
        }
    }

    void GradidoGroupBlockchain::add_transaction(
                                 const Transaction& tr, 
                                 std::string rec_hash_to_check) {
        // TODO: implement
    }

    

}
