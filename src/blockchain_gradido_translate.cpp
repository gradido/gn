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

    void TransactionUtils::translate_from_ctr(
                           const ConsensusTopicResponse& t, 
                           MultipartTransaction& mt) {
        // TODO: checks against corrupted data
        memset(&mt, 0, sizeof(mt));

        // main
        mt.rec_count = 1;
        GradidoRecord* r0 = mt.rec;
        r0->record_type = (uint8_t)GRADIDO_TRANSACTION;
        Transaction* tt = &r0->transaction;

        grpr::GradidoTransaction gt;
        gt.ParseFromString(t.message());
        grpr::TransactionBody tb;
        tb.ParseFromString(gt.body_bytes());

        std::cerr << "kukurznis1.2 " << t.message() << "; " << 
            t.message().size() << std::endl;
        std::cerr << "kukurznis1.3 " << gt.body_bytes() << "; " << 
            gt.body_bytes().size() << std::endl;


        std::cerr << "kukurznis1.5 " << tb.version_number() << "; " << 
            tb.memo() << std::endl;


        tt->version_number = tb.version_number();
        tt->signature_count = gt.sig_map().sig_pair_size();
        // assert(tt->signature_count > 0)
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

        int sig_part_count = tt->signature_count <= 1 ? 0 : 
            ((int)((tt->signature_count - 1) / 
                   SIGNATURES_PER_RECORD) + 1);
        tt->parts = 1 + (multipart_memo ? 1 : 0) + sig_part_count;

        // parts
        if (multipart_memo) {
            std::string other_part = memo.substr(
                                     MEMO_MAIN_SIZE,
                                     MEMO_PART_SIZE - 1);
            r0 = mt.rec + mt.rec_count;
            mt.rec_count++;

            r0->record_type = (uint8_t)MEMO;
            memcpy(r0->memo, other_part.c_str(), other_part.size());
        }

        int cs = 1;
        for (int i = 0; i < sig_part_count; i++) {
            r0 = mt.rec + mt.rec_count;
            mt.rec_count++;
            r0->record_type = (uint8_t)SIGNATURES;

            for (int j = 0; j < SIGNATURES_PER_RECORD; j++) {
                r0->signature[j] = translate_Signature_from_pb(gt.sig_map().sig_pair()[cs++]);
                if (cs == tt->signature_count)
                    break;
            }
        }
    }

    void TransactionUtils::translate_from_br(
                           const grpr::BlockRecord& br, 
                           HashedMultipartTransaction& hmt) {
        memset(&hmt, 0, sizeof(hmt));
        hmt.rec_count = br.part_size();
        for (int i = 0; i < hmt.rec_count; i++) {
            const grpr::TransactionPart& tp = br.part()[i];
            memcpy(hmt.rec + i, tp.record().c_str(), 
                   sizeof(GradidoRecord));
            memcpy(hmt.rec + i, tp.hash().c_str(), BLOCKCHAIN_HASH_SIZE);
        }
    }

    void TransactionUtils::translate_to_ctr(
                           const MultipartTransaction& mt,
                           ConsensusTopicResponse& t) {
        // TODO
    }

}
