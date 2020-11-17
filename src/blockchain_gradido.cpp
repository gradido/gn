#include "blockchain_gradido.h"
#include <Poco/File.h>


namespace gradido {

    bool BlockchainGradido::is_valid(const GradidoRecord* rec, 
                                 uint32_t count) {
        // TODO
        return true;
    }


    bool BlockchainGradido::add_index_data() {

        StorageType::ExitCode ec;
        for (uint32_t i = indexed_blocks; 
             i < storage.get_checksum_valid_block_count(); i++) {
            StorageType::Record* block = storage.get_block(i, ec);
            for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                StorageType::Record* rec = block + j;

                if (rec->type == StorageType::RecordType::CHECKSUM)
                    break;
                GradidoRecord* pp = &rec->payload;

                if (pp->record_type == 
                    GradidoRecordType::GRADIDO_TRANSACTION && 
                    pp->transaction.result == (uint8_t)TransactionResult::SUCCESS) {

                    const Transaction& tr = pp->transaction;

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
                        ui.last_record_with_balance = storage.get_rec_count();
                        user_index.insert({std::string((char*)tr.add_user.user, 
                                                       PUB_KEY_LENGTH), ui});
                        break;
                    }
                    case MOVE_USER_INBOUND: {
                        UserInfo ui = {0};
                        ui.current_balance = tr.move_user_inbound.new_balance;
                        ui.last_record_with_balance = i * GRADIDO_BLOCK_SIZE + j;
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
                        outbound_transactions.insert({tr.move_user_outbound.paired_transaction_id, tr});

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
                                                           (char*)tr.inbound_transfer.receiver.user, 
                                                           PUB_KEY_LENGTH);
                            auto urec = user_index.find(user);
                            urec->second.current_balance = tr.inbound_transfer.receiver.new_balance;
                        }
                        break;
                    }
                    case OUTBOUND_TRANSFER: {
                        {
                            std::string user = std::string(
                                                           (char*)tr.outbound_transfer.sender.user, 
                                                           PUB_KEY_LENGTH);
                            auto urec = user_index.find(user);
                            urec->second.current_balance = tr.outbound_transfer.sender.new_balance;
                            outbound_transactions.insert({tr.outbound_transfer.paired_transaction_id, tr});
                        }
                        break;
                    }
                    }
                }
            }
        }
        return true;
    }

    bool BlockchainGradido::get_latest_transaction_id(uint64_t& res) {
        if (storage.get_rec_count() == 0)
            return false;
        StorageType::ExitCode ec;
        StorageType::Record* rec = storage.get_record(
                                   storage.get_rec_count() - 2, ec);
        GradidoRecord& gr = rec->payload;
        // TODO: stucturally bad message
        res = gr.transaction.hedera_transaction.sequenceNumber;
        return true;
    }

    BlockchainGradido::BlockchainGradido(std::string name,
                                         Poco::Path root_folder,
                                         IGradidoFacade* gf,
                                         HederaTopicID topic_id) : 
        Parent(name, root_folder, gf, topic_id), 
        waiting_for_paired(false) {}



    bool BlockchainGradido::get_transaction_id(uint64_t& res, 
                                               const Batch<GradidoRecord>& rec) {
        // TODO: structurally bad message
        res = rec.buff[0].transaction.hedera_transaction.sequenceNumber;
        return true;
    }

    bool BlockchainGradido::calc_fields_and_update_indexes_for(
                        Batch<GradidoRecord>& b) {

        if (waiting_for_paired)
            return false;

        // TODO: should be used from outside
        bool prepare_rec_cannot_proceed = false;

        Transaction& tr = b.buff[0].transaction;

        switch ((TransactionType)tr.transaction_type) {
        case GRADIDO_CREATION: {
            std::string user = std::string(
                               (char*)tr.gradido_creation.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec == user_index.end())
                tr.result = UNKNOWN_LOCAL_USER;
            else if (tr.gradido_creation.amount < 0) 
                tr.result = NEGATIVE_AMOUNT;
            else {
                tr.gradido_creation.new_balance = 
                    urec->second.current_balance + 
                    tr.gradido_creation.amount;
                tr.gradido_creation.prev_transfer_rec_num = 
                    urec->second.last_record_with_balance;
                tr.result = SUCCESS;

                auto urec = user_index.find(user);
                urec->second.current_balance += tr.gradido_creation.amount;
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
                FriendGroupInfo gi;
                friend_group_index.insert({group, gi});
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
                friend_group_index.erase(group);
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
                UserInfo ui = {0};
                ui.last_record_with_balance = storage.get_rec_count();
                user_index.insert({std::string((char*)tr.add_user.user, 
                                               PUB_KEY_LENGTH), ui});

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

                Transaction tt;
                bool found = (other_group.compare(sender_group) == 0 &&
                              paired_tr.transaction_type == (uint8_t)TransactionType::MOVE_USER_OUTBOUND &&
                              paired_tr.move_user_outbound.paired_transaction_id == tr.move_user_inbound.paired_transaction_id);

                if (!b && !found) {
                    prepare_rec_cannot_proceed = true;
                    other_group = sender_group;
                    paired_transaction = tr.move_user_inbound.paired_transaction_id;
                    waiting_for_paired = true;
        
                    gf->exec_once_paired_transaction_done(
                                         other_group,
                                         this,
                                         paired_transaction);
                    return false;
                } else {
                    uint64_t pt_seq_num;
                    bool zz;
                    if (found) {
                        zz = true;
                        tt = paired_tr;
                    } else {
                        zz = b->get_paired_transaction(
                          tr.move_user_inbound.paired_transaction_id, 
                          pt_seq_num);
                        zz = b->get_transaction(pt_seq_num, tt);
                    }
                    if (!zz) {
                        prepare_rec_cannot_proceed = true;
                        other_group = sender_group;
                        paired_transaction = tr.move_user_inbound.paired_transaction_id;
                        waiting_for_paired = true;
                        gf->exec_once_paired_transaction_done(
                                             other_group,
                                             this,
                                             paired_transaction);
                        return false;
                    } else {
                        if (tt.result != SUCCESS) {
                            tr.result = OUTBOUND_TRANSACTION_FAILED;
                        } else {
                            tr.result = SUCCESS;

                            UserInfo ui = {0};
                            ui.current_balance = tr.move_user_inbound.new_balance;
                            ui.last_record_with_balance = storage.get_rec_count();
                            user_index.insert(
                                              {std::string((char*)tr.move_user_inbound.user, 
                                                           PUB_KEY_LENGTH), ui});

                        }
                    }
                }                
            }

            break;
        }
        case MOVE_USER_OUTBOUND: {
            outbound_transactions.insert({tr.move_user_outbound.paired_transaction_id, tr});
            std::string user = std::string(
                               (char*)tr.move_user_outbound.user, 
                               PUB_KEY_LENGTH);
            auto urec = user_index.find(user);
            if (urec == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else {
                tr.result = SUCCESS;
                user_index.erase(user);
                outbound_transactions.insert({tr.move_user_outbound.paired_transaction_id, tr});

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
            } else if (lt.amount < 0) {
                tr.result = NEGATIVE_AMOUNT;
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
                {
                    auto urec = user_index.find(sender);
                    urec->second.current_balance = tr.local_transfer.sender.new_balance;
                }
                {
                    auto urec = user_index.find(receiver);
                    urec->second.current_balance = tr.local_transfer.receiver.new_balance;
                }

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

                Transaction tt;
                bool found = (other_group.compare(sender_group) == 0 &&
                              paired_tr.transaction_type == (uint8_t)TransactionType::OUTBOUND_TRANSFER &&
                              paired_tr.outbound_transfer.paired_transaction_id == tr.inbound_transfer.paired_transaction_id);

                if (!b && !found) {
                    prepare_rec_cannot_proceed = true;
                    other_group = sender_group;
                    paired_transaction = lt.paired_transaction_id;
                    gf->exec_once_paired_transaction_done(
                                             other_group,
                                             this,
                                             paired_transaction);
                    return false;
                } else {
                    uint64_t pt_seq_num;
                    bool zz;
                    if (found) {
                        zz = true;
                        tt = paired_tr;
                    } else {
                        zz = b->get_paired_transaction(
                             lt.paired_transaction_id, pt_seq_num);
                        zz = b->get_transaction(pt_seq_num, tt);
                    }
                    if (!zz) {
                        prepare_rec_cannot_proceed = true;
                        other_group = sender_group;
                        paired_transaction = lt.paired_transaction_id;
                        gf->exec_once_paired_transaction_done(
                                             other_group,
                                             this,
                                             paired_transaction);
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
                            {
                                std::string user = std::string(
                                                               (char*)tr.inbound_transfer.receiver.user, 
                                                               PUB_KEY_LENGTH);
                                auto urec = user_index.find(user);
                                urec->second.current_balance = tr.inbound_transfer.receiver.new_balance;
                            }

                        }
                    }
                }
            }
            break;
        }
        case OUTBOUND_TRANSFER: {
            outbound_transactions.insert({tr.outbound_transfer.paired_transaction_id, tr});
            OutboundTransfer& lt = tr.outbound_transfer;
            std::string sender = std::string(
                               (char*)lt.sender.user, 
                               PUB_KEY_LENGTH);
            auto r_sender = user_index.find(sender);

            if (r_sender == user_index.end()) {
                tr.result = UNKNOWN_LOCAL_USER;
            } else if (lt.amount < 0) {
                tr.result = NEGATIVE_AMOUNT;
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
                {
                    std::string user = std::string(
                                                   (char*)tr.outbound_transfer.sender.user, 
                                                   PUB_KEY_LENGTH);
                    auto urec = user_index.find(user);
                    urec->second.current_balance = tr.outbound_transfer.sender.new_balance;
                    outbound_transactions.insert({tr.outbound_transfer.paired_transaction_id, tr});
                }
            }
            break;
        }
        }
        return true;
    }

    void BlockchainGradido::clear_indexes() {
        user_index.clear();
        friend_group_index.clear();
        outbound_transactions.clear();
    }

    bool BlockchainGradido::get_paired_transaction(HederaTimestamp hti, 
                                                   uint64_t& seq_num) {
        MLock lock(main_lock);

        auto pt = outbound_transactions.find(hti);
        if (pt == outbound_transactions.end()) 
            return false;
        Transaction t = pt->second;
        seq_num = t.hedera_transaction.sequenceNumber;
        return true;
    }

    void BlockchainGradido::on_paired_transaction_done(Transaction t) {
        {
            MLock lock(main_lock);
            waiting_for_paired = false;
            paired_tr = t;
        }
        gf->push_task(new ContinueTransactionsTask<GradidoRecord>(this));
    }

    bool BlockchainGradido::get_transaction(uint64_t seq_num, 
                                            Transaction& t) {
        if (seq_num >= storage.get_rec_count())
            return false;
        StorageType::ExitCode ec;
        StorageType::Record* rec = storage.get_record(seq_num, ec);
        t = rec->payload.transaction;
        return true;
    }



}
