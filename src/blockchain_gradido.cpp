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
            // TODO: stop transmission if bad
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

    bool GradidoGroupBlockchain::prepare_rec(
                                 MultipartTransaction& tr0) {
        // TODO: put inside protobuf structures, verify signatures

        prepare_rec_cannot_proceed = false;
        Transaction& tr = tr0.rec[0].transaction;

        switch ((TransactionType)tr.transaction_type) {
        case GRADIDO_CREATION: {
            std::string user = std::string(
                               (char*)tr.gradido_creation.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec == user_index.end())
                tr.result = UNKNOWN_LOCAL_USER;
            else {
                tr.gradido_creation.new_balance = 
                    urec->second.current_balance + 
                    tr.gradido_creation.amount;
                tr.gradido_creation.prev_transfer_rec_num = 
                    urec->second.last_record_with_balance;
                tr.result = SUCCESS;
            }
            break;
        }
        case ADD_GROUP_FRIEND: {
            std::string group = std::string((char*)tr.add_group_friend.group);
            auto grec = friend_group_index.find(group);;
            if (grec != friend_group_index.end()) {
                tr.result = GROUP_IS_ALREADY_FRIEND;
            } else {
                tr.result = SUCCESS;
            }
            break;
        }
        case REMOVE_GROUP_FRIEND: {
            std::string group = std::string((char*)tr.remove_group_friend.group);
            auto grec = friend_group_index.find(group);;
            if (grec == friend_group_index.end()) {
                tr.result = GROUP_IS_NOT_FRIEND;
            } else {
                tr.result = SUCCESS;
            }
            break;
        }
        case ADD_USER: {
            std::string user = std::string(
                               (char*)tr.add_user.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec != user_index.end()) {
                tr.result = USER_ALREADY_EXISTS;
            } else {
                tr.result = SUCCESS;
            }
            break;
        }
        case MOVE_USER_INBOUND: {
            std::string user = std::string(
                               (char*)tr.move_user_inbound.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec != user_index.end()) {
                tr.result = USER_ALREADY_EXISTS;
            } else {
                std::string sender_group((char*)tr.move_user_inbound.other_group);
                IBlockchain* b = gf->get_group_blockchain(sender_group);
                if (!b) {
                    prepare_rec_cannot_proceed = true;
                    other_group = sender_group;
                    paired_transaction = tr.move_user_inbound.paired_transaction_id;
                    return false;
                } else {
                    Transaction tt;
                    bool zz = b->get_paired_transaction(
                                 tr.move_user_inbound.paired_transaction_id, tt);
                    if (!zz) {
                        prepare_rec_cannot_proceed = true;
                        other_group = sender_group;
                        paired_transaction = tr.move_user_inbound.paired_transaction_id;
                        return false;
                    } else {
                        if (tt.result != SUCCESS) {
                            tr.result = OUTBOUND_TRANSACTION_FAILED;
                        } else {
                            tr.result = SUCCESS;
                        }
                    }
                }                
            }

            break;
        }
        case MOVE_USER_OUTBOUND: {
            std::string user = std::string(
                               (char*)tr.move_user_outbound.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else {
                tr.result = SUCCESS;
            }
            break;
        }
        case LOCAL_TRANSFER: {
            LocalTransfer& lt = tr.local_transfer;
            std::string sender = std::string(
                               (char*)lt.sender.user, 
                               PUB_KEY_LENGTH);
            auto r_sender = user_index.find(sender);

            std::string receiver = std::string(
                               (char*)lt.receiver.user, 
                               PUB_KEY_LENGTH);
            auto r_receiver = user_index.find(receiver);

            if (r_sender == user_index.end() || 
                r_receiver == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else if (r_sender->second.current_balance < 
                       lt.amount) {
                tr.result = NOT_ENOUGH_GRADIDOS;
            } else {
                lt.sender.new_balance = 
                    r_sender->second.current_balance - 
                    lt.amount;
                lt.receiver.new_balance = 
                    r_receiver->second.current_balance +
                    lt.amount;
                lt.sender.prev_transfer_rec_num = 
                    r_sender->second.last_record_with_balance;
                lt.receiver.prev_transfer_rec_num = 
                    r_receiver->second.last_record_with_balance;
                tr.result = SUCCESS;
            }
            break;
        }            
        case INBOUND_TRANSFER: {
            InboundTransfer& lt = tr.inbound_transfer;
            std::string sender = std::string(
                                 (char*)lt.sender.user, 
                                 PUB_KEY_LENGTH);
            std::string sender_group((char*)lt.other_group);


            std::string receiver = std::string(
                               (char*)lt.receiver.user, 
                               PUB_KEY_LENGTH);
            auto r_receiver = user_index.find(receiver);

            if (r_receiver == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else {

                IBlockchain* b = gf->get_group_blockchain(sender_group);

                if (!b) {
                    prepare_rec_cannot_proceed = true;
                    other_group = sender_group;
                    paired_transaction = lt.paired_transaction_id;
                    return false;
                } else {
                    Transaction tt;
                    bool zz = b->get_paired_transaction(
                                 lt.paired_transaction_id, tt);
                    if (!zz) {
                        prepare_rec_cannot_proceed = true;
                        other_group = sender_group;
                        paired_transaction = lt.paired_transaction_id;
                        return false;
                    } else {
                        if (tt.result != SUCCESS) {
                            tr.result = OUTBOUND_TRANSACTION_FAILED;
                        } else {
                            lt.receiver.new_balance = 
                                r_receiver->second.current_balance +
                                lt.amount;
                            lt.receiver.prev_transfer_rec_num = 
                                r_receiver->second.last_record_with_balance;
                            tr.result = SUCCESS;
                        }
                    }
                }
            }
            break;
        }
        case OUTBOUND_TRANSFER: {
            OutboundTransfer& lt = tr.outbound_transfer;
            std::string sender = std::string(
                               (char*)lt.sender.user, 
                               PUB_KEY_LENGTH);
            auto r_sender = user_index.find(sender);

            if (r_sender == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else if (r_sender->second.current_balance < 
                       lt.amount) {
                tr.result = NOT_ENOUGH_GRADIDOS;
            } else {
                lt.sender.new_balance = 
                    r_sender->second.current_balance - 
                    lt.amount;
                lt.sender.prev_transfer_rec_num = 
                    r_sender->second.last_record_with_balance;
                tr.result = SUCCESS;
            }
            break;
        }
        }
        return true;
    }

    GradidoGroupBlockchain::RVR GradidoGroupBlockchain::validate(const GradidoRecord& rec) {
        if ((validation_buff.rec_count == 0 && 
             (GradidoRecordType)rec.record_type != GRADIDO_TRANSACTION) ||
            (validation_buff.rec_count == MAX_RECORD_PARTS)) {
            validation_buff.rec_count = 0;   
            return GradidoGroupBlockchain::RVR::INVALID;
        }
        validation_buff.rec[validation_buff.rec_count++] = rec;

        if (validation_buff.rec[0].transaction.parts == 
            validation_buff.rec_count) {
            GradidoGroupBlockchain::RVR res = 
                validate_multipart(validation_buff);
            validation_buff.rec_count = 0;   
            return res;
        } else
            return GradidoGroupBlockchain::RVR::NEED_NEXT;
    }

    GradidoGroupBlockchain::RVR 
    GradidoGroupBlockchain::validate_multipart(
                            const MultipartTransaction& mtr) {

        // checking if mtr is correctly split in parts
        if (mtr.rec_count == 0)
            return GradidoGroupBlockchain::RVR::NEED_NEXT;

        if ((GradidoRecordType)mtr.rec[0].record_type != 
            GRADIDO_TRANSACTION)
            return GradidoGroupBlockchain::RVR::INVALID;

        const Transaction& tr = mtr.rec[0].transaction;

        if (tr.parts > MAX_RECORD_PARTS)
            return GradidoGroupBlockchain::RVR::INVALID;

        bool next_is_memo = tr.memo[MEMO_MAIN_SIZE - 1] != 0;
        for (int i = 1; i < tr.parts; i++) {
            if (next_is_memo) {
                next_is_memo = false;
                if ((GradidoRecordType)mtr.rec[i].record_type !=
                    MEMO)
                    return GradidoGroupBlockchain::RVR::INVALID;
            } else {
                if ((GradidoRecordType)mtr.rec[i].record_type !=
                    SIGNATURES)
                    return GradidoGroupBlockchain::RVR::INVALID;
            }
        }

        // replaying preparation, comparing if it is the same
        MultipartTransaction tmp = mtr;
        bool prr = prepare_rec(tmp);
        if (!prr || strncmp((char*)&tmp, (char*)&mtr, 
                    sizeof(MultipartTransaction)))
            return GradidoGroupBlockchain::RVR::INVALID;
    }

    void GradidoGroupBlockchain::added_successfuly(const GradidoRecord& rec) {
        // at this point prepare_rec() is already successfuly called,
        // which means necessary data from other blockchains is 
        // available
        if ((GradidoRecordType)rec.record_type != GRADIDO_TRANSACTION)
            return;
        const Transaction& tr = rec.transaction;
        if ((TransactionResult)tr.result != SUCCESS)
            return; // if not successful, no further processing needed
        switch ((TransactionType)tr.transaction_type) {
        case GRADIDO_CREATION: {
            std::string user = std::string(
                               (char*)tr.gradido_creation.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            urec->second.current_balance += tr.gradido_creation.amount;
            break;
        }
        case ADD_GROUP_FRIEND: {
            std::string group((char*)tr.remove_group_friend.group);
            FriendGroupInfo gi;
            friend_group_index.insert({group, gi});
            break;
        }
        case REMOVE_GROUP_FRIEND: {
            std::string group((char*)tr.remove_group_friend.group);
            friend_group_index.erase(group);
            break;
        }
        case ADD_USER: {
            UserInfo ui = {0};
            user_index.insert({std::string((char*)tr.add_user.user, 
                                           PUB_KEY_LENGTH), ui});
            break;
        }
        case MOVE_USER_INBOUND: {
            UserInfo ui = {0};
            ui.current_balance = tr.move_user_inbound.new_balance;
            ui.last_record_with_balance = transaction_count;
            user_index.insert(
                       {std::string((char*)tr.move_user_inbound.user, 
                                    PUB_KEY_LENGTH), ui});
            break;
        }
        case MOVE_USER_OUTBOUND: {
            std::string user = std::string(
                               (char*)tr.move_user_outbound.user, 
                               PUB_KEY_LENGTH);
            user_index.erase(user);
            break;
        }
        case LOCAL_TRANSFER: {
            {
                std::string user = std::string(
                                   (char*)tr.local_transfer.sender.user, 
                                   PUB_KEY_LENGTH);
                auto urec = user_index.find(user);
                urec->second.current_balance = tr.local_transfer.sender.new_balance;
            }
            {
                std::string user = std::string(
                                   (char*)tr.local_transfer.receiver.user, 
                                   PUB_KEY_LENGTH);
                auto urec = user_index.find(user);
                urec->second.current_balance = tr.local_transfer.receiver.new_balance;
            }
            break;
        }            
        case INBOUND_TRANSFER: {
            {
                std::string user = std::string(
                                   (char*)tr.local_transfer.receiver.user, 
                                   PUB_KEY_LENGTH);
                auto urec = user_index.find(user);
                urec->second.current_balance = tr.local_transfer.receiver.new_balance;
            }
            break;
        }
        case OUTBOUND_TRANSFER: {
            {
                std::string user = std::string(
                                   (char*)tr.local_transfer.sender.user, 
                                   PUB_KEY_LENGTH);
                auto urec = user_index.find(user);
                urec->second.current_balance = tr.local_transfer.sender.new_balance;
            }
            break;
        }
        }
        transaction_count++;
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

        memset(&validation_buff, 0, sizeof(validation_buff));
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
                    {
                        MLock lock0(blockchain_lock);
                        MLock lock(queue_lock); 
                        last_known_rec_seq_num = hsn;
                        inbound.push(tr); // putting back
                        write_owner = 0; // disowning current write 
                        // operation, giving it to event 
                        // chain which updates blockchain
                    }
                    require_transactions(gf->get_conf()->get_sibling_nodes());
                    return; 
                }
            }
            // it is guaranteed that not more than one thread will get
            // to this point at the same time
            bool prr = false;
            {                    
                MLock lock0(blockchain_lock);
                prr = prepare_rec(tr);
            }
            if (!prr) {
                IBlockchain* b = gf->create_or_get_group_blockchain(
                                     other_group);
                ITask* task = new AttemptToContinueBlockchainTask(this);
                {
                    MLock lock0(blockchain_lock);
                    MLock lock(queue_lock); 
                    inbound.push(tr); // putting back
                    write_owner = 0; // disowning current write;  
                }
                b->exec_once_paired_transaction_done(
                   task, paired_transaction);
            } else
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
        if (prepare_rec_cannot_proceed) {
            IBlockchain* b = gf->create_or_get_group_blockchain(
                             other_group);
            ITask* task = new AttemptToContinueBlockchainTask(this);
            b->exec_once_paired_transaction_done(
               task, paired_transaction);
        } else if (num == batch_size) {
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

    bool GradidoGroupBlockchain::get_paired_transaction(
                                 HederaTimestamp hti, 
                                 Transaction tt) {
        return false;
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
            brd.set_group(std::string(gi.alias));
            brd.set_first_record(from);
            brd.set_record_count(count);
            ITask* task = new TryGetRecords(this, gf, brd, endpoints);
            gf->push_task(task);
        }
    }

}
