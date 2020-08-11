#ifndef BLOCKCHAIN_GRADIDO_DEF_H
#define BLOCKCHAIN_GRADIDO_DEF_H

#include <cstdint>

namespace gradido {
// TODO: add blockchain for group list as well

// change with care; blockchain validity depends on those structs

#define GRADIDO_BLOCK_SIZE 1000

enum TransactionType {
    GRADIDO_TRANSACTION=0,
    GROUP_UPDATE,
    GROUP_FRIENDS_UPDATE,
    GROUP_MEMBER_UPDATE
};

enum GradidoTransferType {
    LOCAL=0,
    INBOUND,
    OUTBOUND
};

struct HederaTimestamp {
    int64_t seconds;
    int32_t nanos;
};

struct HederaAccountID {
    int64_t shardNum;
    int64_t realmNum;
    int64_t accountNum;
};

struct HederaTopicID {
    int64_t shardNum;
    int64_t realmNum;
    int64_t topicNum;
};

struct HederaTransactionID  {
    HederaTimestamp transactionValidStart;
    HederaAccountID accountID;
};

struct HederaTransaction {
    // TODO: consider includind topic_id
    HederaTimestamp consensusTimestamp;
    char runningHash[48];
    uint64_t sequenceNumber;
    uint64_t runningHashVersion;
};

struct ParticipantBase {
    uint64_t user_id;
};

struct Participant : public ParticipantBase {

    // TODO: consider including those for RemoteParticipant as well, as
    // they could speed up search
    
    uint64_t prev_user_rec_num; // refers to record in blockchain which
                                // contains previous Gradido transaction
                                // involving this user; this is only
                                // needed to speed up searches;
                                // blockchain can be validated without it;
                                // TODO: consider if needed; indexes can
                                // be built during validation, if not only
                                // too large to fit in RAM, in general
    uint64_t new_balance;
};

struct RemoteParticipant : public ParticipantBase {
    uint64_t group_id;
};

struct LocalTransfer {
    Participant user_from;
    Participant user_to;
};

struct InboundTransfer {
    RemoteParticipant user_from;
    Participant user_to;
};

struct OutboundTransfer {
    Participant user_from;
    RemoteParticipant user_to;
};


struct GradidoTransaction {
    GradidoTransferType gradido_transfer_type;
    union {
        LocalTransfer local;
        InboundTransfer inbound;
        OutboundTransfer outbound;
    } data;
    uint64_t amount;
    uint64_t amount_with_deduction; // includes Gradido deduction
    char sender_signature[64];
};

struct Disableable {
    bool is_disabled;
};

struct UpdateAuthor {
    uint64_t update_author_user_id;
    char update_author_signature[64];
};

struct GroupUpdate : public Disableable, UpdateAuthor {
    // assuming topic_id stays unchanged forever; for now there aren't
    // things to update apart from disabling / enabling group
};

struct GroupFriendsUpdate : public Disableable, UpdateAuthor {
    uint64_t group_id;
};

enum MemberStatus {
    OWNER=0,
    MEMBER
};

enum MemberUpdateType {

    // only owner can manipulate with users for now
    ADD_USER=0,
    UPDATE_USER,
    MOVE_USER_INBOUND,
    MOVE_USER_OUTBOUND,
    DISABLE_USER
};

// first transaction is ADD_USER with user_id of group's creator;
// for this transaction update_author_user_id is set to 0,
// member_status is OWNER
struct GroupMemberUpdate : public Disableable, UpdateAuthor {
    uint64_t user_id;
    MemberStatus member_status;
    MemberUpdateType member_update_type;

    // TODO: target group for user move
    
    char public_key[32];
};

// this is included to allow further customizations of how permissions
// are decided for various operations (for example, in future some
// things may get more strict, still blockchain would need to validate
// anyway also for older ways)
//
// in other words, this refers to certain way how a transaction is
// validated in the code
enum ValidationSchema {

    // group disable: allowed only to owner
    // add, disable, update: allowed only to owner
    // move user: allowed to user himself
    // add group friend: allowed only to owner
    DEFAULT=0
};

enum TransactionResult {
    SUCCESS=0,
    NOT_ENOUGH_GRADIDOS,
    NOT_ALLOWED,
    INVALID_SIGNATURE
};

struct Transaction {
    HederaTransaction hedera_transaction;
    TransactionType transaction_type;
    ValidationSchema validation_schema;
    union {
        GradidoTransaction gradido_transaction;
        GroupUpdate group_update;
        GroupFriendsUpdate group_friends_update;
        GroupMemberUpdate group_member_update;
    } data;

    // this is not part of data sent by Hedera itself, however, it can
    // be included in payload, as transactionID is generated on user
    // side; here it is necessary only for cross-group transfer
    // validations (to quickly find other part of transfer in respective
    // blokchain); however, it can be used in debugging as well, so
    // it appears here; if 36 bytes are too much, then it can be stored
    // only inside inbound/outbound transfer data
    HederaTransactionID transaction_id;
    
    TransactionResult result;
    // to facilitate small updates, if they are necessary
    char version_number;
    char reserved[32];

};
 
}

#endif
