#ifndef BLOCKCHAIN_GRADIDO_DEF_H
#define BLOCKCHAIN_GRADIDO_DEF_H

#include <cstdint>
#include <math.h>


namespace gradido {

// change with care; blockchain validity depends on those structs

// all structures has to be set to 0 before putting any data in them

#define GRADIDO_BLOCK_SIZE 1000

#define HEDERA_RUNNING_HASH_LENGTH 48

#define MAX_RECORD_PARTS 5

#define PUB_KEY_LENGTH 32
#define SIGNATURE_LENGTH 32

#define MEMO_MAIN_SIZE 16
#define MEMO_FULL_SIZE 150

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

    // any of needed signatures is invalid
    INVALID_SIGNATURE,

    // any of needed signatures is missing
    MISSING_SIGNATURE,
    
    // user has to be in local group
    UNKNOWN_LOCAL_USER,

    // user has to be in remote group
    UNKNOWN_REMOTE_USER,

    // group has to be in list of friends
    UNKNOWN_GROUP,

    // transaction is too large to be expressed by number of parts
    // less or equal to MAX_RECORD_PARTS
    TOO_LARGE,

    // not possible to finish inbound transaction, as outbound 
    // transaction failed
    OUTBOUND_TRANSACTION_FAILED,

    // for add_user and move_user_inbound
    USER_ALREADY_EXISTS,

    // for add/remove friend group
    GROUP_IS_ALREADY_FRIEND,
    GROUP_IS_NOT_FRIEND
};

struct GradidoCreation : public UserTransfer, AbstractTransferOp {};

struct Transaction {
    // starts with 1; it may affect the way how transaction is parsed and
    // validated; version_number is the same for all parts of single 
    // transaction
    uint8_t version_number;

    // additional signatures can be stored in more parts
    Signature signature;
    uint8_t signature_count;

    HederaTransaction hedera_transaction;
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
    
    // other parts may follow this record; if not, then parts == 1;
    // cannot be larger than MAX_RECORD_PARTS
    uint8_t parts;

    // first part of memo, to avoid multi-parts, if possible (assuming,
    // usually memos are small); has to be null-terminated string;
    // memo is first of possible parts; if this->memo doesn't end with
    // \0, then it is considered multi-part memo
    uint8_t memo[MEMO_MAIN_SIZE];
};

#define MEMO_PART_SIZE (int)fmax(sizeof(Transaction), MEMO_FULL_SIZE - MEMO_MAIN_SIZE)
#define SIGNATURES_PER_RECORD (int)(sizeof(Transaction) / sizeof(Signature))

enum GradidoRecordType {
    BLANK=0,
    GRADIDO_TRANSACTION,
    MEMO,
    SIGNATURES
};

struct GradidoRecord {
    uint8_t record_type; // enum GradidoRecordType
    union {
        Transaction transaction;
        uint8_t memo[MEMO_PART_SIZE];
        Signature signature[SIGNATURES_PER_RECORD];
    };
};

struct GroupRecord {
    uint8_t alias[GROUP_ALIAS_LENGTH];
    HederaTransaction hedera_transaction;
    Signature signature;
};

}

#endif
