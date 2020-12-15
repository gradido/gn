#ifndef BLOCKCHAIN_GRADIDO_DEF_H
#define BLOCKCHAIN_GRADIDO_DEF_H

#include <cstdint>
#include <math.h>
#include "ed25519/ed25519.h"


namespace gradido {

// change with care; blockchain validity depends on those structs

// all structures has to be set to 0 before putting any data in them

#define GRADIDO_BLOCK_SIZE 1000

#define HEDERA_RUNNING_HASH_LENGTH 48

#define MAX_RECORD_PARTS 5

// not flexible; would take some more adjustments when changing this
#define SIGNATURES_IN_MAIN_PART 1

#define NON_SIGNATURE_PARTS 2
#define SIGNATURE_PARTS (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS)
#define MINIMUM_SIGNATURE_COUNT 1
#define MAXIMUM_SIGNATURE_COUNT (SIGNATURES_PER_RECORD * (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS) + SIGNATURES_IN_MAIN_PART)

#define PUB_KEY_LENGTH 32
#define SIGNATURE_LENGTH 64

#define MEMO_MAIN_SIZE 16
#define MEMO_FULL_SIZE 150

#define GROUP_REGISTER_NAME "group-register"

#define BLOCKCHAIN_CHECKSUM_SIZE SHA_512_SIZE

// group alias field ends with null characters to have exact length and
// have null-terminated string as well
#define GROUP_ALIAS_LENGTH 16

typedef double GradidoValue;

struct User {
    uint8_t user[PUB_KEY_LENGTH];
};

struct UserState {
    GradidoValue new_balance;
    uint64_t prev_transfer_rec_num;
};

struct Signature {
    uint8_t pubkey[PUB_KEY_LENGTH];
    uint8_t signature[SIGNATURE_LENGTH];
};

enum TransactionType {
    GRADIDO_CREATION=1,
    ADD_GROUP_FRIEND,
    REMOVE_GROUP_FRIEND,
    ADD_USER,
    MOVE_USER_INBOUND,
    MOVE_USER_OUTBOUND,
    LOCAL_TRANSFER,
    INBOUND_TRANSFER,
    OUTBOUND_TRANSFER
};

struct HederaTimestamp {
    int64_t seconds;
    int32_t nanos;

    bool operator<(const HederaTimestamp& ts) const {
        return ts.seconds > seconds || (ts.seconds == seconds &&
                                        ts.nanos > nanos);
    }
    bool operator==(const HederaTimestamp& ts) const {
        return ts.seconds == seconds && ts.nanos == nanos;
    }

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
    HederaTimestamp consensusTimestamp;
    uint8_t runningHash[HEDERA_RUNNING_HASH_LENGTH];
    uint64_t sequenceNumber;
    uint64_t runningHashVersion;
};

struct AbstractTransferOp {
    GradidoValue amount;
};

struct UserTransfer : public User, UserState {};

struct LocalTransfer : public AbstractTransferOp {
    UserTransfer sender;
    UserTransfer receiver;
};

struct PairedTransaction {
    HederaTimestamp paired_transaction_id;
    uint8_t other_group[GROUP_ALIAS_LENGTH];
};

struct InboundTransfer : public AbstractTransferOp, PairedTransaction {
    User sender;
    UserTransfer receiver;
};
struct OutboundTransfer : public AbstractTransferOp, PairedTransaction {
    UserTransfer sender;
    User receiver;
};

struct FriendUpdate {
    uint8_t group[GROUP_ALIAS_LENGTH];
};

// first transaction is ADD_USER with user_id of group's creator
struct AddUser : public User {};
struct MoveUser : public User, PairedTransaction, UserState {};

enum TransactionResult {
    SUCCESS=0,

    // not yet decided
    UNKNOWN,

    // transfer not possible, sender doesn't have enough funds
    NOT_ENOUGH_GRADIDOS,

    // user has to be in local group
    UNKNOWN_LOCAL_USER,

    // user has to be in remote group
    UNKNOWN_REMOTE_USER,

    // group has to be in list of friends
    UNKNOWN_GROUP,

    // not possible to finish inbound transaction, as outbound
    // transaction failed
    OUTBOUND_TRANSACTION_FAILED,

    // for add_user and move_user_inbound
    USER_ALREADY_EXISTS,

    // for add/remove friend group
    GROUP_IS_ALREADY_FRIEND,
    GROUP_IS_NOT_FRIEND
};

struct GradidoCreation : public UserTransfer, AbstractTransferOp {
    uint32_t target_date_seconds;
};

struct TransactionCommonHeader {
    // starts with 1; it may affect the way how transaction is parsed and
    // validated; version_number is the same for all parts of single
    // transaction; for structurally_bad_message it may be 0, if
    // version number cannot be obtained from data
    uint8_t version_number;

    // other parts may follow this record; if not, then parts == 1;
    // cannot be larger than MAX_RECORD_PARTS for structurally validated
    // transaction (one which is therefore translated); for
    // structurally_bad_message it is unlimited
    uint8_t parts;

    HederaTransaction hedera_transaction;
};

struct Transaction : public TransactionCommonHeader {

    // additional signatures can be stored in more parts
    Signature signature;
    uint8_t signature_count;

    uint8_t transaction_type; // enum TransactionType
    union {
        GradidoCreation gradido_creation;
        FriendUpdate add_group_friend;
        FriendUpdate remove_group_friend;
        AddUser add_user;
        MoveUser move_user_inbound;
        MoveUser move_user_outbound;
        LocalTransfer local_transfer;
        InboundTransfer inbound_transfer;
        OutboundTransfer outbound_transfer;
    };

    uint8_t result; // enum TransactionResult


    // first part of memo, to avoid multi-parts, if possible (assuming,
    // usually memos are small); has to be null-terminated string;
    // memo is first of possible parts; if this->memo doesn't end with
    // \0, then it is considered multi-part memo
    uint8_t memo[MEMO_MAIN_SIZE];
};

#define MEMO_PART_SIZE (int)fmax(sizeof(Transaction), MEMO_FULL_SIZE - MEMO_MAIN_SIZE)
#define SIGNATURES_PER_RECORD (int)(sizeof(Transaction) / sizeof(Signature))
#define RAW_MESSAGE_PART_SIZE MEMO_PART_SIZE

enum GradidoRecordType {
    BLANK=0,
    GRADIDO_TRANSACTION,
    MEMO,
    SIGNATURES,
    STRUCTURALLY_BAD_MESSAGE,
    RAW_MESSAGE
};

enum StructurallyBadMessageResult {
    // couldn't be deserialized
    UNDESERIALIZABLE=1,

    // any of needed signatures is invalid
    INVALID_SIGNATURE,

    // any of needed signatures is missing
    MISSING_SIGNATURE,

    // transaction is too large to be expressed by number of parts
    // less or equal to MAX_RECORD_PARTS; other way to get this is to
    // provide too large byte buffer in message (such as alias, memo,
    // etc.) - reason is that such message cannot be saved in blockchain
    // and later validated; this shoud not happen, transaction sender
    // has to validate data before sending
    TOO_LARGE,

    // if amount is negative
    NEGATIVE_AMOUNT,

    // memo cannot contain null character
    BAD_MEMO_CHARACTER,

    // group alias has to consist of letters, numbers, minus, underscore
    BAD_ALIAS_CHARACTER,

    // key / signature size is not correct
    KEY_SIZE_NOT_CORRECT
};

struct StructurallyBadMessage : public TransactionCommonHeader {
    uint8_t result; // enum StructurallyBadMessageResult
    uint64_t length; // total length of message in bytes; contents is
                     // provided in following parts
    HederaTransaction hedera_transaction;
};

// idea is that most transactions are supposed to be transfers; those
// will consist of single part (if memo is short enough); size of other
// types of transactions is less relevant
struct GradidoRecord {
    uint8_t record_type; // enum GradidoRecordType
    union {
        Transaction transaction;
        uint8_t memo[MEMO_PART_SIZE];
        Signature signature[SIGNATURES_PER_RECORD];
        StructurallyBadMessage structurally_bad_message;
        uint8_t raw_message[RAW_MESSAGE_PART_SIZE];
    };
};

struct GroupRecord {
    uint8_t alias[GROUP_ALIAS_LENGTH];
    HederaTopicID topic_id;
    bool success;
    HederaTransaction hedera_transaction;
    Signature signature;
};

enum GroupRegisterRecordType {
    GROUP_RECORD=0,
    GR_STRUCTURALLY_BAD_MESSAGE,
    GR_RAW_MESSAGE
};

struct GroupRegisterRecord {
    uint8_t record_type; // enum GroupRegisterRecordType
    union {
        GroupRecord group_record;
        StructurallyBadMessage structurally_bad_message;
        uint8_t raw_message[RAW_MESSAGE_PART_SIZE];
    };
};

}

#endif
