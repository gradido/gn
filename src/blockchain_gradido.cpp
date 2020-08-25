#include "blockchain_gradido.h"
#include <Poco/File.h>

namespace gradido {

    class PushRecordsTask : public ITask {
    private:
        IBlockchain* b;
        grpr::BlockRecord br;
    public:
        PushRecordsTask(IBlockchain* b, grpr::BlockRecord br) : 
            b(b), br(br) {}
        virtual void run() {
            // TODO: reordering, if necessary
            HashedMultipartTransaction hmt;
            TransactionUtils::translate_from_br(br, hmt);
            b->add_transaction(hmt);
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
            b->require_transactions(endpoints);
        }
    };

    class RecordReceiverImpl : public ICommunicationLayer::RecordReceiver {
    private:
        IBlockchain* b;
        std::vector<std::string> endpoints;
        IGradidoFacade* gf;
    public:
        RecordReceiverImpl(IBlockchain* b, 
                           std::vector<std::string> endpoints,
                           IGradidoFacade* gf) : 
            b(b), endpoints(endpoints), gf(gf) {}
        virtual void on_record(grpr::BlockRecord br) {
            if (br.success()) {
                gf->push_task(new PushRecordsTask(b, br));
            } else {
                gf->push_task(new RequireRecordsTask(b, endpoints));
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
                std::make_shared<RecordReceiverImpl>(b, endpoints, gf);
            gf->get_communications()->require_transactions(endpoint, brd, rr);
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
            MultipartTransaction mt;
            TransactionUtils::translate_from_ctr(transaction, mt);
            b->add_transaction(mt);
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
            std::cerr << "kukurznis " << std::endl;
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

    void GradidoGroupBlockchain::prepare_rec_and_indexes(
                                 MultipartTransaction& tr0) {
        Transaction* tr = &tr0.rec[0].transaction;
        uint64_t seq_num = tr->hedera_transaction.sequenceNumber;
        uint64_t rec_num = blockchain.get_rec_count();
        switch ((TransactionType)tr->transaction_type) {
        case GRADIDO_CREATION:

            break;
        case ADD_GROUP_FRIEND:
            break;
        case REMOVE_GROUP_FRIEND:
            break;
        case ADD_USER:
            // TODO

            break;

        case MOVE_USER_INBOUND:
            // TODO
            break;
            
        case MOVE_USER_OUTBOUND:
            // TODO

            break;
        case LOCAL_TRANSFER: {
            LocalTransfer& tr2 = tr->local_transfer;

            uint8_t* u0 = tr2.sender.user;
            uint8_t* u1 = tr2.receiver.user;
            GradidoValue amount = tr2.amount;
            auto user_from = user_index.find(std::string((char*)u0, 
                                                  PUB_KEY_LENGTH));
            if (user_from == user_index.end()) {
                tr->result = BAD_LOCAL_USER_ID;
                return;
            } 
            UserInfo ufr = user_from->second;
            if (ufr.current_balance < amount) {
                // TODO: amount with deduction?
                tr->result = NOT_ENOUGH_GRADIDOS;
                return;
            }
            auto user_to = user_index.find(std::string((char*)u1, 
                                                       PUB_KEY_LENGTH));
            if (user_to == user_index.end()) {
                tr->result = BAD_LOCAL_USER_ID;
                return;
            } 
            UserInfo uto = user_to->second;
            ufr.current_balance -= amount;
            uto.current_balance += amount;

            tr2.sender.new_balance = ufr.current_balance;
            tr2.sender.prev_transfer_rec_num = 
                ufr.last_record_with_balance;
            tr2.receiver.new_balance = uto.current_balance;
            tr2.receiver.prev_transfer_rec_num = 
                uto.last_record_with_balance;

            ufr.last_record_with_balance = rec_num;
            uto.last_record_with_balance = rec_num;
            user_from->second = ufr;
            user_to->second = uto;
            break;
        }            
        case INBOUND_TRANSFER:
            break;
            
        case OUTBOUND_TRANSFER:
            break;
        default:
            // TODO
            break;
        }
        tr->result = SUCCESS;
    }

    GradidoGroupBlockchain::RVR GradidoGroupBlockchain::validate(const GradidoRecord& rec) {
        return GradidoGroupBlockchain::RVR::VALID;
    }

    void GradidoGroupBlockchain::added_successfuly(const GradidoRecord& rec) {
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
        first_rec_came(false),
        transaction_count(0)
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

    void GradidoGroupBlockchain::add_transaction(const MultipartTransaction& tr) {
        {
            MLock lock(queue_lock); 
            std::cerr << "kukurznis2 " << tr.rec_count << "; " << (int)tr.rec[0].record_type << "; " << (int)tr.rec[0].transaction.version_number << std::endl;

            inbound.push(tr);
        }
        continue_with_transactions();
    }

    void GradidoGroupBlockchain::add_transaction(const HashedMultipartTransaction& tr) {
        // TODO
    }

    void GradidoGroupBlockchain::continue_with_transactions() {
        int batch_size = gf->get_conf()->get_blockchain_append_batch_size();
        BusyGuard bd(*this);
        while (1) {
            MultipartTransaction tr;
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
                seq_num_offset = tr.rec[0].transaction.hedera_transaction.sequenceNumber;
            } else {
                uint64_t expected_seq_num = 0;
                {
                    MLock lock(blockchain_lock);
                    expected_seq_num = transaction_count + 
                        seq_num_offset;
                }
                uint64_t hsn = tr.rec[0].transaction.hedera_transaction.
                    sequenceNumber;
                if (hsn < expected_seq_num)
                    continue; // just drop it
                if (hsn > expected_seq_num) {
                    // no use to proceed further, as there is a hole
                    MLock lock0(blockchain_lock);
                    MLock lock(queue_lock); 
                    last_known_rec_seq_num = hsn;
                    inbound.push(tr); // putting back
                    write_owner = 0; // disowning current write operation, 
                    // giving it to event chain which updates
                    // blockchain
                    require_transactions(gf->get_conf()->get_sibling_nodes());
                    return; 
                }
            }
            prepare_rec_and_indexes(tr);
            append(tr);
        }        
    }

    bool GradidoGroupBlockchain::TransactionCompare::operator()(
                    const MultipartTransaction& lhs, 
                    const MultipartTransaction& rhs) const {

        return 
            lhs.rec[0].transaction.hedera_transaction.sequenceNumber < 
            rhs.rec[0].transaction.hedera_transaction.sequenceNumber;
    }

    void GradidoGroupBlockchain::append(const MultipartTransaction& tr) {
        MLock lock(blockchain_lock); 
        // TODO: check output
        for (int i = 0; i < tr.rec_count; i++)
            blockchain.append(tr.rec[i]);
    }

    void GradidoGroupBlockchain::continue_validation() {
        if (omit_previous_transactions) {
            MLock lock2(queue_lock);
            is_busy = false;
            ITask* task = new AttemptToContinueBlockchainTask(this);
            gf->push_task(task);
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

        MLock lock(blockchain_lock);

        if (transaction_count <= seq_num)
            return res;

        std::string hash;
        GradidoRecord rec;
        bool found = false;
        uint64_t i = 0;
        // TODO: optimize; should find by division + block bound index
        for (i = 0; i < blockchain.get_rec_count(); i++) {
            rec = blockchain.get_rec(i, hash);
            if ((GradidoRecordType)rec.record_type == 
                GRADIDO_TRANSACTION && rec.transaction.hedera_transaction.sequenceNumber == seq_num) {
                found = true;
                break;
            }
        }

        if (found) {
            bool first = true;
            while (true) {
                rec = blockchain.get_rec(i, hash);
                if ((GradidoRecordType)rec.record_type ==  
                    GRADIDO_TRANSACTION) {
                    if (!first) 
                        break;
                    first = false;
                }
                std::string rec_str((char*)&rec, sizeof(GradidoRecord));
                grpr::TransactionPart* tr1 = res.add_part();
                tr1->set_hash(hash);
                tr1->set_record(rec_str);
                if (++i >= blockchain.get_rec_count())
                    break;
            }
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
    
    uint64_t GradidoGroupBlockchain::get_transaction_count() {
        MLock lock(blockchain_lock);
        return transaction_count;
    }

    void GradidoGroupBlockchain::require_transactions(std::vector<std::string> endpoints) {

        // TODO: shuffle endpoints
        MLock lock(blockchain_lock); 
        uint64_t from = transaction_count + seq_num_offset;
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

}
