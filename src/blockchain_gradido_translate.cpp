#include "blockchain_gradido_translate.h"

namespace gradido {

    HederaTimestamp TransactionUtils::translate_Timestamp_from_pb(const ::proto::Timestamp& ts) {
        HederaTimestamp res;
        res.seconds = ts.seconds();
        res.nanos = ts.nanos();
        return res;
    }

    ::proto::Timestamp TransactionUtils::translate_Timestamp_to_pb(const HederaTimestamp& ts) {
        ::proto::Timestamp res;
        res.set_seconds(ts.seconds);
        res.set_nanos(ts.nanos);
        return res;
    }
    HederaAccountID TransactionUtils::translate_AccountID_from_pb(const ::proto::AccountID& val) {
        HederaAccountID res;
        res.shardNum = val.shardnum();
        res.realmNum = val.realmnum();
        res.accountNum = val.accountnum();
        return res;
    }
    ::proto::AccountID TransactionUtils::translate_AccountID_to_pb(const HederaAccountID& val) {
        ::proto::AccountID res;
        res.set_shardnum(val.shardNum);
        res.set_realmnum(val.realmNum);
        res.set_accountnum(val.accountNum);
        return res;
    }

    HederaTopicID TransactionUtils::translate_TopicID_from_pb(const ::proto::TopicID& val) {
        HederaTopicID res;
        res.shardNum = val.shardnum();
        res.realmNum = val.realmnum();
        res.topicNum = val.topicnum();
        return res;
    }
    ::proto::TopicID TransactionUtils::translate_TopicID_to_pb(const HederaTopicID& val) {
        ::proto::TopicID res;
        res.set_shardnum(val.shardNum);
        res.set_realmnum(val.realmNum);
        res.set_topicnum(val.topicNum);
        return res;
    }
    
    HederaTransactionID TransactionUtils::translate_TransactionID_from_pb(const ::proto::TransactionID& val) {
        HederaTransactionID res;
        res.transactionValidStart = translate_Timestamp_from_pb(val.transactionvalidstart());
        res.accountID = translate_AccountID_from_pb(val.accountid());
        return res;
    }
    ::proto::TransactionID TransactionUtils::translate_TransactionID_to_pb(const HederaTransactionID& val) {
        ::proto::TransactionID res;
        ::proto::Timestamp* vs = res.mutable_transactionvalidstart();
        *vs = translate_Timestamp_to_pb(val.transactionValidStart);
        ::proto::AccountID* ai = res.mutable_accountid();
        *ai = translate_AccountID_to_pb(val.accountID);
        return res;
    }

    HederaTransaction TransactionUtils::translate_HederaTransaction_from_pb(const ConsensusTopicResponse& val) {
        HederaTransaction res;
        res.consensusTimestamp = translate_Timestamp_from_pb(val.consensustimestamp());
        memcpy((void*)res.runningHash,
               (void*)val.runninghash().c_str(), 48);
        
        res.sequenceNumber = val.sequencenumber();
        res.runningHashVersion = val.runninghashversion();
        return res;
    }

    ConsensusTopicResponse TransactionUtils::translate_HederaTransaction_to_pb(const HederaTransaction& val) {
        ConsensusTopicResponse res;
        ::proto::Timestamp* tmp = res.mutable_consensustimestamp();
        *tmp = translate_Timestamp_to_pb(val.consensusTimestamp);
        res.set_runninghash(std::string(val.runningHash, val.runningHash + 48));
        res.set_sequencenumber(val.sequenceNumber);
        res.set_runninghashversion(val.runningHashVersion);
        return res;
    }

    Signature TransactionUtils::translate_Signature_from_pb(
                                grpr::SignaturePair sig_pair) {
        Signature res;
        memcpy(res.pubkey, sig_pair.pub_key().c_str(), PUB_KEY_LENGTH);
        memcpy(res.signature, sig_pair.ed25519().c_str(), 
               SIGNATURE_LENGTH);
        return res;
    }

    void TransactionUtils::translate_Signature_to_pb(
                                grpr::SignatureMap* sig_map,
                                const Signature& signature) {
        grpr::SignaturePair* sig_pair = sig_map->add_sig_pair();
        sig_pair->set_pub_key(std::string((char*)signature.pubkey, 
                                          PUB_KEY_LENGTH));
        sig_pair->set_ed25519(std::string((char*)signature.signature, 
                                          SIGNATURE_LENGTH));
    }

    void TransactionUtils::translate_from_ctr(
                           const ConsensusTopicResponse& t, 
                           GroupRegisterRecordBatch& batch) {

        memset(&batch, 0, sizeof(batch));

        try {

            GroupRegisterRecord buff[100];
            memset(buff, 0, sizeof(GroupRegisterRecord) * 100);

            int buff_size = 1;
            GroupRegisterRecord* r0 = buff;

            r0->record_type = (uint8_t)GROUP_RECORD;
            GroupRecord* tt = &r0->group_record;

            grpr::GradidoTransaction gt;
            gt.ParseFromString(t.message());
            grpr::AddGroupToRegister tb;
            tb.ParseFromString(gt.body_bytes());

            // TODO: check overflow
            strcpy((char*)tt->alias, tb.alias().c_str());
            tt->topic_id.shardNum = tb.topic_id().shardnum();
            tt->topic_id.realmNum = tb.topic_id().realmnum();
            tt->topic_id.topicNum = tb.topic_id().topicnum();

            tt->hedera_transaction = translate_HederaTransaction_from_pb(t);
            // TODO: finish here
            //tt->signature = translate_Signature_from_pb();
            
            memcpy(batch.buff, buff, sizeof(GroupRegisterRecord) * buff_size);
            batch.size = buff_size;
        } catch (...) {
            LOG("couldn't translate, resetting");
            batch.reset_blockchain = true;
        }
    }


    void TransactionUtils::translate_from_ctr(
                           const ConsensusTopicResponse& t, 
                           GradidoRecordBatch& batch) {

        //check_structure(t);

        memset(&batch, 0, sizeof(batch));

        try {
        
            GradidoRecord buff[100];
            memset(buff, 0, sizeof(GradidoRecord) * 100);

            // main
            int buff_size = 1;
            GradidoRecord* r0 = buff;
            r0->record_type = (uint8_t)GRADIDO_TRANSACTION;
            Transaction* tt = &r0->transaction;

            grpr::GradidoTransaction gt;
            gt.ParseFromString(t.message());
            grpr::TransactionBody tb;
            tb.ParseFromString(gt.body_bytes());

            tt->version_number = tb.version_number();
            tt->signature_count = gt.sig_map().sig_pair_size();

            if (tt->signature_count < MINIMUM_SIGNATURE_COUNT)
                throw NotEnoughSignatures();
            if (tt->signature_count > MAXIMUM_SIGNATURE_COUNT)
                throw TooManySignatures();

            tt->signature = translate_Signature_from_pb(gt.sig_map().sig_pair()[0]);
        
            tt->hedera_transaction = translate_HederaTransaction_from_pb(t);
            if (tb.has_creation()) {
                tt->transaction_type = (uint8_t)GRADIDO_CREATION;
                tt->gradido_creation.amount = 
                    tb.creation().receiver().amount();
                memcpy(tt->gradido_creation.user, 
                       tb.creation().receiver().pubkey().c_str(), 
                       PUB_KEY_LENGTH);
            } else if (tb.has_group_friends_update()) {
                std::string g = tb.group_friends_update().group();
                // TODO: mention exact pb enum type
                if (tb.group_friends_update().action() == 0) {
                    tt->transaction_type = (uint8_t)ADD_GROUP_FRIEND;
                    memcpy(tt->add_group_friend.group, g.c_str(), g.size());
                } else {
                    tt->transaction_type = (uint8_t)REMOVE_GROUP_FRIEND;
                    memcpy(tt->remove_group_friend.group, g.c_str(), g.size());
                }
            } else if (tb.has_group_member_update()) {
                uint8_t mut = tb.group_member_update().member_update_type();
                std::string u = tb.group_member_update().user_pubkey();

                if (mut == 0) {
                    tt->transaction_type = (uint8_t)ADD_USER;
                    memcpy(tt->add_user.user, u.c_str(), PUB_KEY_LENGTH);
                } else if (mut == 1) {
                    tt->transaction_type = (uint8_t)MOVE_USER_INBOUND;
                    memcpy(tt->move_user_inbound.user, u.c_str(), PUB_KEY_LENGTH);
                    tt->move_user_inbound.paired_transaction_id = 
                        translate_Timestamp_from_pb(tb.group_member_update().
                                                    paired_transaction_id());
                    std::string group = tb.group_member_update().
                        target_group();
                    memcpy(tt->move_user_inbound.other_group, 
                           group.c_str(), group.size());
                } else {
                    tt->transaction_type = (uint8_t)MOVE_USER_OUTBOUND;
                    memcpy(tt->move_user_outbound.user, u.c_str(), PUB_KEY_LENGTH);
                    tt->move_user_outbound.paired_transaction_id = 
                        translate_Timestamp_from_pb(tb.group_member_update().
                                                    paired_transaction_id());
                    std::string group = tb.group_member_update().
                        target_group();
                    memcpy(tt->move_user_outbound.other_group, 
                           group.c_str(), group.size());
                }
            } else if (tb.has_transfer()) {
                grpr::GradidoTransfer g2 = tb.transfer();
                if (g2.has_local()) {
                    tt->transaction_type = (uint8_t)LOCAL_TRANSFER;
                    tt->local_transfer.amount = g2.local().sender().amount();
                    memcpy(tt->local_transfer.sender.user, 
                           g2.local().sender().pubkey().c_str(), 
                           PUB_KEY_LENGTH);
                    memcpy(tt->local_transfer.receiver.user, 
                           g2.local().receiver().c_str(), 
                           PUB_KEY_LENGTH);
                } else if (g2.has_inbound()) {
                    tt->transaction_type = (uint8_t)INBOUND_TRANSFER;
                    tt->inbound_transfer.amount = g2.inbound().sender().
                        amount();
                    memcpy(tt->inbound_transfer.sender.user, 
                           g2.inbound().sender().pubkey().c_str(), 
                           PUB_KEY_LENGTH);
                    memcpy(tt->inbound_transfer.receiver.user, 
                           g2.inbound().receiver().c_str(), 
                           PUB_KEY_LENGTH);
                    tt->inbound_transfer.paired_transaction_id = 
                        translate_Timestamp_from_pb(g2.inbound().
                                                    paired_transaction_id());
                    std::string group = g2.inbound().other_group();
                    memcpy(tt->inbound_transfer.other_group, 
                           group.c_str(), group.size());
                } else if (g2.has_outbound()) {
                    tt->transaction_type = (uint8_t)OUTBOUND_TRANSFER;
                    tt->outbound_transfer.amount = g2.outbound().sender().
                        amount();
                    memcpy(tt->outbound_transfer.sender.user, 
                           g2.outbound().sender().pubkey().c_str(), 
                           PUB_KEY_LENGTH);
                    memcpy(tt->outbound_transfer.receiver.user, 
                           g2.outbound().receiver().c_str(), 
                           PUB_KEY_LENGTH);
                    tt->outbound_transfer.paired_transaction_id = 
                        translate_Timestamp_from_pb(g2.outbound().
                                                    paired_transaction_id());
                    std::string group = g2.outbound().other_group();
                    memcpy(tt->outbound_transfer.other_group, 
                           group.c_str(), group.size());
                }
            }
            tt->result == UNKNOWN;

            std::string memo = tb.memo();
            bool multipart_memo = false;
            if (memo.size() > MEMO_MAIN_SIZE - 1) {
                memcpy(tt->memo, memo.c_str(), MEMO_MAIN_SIZE);
                multipart_memo = true;
            } else 
                memcpy(tt->memo, memo.c_str(), memo.size());

            int sig_part_count = 
                tt->signature_count <= SIGNATURES_IN_MAIN_PART ? 0 : 
                ((int)((tt->signature_count - SIGNATURES_IN_MAIN_PART - 1) / 
                       SIGNATURES_PER_RECORD) + 1);
            tt->parts = 1 + (multipart_memo ? 1 : 0) + sig_part_count;

            // parts
            if (multipart_memo) {
                std::string other_part = memo.substr(
                                                     MEMO_MAIN_SIZE,
                                                     MEMO_PART_SIZE - 1);
                r0 = buff + buff_size;
                buff_size++;

                r0->record_type = (uint8_t)MEMO;
                memcpy(r0->memo, other_part.c_str(), other_part.size());
            }

            int cs = SIGNATURES_IN_MAIN_PART;
            for (int i = 0; i < sig_part_count; i++) {
                r0 = buff + buff_size;
                buff_size++;
                r0->record_type = (uint8_t)SIGNATURES;
                for (int j = 0; j < SIGNATURES_PER_RECORD; j++) {
                    r0->signature[j] = translate_Signature_from_pb(gt.sig_map().sig_pair()[cs++]);
                    if (cs == tt->signature_count)
                        break;
                }
            }

            memcpy(batch.buff, buff, sizeof(GradidoRecord) * buff_size);
            batch.size = buff_size;
        } catch (...) {
            LOG("couldn't translate, resetting");
            batch.reset_blockchain = true;
        }
    }

    /*
    void TransactionUtils::translate_from_br(
                           const grpr::BlockRecord& br, 
                           HashedMultipartTransaction& hmt) {
        memset(&hmt, 0, sizeof(hmt));
        // TODO: put back
        // hmt.rec_count = br.part_size();
        // for (int i = 0; i < hmt.rec_count; i++) {
        //     const grpr::TransactionPart& tp = br.part()[i];
        //     memcpy(hmt.rec + i, tp.record().c_str(), 
        //            sizeof(GradidoRecord));
        //     memcpy(hmt.rec + i, tp.hash().c_str(), BLOCKCHAIN_HASH_SIZE);
        // }
    }
    */

    void TransactionUtils::check_structure(
                           const ConsensusTopicResponse& t) {
        
    }

    /*
    void TransactionUtils::check_structure(
                           const MultipartTransaction& mt) {
        if (mt.rec_count == 0)
            throw RecCountZero();
        if (mt.rec_count > MAX_RECORD_PARTS)
            throw RecCountTooLarge();
        if (mt.rec[0].record_type != GRADIDO_TRANSACTION)
            throw BadFirstRecType();
        for (int i = 1; i < mt.rec_count; i++) {
            if (i == 1) {
                if (mt.rec[i].record_type != MEMO && 
                    mt.rec[i].record_type != SIGNATURES)
                    throw BadRecType(i);
            } else {
                if (mt.rec[i].record_type != SIGNATURES)
                    throw BadRecType(i);
            }
        }
        bool big_memo = mt.rec[0].transaction.memo[MEMO_MAIN_SIZE - 1] != 0;
        if (big_memo &&
            (mt.rec_count < 2 || mt.rec[1].record_type != MEMO))
            throw ExpectedLongMemo();

        if (mt.rec[0].transaction.signature_count == 0)
            throw NotEnoughSignatures();
        
        int max_sig_count = big_memo ? 
            (mt.rec_count - 2) * SIGNATURES_PER_RECORD + 1 : 
            (mt.rec_count - 1) * SIGNATURES_PER_RECORD + 1;
        
        if (mt.rec[0].transaction.signature_count > max_sig_count)
            throw TooManySignatures();

        const Transaction& tt = mt.rec[0].transaction;
        
        check_memo(tt.memo, big_memo ? mt.rec[1].memo : 0);

        // TODO
        check_alias(0);

        // TODO: check hedera hashsum validity
        // TODO: check key non-emptyness
        // TODO: check if bytes are correct

        int first_sig_rec = big_memo ? 2 : 1;
        int signatures_checked = 0;
        bool blank_found = false;
        for (int i = 0; i < MAXIMUM_SIGNATURE_COUNT; i++) {
            if (i < SIGNATURES_IN_MAIN_PART) {
                // assert(SIGNATURES_IN_MAIN_PART == 1);
                if (is_empty(tt.signature))
                    throw EmptySignature();
                signatures_checked++;
            } else {
                int sig_rec = (int)((i - SIGNATURES_IN_MAIN_PART) / 
                                    SIGNATURES_PER_RECORD) + 
                    first_sig_rec;
                if (sig_rec >= mt.rec_count) 
                    break;
                int sig_pos = (int)((i - SIGNATURES_IN_MAIN_PART) / 
                                    SIGNATURES_PER_RECORD) + 
                    first_sig_rec;
                GradidoRecordType rt = (GradidoRecordType)mt.
                    rec[sig_rec].record_type;
                if (rt == SIGNATURES) {
                    if (blank_found) 
                        throw OmittedSignatureInRecord(); 
                    if (is_empty(mt.rec[sig_rec].signature[sig_pos]))
                        throw EmptySignature();
                    signatures_checked++;
                } else if (rt == BLANK) {
                    blank_found = true;
                }
            }
        }
        int sig_count = mt.rec[0].transaction.signature_count;
        if (signatures_checked != sig_count)
            throw SignatureCountNotCorrect();
    }


    void TransactionUtils::translate_to_ctr(
                           const MultipartTransaction& mt,
                           ConsensusTopicResponse& t) {
        check_structure(mt);

        const Transaction& tt = mt.rec[0].transaction;
        t = translate_HederaTransaction_to_pb(tt.hedera_transaction);

        grpr::GradidoTransaction gt;
        grpr::SignatureMap* sig_map = gt.mutable_sig_map();
        grpr::TransactionBody tb;

        tb.set_version_number(tt.version_number);
        translate_Signature_to_pb(sig_map, tt.signature);
        bool big_memo = tt.memo[MEMO_MAIN_SIZE - 1];

        int first_sig = big_memo ? 2 : 1;
        int sig_count = tt.signature_count - 1;
        for (int i = first_sig; i < mt.rec_count; i++) {
            for (int j = 0; j < SIGNATURES_PER_RECORD; j++) {
                if (sig_count == 0)
                    break;
                translate_Signature_to_pb(sig_map, mt.rec[i].signature[j]);
                sig_count--;
            }
        }

        switch ((TransactionType)tt.transaction_type) {
        case GRADIDO_CREATION: {
            auto creation = tb.mutable_creation();
            auto receiver = creation->mutable_receiver();
            receiver->set_amount(tt.gradido_creation.amount);
            receiver->set_pubkey(std::string((char*)tt.gradido_creation.user,
                                             PUB_KEY_LENGTH));
            break;
        }
        case ADD_GROUP_FRIEND: {
            auto gfu = tb.mutable_group_friends_update();
            gfu->set_group(std::string((char*)tt.add_group_friend.group));
            gfu->set_action(grpr::GroupFriendsUpdate::ADD_FRIEND);
            break;
        }
        case REMOVE_GROUP_FRIEND: {
            auto gfu = tb.mutable_group_friends_update();
            gfu->set_group(std::string((char*)tt.remove_group_friend.group));
            gfu->set_action(grpr::GroupFriendsUpdate::REMOVE_FRIEND);
            break;
        }
        case ADD_USER: {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::ADD_USER);
            gmu->set_user_pubkey(std::string((char*)tt.add_user.user, 
                                             PUB_KEY_LENGTH));
            break;
        }
        case MOVE_USER_INBOUND: {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::MOVE_USER_INBOUND);
            gmu->set_user_pubkey(
                 std::string((char*)tt.move_user_inbound.user, 
                             PUB_KEY_LENGTH));
            auto ts = gmu->mutable_paired_transaction_id();
            *ts = translate_Timestamp_to_pb(
                  tt.move_user_inbound.paired_transaction_id);
            gmu->set_target_group(std::string((char*)
                                  tt.move_user_inbound.other_group));
            break;
        }
        case MOVE_USER_OUTBOUND: {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::MOVE_USER_OUTBOUND);
            gmu->set_user_pubkey(
                 std::string((char*)tt.move_user_outbound.user, 
                             PUB_KEY_LENGTH));
            auto ts = gmu->mutable_paired_transaction_id();
            *ts = translate_Timestamp_to_pb(
                  tt.move_user_outbound.paired_transaction_id);
            gmu->set_target_group(std::string((char*)
                                  tt.move_user_outbound.other_group));
            break;
        }
        case LOCAL_TRANSFER: {
            auto transfer = tb.mutable_transfer();
            auto local = transfer->mutable_local();

            auto sender = local->mutable_sender();
            sender->set_amount(tt.local_transfer.amount);
            sender->set_pubkey(std::string((char*)tt.local_transfer.
                                           sender.user, 
                                           PUB_KEY_LENGTH));
            local->set_receiver(std::string((char*)tt.local_transfer.
                                             receiver.user, 
                                             PUB_KEY_LENGTH));
            break;
        }
        case INBOUND_TRANSFER: {
            auto transfer = tb.mutable_transfer();
            auto inbound = transfer->mutable_inbound();

            auto sender = inbound->mutable_sender();
            sender->set_amount(tt.inbound_transfer.amount);
            sender->set_pubkey(std::string((char*)tt.inbound_transfer.
                                           sender.user, 
                                           PUB_KEY_LENGTH));
            inbound->set_receiver(std::string((char*)tt.inbound_transfer.
                                              receiver.user, 
                                              PUB_KEY_LENGTH));
            auto ts = inbound->mutable_paired_transaction_id();
            *ts = translate_Timestamp_to_pb(
                  tt.inbound_transfer.paired_transaction_id);
            inbound->set_other_group(std::string((char*)
                                     tt.inbound_transfer.other_group));
            break;
        }
        case OUTBOUND_TRANSFER: {
            auto transfer = tb.mutable_transfer();
            auto outbound = transfer->mutable_outbound();

            auto sender = outbound->mutable_sender();
            sender->set_amount(tt.outbound_transfer.amount);
            sender->set_pubkey(std::string((char*)tt.outbound_transfer.
                                           sender.user, 
                                           PUB_KEY_LENGTH));
            outbound->set_receiver(std::string((char*)tt.outbound_transfer.
                                              receiver.user, 
                                              PUB_KEY_LENGTH));
            auto ts = outbound->mutable_paired_transaction_id();
            *ts = translate_Timestamp_to_pb(
                  tt.outbound_transfer.paired_transaction_id);
            outbound->set_other_group(std::string((char*)
                                     tt.outbound_transfer.other_group));

            break;
        }
        }

        std::string memo((char*)tt.memo);
        if (big_memo != 0)
            memo += std::string((char*)mt.rec[1].memo);
        tb.set_memo(memo);

        std::string tb_bytes;
        tb.SerializeToString(&tb_bytes);
        gt.set_body_bytes(tb_bytes);        

        std::string gt_bytes;
        gt.SerializeToString(&gt_bytes);

        t.set_message(gt_bytes);
    }
    */
    void TransactionUtils::check_alias(const uint8_t* str) {
    }
    
    void TransactionUtils::check_memo(const uint8_t* str, 
                                      const uint8_t* str2) {
    }

    bool TransactionUtils::is_empty(const Signature& signature) {
        return false;
    }
    
}
