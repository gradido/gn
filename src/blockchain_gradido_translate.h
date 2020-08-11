#ifndef BLOCKCHAIN_GRADIDO_TRANSLATE_H
#define BLOCKCHAIN_GRADIDO_TRANSLATE_H

#include "gradido_messages.h"

namespace gradido {
    
class TransactionUtils {

private:
    // TODO: optimize with move or putting result as a parameter
    // (although it is likely that compiler does the thing already)
    // TODO: check enum overflows
    
    static HederaTimestamp translate_Timestamp_from_pb(const ::proto::Timestamp& ts) {
        HederaTimestamp res;
        res.seconds = ts.seconds();
        res.nanos = ts.nanos();
        return res;
    }
    static ::proto::Timestamp translate_Timestamp_to_pb(const HederaTimestamp& ts) {
        ::proto::Timestamp res;
        res.set_seconds(ts.seconds);
        res.set_nanos(ts.nanos);
        return res;
    }
    static HederaAccountID translate_AccountID_from_pb(const ::proto::AccountID& val) {
        HederaAccountID res;
        res.shardNum = val.shardnum();
        res.realmNum = val.realmnum();
        res.accountNum = val.accountnum();
        return res;
    }
    static ::proto::AccountID translate_AccountID_to_pb(const HederaAccountID& val) {
        ::proto::AccountID res;
        res.set_shardnum(val.shardNum);
        res.set_realmnum(val.realmNum);
        res.set_accountnum(val.accountNum);
        return res;
    }

    static HederaTopicID translate_TopicID_from_pb(const ::proto::TopicID& val) {
        HederaTopicID res;
        res.shardNum = val.shardnum();
        res.realmNum = val.realmnum();
        res.topicNum = val.topicnum();
        return res;
    }
    static ::proto::TopicID translate_TopicID_to_pb(const HederaTopicID& val) {
        ::proto::TopicID res;
        res.set_shardnum(val.shardNum);
        res.set_realmnum(val.realmNum);
        res.set_topicnum(val.topicNum);
        return res;
    }
    
    static HederaTransactionID translate_TransactionID_from_pb(const ::proto::TransactionID& val) {
        HederaTransactionID res;
        res.transactionValidStart = translate_Timestamp_from_pb(val.transactionvalidstart());
        res.accountID = translate_AccountID_from_pb(val.accountid());
        return res;
    }
    static ::proto::TransactionID translate_TransactionID_to_pb(const HederaTransactionID& val) {
        ::proto::TransactionID res;
        ::proto::Timestamp* vs = res.mutable_transactionvalidstart();
        *vs = translate_Timestamp_to_pb(val.transactionValidStart);
        ::proto::AccountID* ai = res.mutable_accountid();
        *ai = translate_AccountID_to_pb(val.accountID);
        return res;
    }

    static Participant translate_Participant_from_pb(const grpr::Participant& val) {
        Participant res;
        res.user_id = val.user_id();
        return res;
    }
    static grpr::Participant translate_Participant_to_pb(const Participant& val) {
        grpr::Participant res;
        res.set_user_id(val.user_id);
        return res;
    }
    

    static RemoteParticipant translate_RemoteParticipant_from_pb(const grpr::RemoteParticipant& val) {
        RemoteParticipant res;
        res.user_id = val.user_id();
        res.group_id = val.group_id();
        return res;
    }
    static grpr::RemoteParticipant translate_RemoteParticipant_to_pb(const RemoteParticipant& val) {
        grpr::RemoteParticipant res;
        res.set_user_id(val.user_id);
        res.set_group_id(val.group_id);
        return res;
    }

    static LocalTransfer translate_LocalTransfer_from_pb(const grpr::LocalTransfer& val) {
        LocalTransfer res;
        res.user_from = translate_Participant_from_pb(val.user_from());
        res.user_to = translate_Participant_from_pb(val.user_to());
        return res;
    }
    static grpr::LocalTransfer translate_LocalTransfer_to_pb(const LocalTransfer& val) {
        grpr::LocalTransfer res;
        grpr::Participant* p0 = res.mutable_user_from();
        *p0 = translate_Participant_to_pb(val.user_from);
        grpr::Participant* p1 = res.mutable_user_to();
        *p1 = translate_Participant_to_pb(val.user_to);
        return res;
    }

    static InboundTransfer translate_InboundTransfer_from_pb(const grpr::InboundTransfer& val) {
        InboundTransfer res;
        res.user_from = translate_RemoteParticipant_from_pb(val.user_from());
        res.user_to = translate_Participant_from_pb(val.user_to());
        return res;
    }
    static grpr::InboundTransfer translate_InboundTransfer_to_pb(const InboundTransfer& val) {
        grpr::InboundTransfer res;
        grpr::RemoteParticipant* p0 = res.mutable_user_from();
        *p0 = translate_RemoteParticipant_to_pb(val.user_from);
        grpr::Participant* p1 = res.mutable_user_to();
        *p1 = translate_Participant_to_pb(val.user_to);
        return res;
    }

    static OutboundTransfer translate_OutboundTransfer_from_pb(const grpr::OutboundTransfer& val) {
        OutboundTransfer res;
        res.user_from = translate_Participant_from_pb(val.user_from());
        res.user_to = translate_RemoteParticipant_from_pb(val.user_to());
        return res;
    }
    static grpr::OutboundTransfer translate_OutboundTransfer_to_pb(const OutboundTransfer& val) {
        grpr::OutboundTransfer res;
        grpr::Participant* p0 = res.mutable_user_from();
        *p0 = translate_Participant_to_pb(val.user_from);
        grpr::RemoteParticipant* p1 = res.mutable_user_to();
        *p1 = translate_RemoteParticipant_to_pb(val.user_to);
        return res;
    }
        
    static GradidoTransaction translate_GradidoTransaction_from_pb(const grpr::GradidoTransaction& val) {
        GradidoTransaction res;
        res.gradido_transfer_type = (GradidoTransferType)val.gradido_transfer_type();
        switch (res.gradido_transfer_type) {
        case LOCAL:
            res.data.local = translate_LocalTransfer_from_pb(val.local());
            break;
        case INBOUND:
            res.data.inbound = translate_InboundTransfer_from_pb(val.inbound());
            break;
        case OUTBOUND:
            res.data.outbound = translate_OutboundTransfer_from_pb(val.outbound());
            break;
        }
        res.amount = val.amount();
        res.amount_with_deduction = val.amount_with_deduction();
        memcpy((void*)res.sender_signature,
               (void*)val.sender_signature().c_str(), 64);
        return res;
    }
    static grpr::GradidoTransaction translate_GradidoTransaction_to_pb(const GradidoTransaction& val) {
        grpr::GradidoTransaction res;
        res.set_gradido_transfer_type((grpr::GradidoTransaction_GradidoTransferType)val.gradido_transfer_type);
        switch (val.gradido_transfer_type) {
        case LOCAL: {
            grpr::LocalTransfer* tmp = res.mutable_local();
            *tmp = translate_LocalTransfer_to_pb(val.data.local);
            break;
        }
        case INBOUND: {
            grpr::InboundTransfer* tmp = res.mutable_inbound();
            *tmp = translate_InboundTransfer_to_pb(val.data.inbound);
            break;
        }
        case OUTBOUND: {
            grpr::OutboundTransfer* tmp = res.mutable_outbound();
            *tmp = translate_OutboundTransfer_to_pb(val.data.outbound);
            break;
        }
        }
        res.set_amount(val.amount);
        res.set_amount_with_deduction(val.amount_with_deduction);
        res.set_sender_signature(std::string(val.sender_signature, val.sender_signature + 64));
        return res;
    }

    static GroupUpdate translate_GroupUpdate_from_pb(const grpr::GroupUpdate& val) {
        GroupUpdate res;
        res.is_disabled = val.is_disabled();
        res.update_author_user_id = val.update_author_user_id();
        memcpy((void*)res.update_author_signature,
               (void*)val.update_author_signature().c_str(), 64);
        return res;
    }
    static grpr::GroupUpdate translate_GroupUpdate_to_pb(const GroupUpdate& val) {
        grpr::GroupUpdate res;
        res.set_is_disabled(val.is_disabled);
        res.set_update_author_user_id(val.update_author_user_id);
        res.set_update_author_signature(std::string(val.update_author_signature, val.update_author_signature + 64));
        return res;
    }

    static GroupFriendsUpdate translate_GroupFriendsUpdate_from_pb(const grpr::GroupFriendsUpdate& val) {
        GroupFriendsUpdate res;
        res.is_disabled = val.is_disabled();
        res.update_author_user_id = val.update_author_user_id();
        memcpy((void*)res.update_author_signature,
               (void*)val.update_author_signature().c_str(), 64);
        res.group_id = val.group_id();
        return res;
    }
    static grpr::GroupFriendsUpdate translate_GroupFriendsUpdate_to_pb(const GroupFriendsUpdate& val) {
        grpr::GroupFriendsUpdate res;
        res.set_is_disabled(val.is_disabled);
        res.set_update_author_user_id(val.update_author_user_id);
        res.set_update_author_signature(std::string(val.update_author_signature, val.update_author_signature + 64));
        res.set_group_id(val.group_id);
        return res;
    }

    static GroupMemberUpdate translate_GroupMemberUpdate_from_pb(const grpr::GroupMemberUpdate& val) {
        GroupMemberUpdate res;
        res.is_disabled = val.is_disabled();
        res.update_author_user_id = val.update_author_user_id();
        memcpy((void*)res.update_author_signature,
               (void*)val.update_author_signature().c_str(), 64);
        res.user_id = val.user_id();
        res.member_status = (MemberStatus)val.member_status();
        res.member_update_type = (MemberUpdateType)val.member_update_type();
        memcpy((void*)res.public_key,
               (void*)val.public_key().c_str(), 32);
        return res;
    }
    static grpr::GroupMemberUpdate translate_GroupMemberUpdate_to_pb(const GroupMemberUpdate& val) {
        grpr::GroupMemberUpdate res;
        res.set_is_disabled(val.is_disabled);
        res.set_update_author_user_id(val.update_author_user_id);
        res.set_update_author_signature(std::string(val.update_author_signature, val.update_author_signature + 64));
        res.set_user_id(val.user_id);
        res.set_member_status((grpr::GroupMemberUpdate_MemberStatus)val.member_status);
        res.set_member_update_type((grpr::GroupMemberUpdate_MemberUpdateType)val.member_update_type);
        res.set_public_key(std::string(val.public_key, val.public_key + 32));
        return res;
    }

    static Transaction translate_Transaction_from_pb(const grpr::Transaction& val) {
        Transaction res;
        res.transaction_type = (TransactionType)val.transaction_type();
        switch (res.transaction_type) {
        case GRADIDO_TRANSACTION:
            res.data.gradido_transaction = translate_GradidoTransaction_from_pb(val.gradido_transaction());
            break;
        case GROUP_UPDATE:
            res.data.group_update = translate_GroupUpdate_from_pb(val.group_update());
            break;
        case GROUP_FRIENDS_UPDATE:
            res.data.group_friends_update = translate_GroupFriendsUpdate_from_pb(val.group_friends_update());
            break;
        case GROUP_MEMBER_UPDATE:
            res.data.group_member_update = translate_GroupMemberUpdate_from_pb(val.group_member_update());
            break;
        }
        res.validation_schema = (ValidationSchema)val.validation_schema();
        res.version_number = val.version_number();
        memcpy((void*)res.reserved,
               (void*)val.reserved().c_str(), 32);
        
        return res;
    }
    static grpr::Transaction translate_Transaction_to_pb(const Transaction& val) {
        grpr::Transaction res;
        res.set_transaction_type((grpr::Transaction_TransactionType)val.transaction_type);
        switch (val.transaction_type) {
        case GRADIDO_TRANSACTION: {
            grpr::GradidoTransaction* tmp = res.mutable_gradido_transaction();
            *tmp = translate_GradidoTransaction_to_pb(val.data.gradido_transaction);
            break;
        }
        case GROUP_UPDATE: {
            grpr::GroupUpdate* tmp = res.mutable_group_update();
            *tmp = translate_GroupUpdate_to_pb(val.data.group_update);
            break;
        }
        case GROUP_FRIENDS_UPDATE: {
            grpr::GroupFriendsUpdate* tmp = res.mutable_group_friends_update();
            *tmp = translate_GroupFriendsUpdate_to_pb(val.data.group_friends_update);
            break;
        }
        case GROUP_MEMBER_UPDATE: {
            grpr::GroupMemberUpdate* tmp = res.mutable_group_member_update();
            *tmp = translate_GroupMemberUpdate_to_pb(val.data.group_member_update);
            break;
        }
        }        
        res.set_validation_schema((grpr::Transaction_ValidationSchema)val.validation_schema);
        res.set_version_number(val.version_number);
        res.set_reserved(std::string(val.reserved, val.reserved + 32));
        return res;
    }

    static HederaTransaction translate_HederaTransaction_from_pb(const ConsensusTopicResponse& val) {
        HederaTransaction res;
        res.consensusTimestamp = translate_Timestamp_from_pb(val.consensustimestamp());
        memcpy((void*)res.runningHash,
               (void*)val.runninghash().c_str(), 48);
        
        res.sequenceNumber = val.sequencenumber();
        res.runningHashVersion = val.runninghashversion();
        return res;
    }
    static ConsensusTopicResponse translate_HederaTransaction_to_pb(const HederaTransaction& val) {
        ConsensusTopicResponse res;
        ::proto::Timestamp* tmp = res.mutable_consensustimestamp();
        *tmp = translate_Timestamp_to_pb(val.consensusTimestamp);
        res.set_runninghash(std::string(val.runningHash, val.runningHash + 48));
        res.set_sequencenumber(val.sequenceNumber);
        res.set_runninghashversion(val.runningHashVersion);
        return res;
    }
    
public:
    static Transaction translate_from_pb(const ConsensusTopicResponse& tr) {

        grpr::Transaction tx;
        tx.ParseFromString(tr.message());

        Transaction res = translate_Transaction_from_pb(tx);
        res.hedera_transaction = translate_HederaTransaction_from_pb(tr);
        return res;
    }

    static ConsensusTopicResponse translate_to_pb(const Transaction& tr) {
        ConsensusTopicResponse res = translate_HederaTransaction_to_pb(tr.hedera_transaction);
        grpr::Transaction tx = translate_Transaction_to_pb(tr);
        std::string message;
        tx.SerializeToString(&message);
        res.set_message(message);
        return res;
    }

    static bool validate_pb_signature(const grpr::Transaction& pb) {
    }
    
};
 
}

#endif
