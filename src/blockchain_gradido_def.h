#ifndef BLOCKCHAIN_GRADIDO_DEF_H
#define BLOCKCHAIN_GRADIDO_DEF_H

#include <cstdint>
#include <math.h>
#include <string.h>
#include "ed25519/ed25519.h"


namespace gradido {

// change with care; blockchain validity depends on those structs
// 
// NOTE: don't add any virtual methods to those classes, as memset() is
// used to blank them; it should be possible to have them zeroed 
// semantically
//
// currently, each struct has such a constructor; if structs are nested,
// some bytes may be initialized more than one time; having a proper
// initialization is of higher importance, though
//
// those structures are used also for signing; in this case, all 
// calculated fields should be 0 (blockchain checksum won't zero them);
// this way we only have one way translation necessary (from protobuf to
// struct) and it is bound to actual version_number

#define GRADIDO_BLOCK_SIZE 1000

#define HEDERA_RUNNING_HASH_LENGTH 48

#define MAX_RECORD_PARTS 5

// not flexible; would take some more adjustments when changing this
#define SIGNATURES_IN_MAIN_PART 1
    
#define NON_SIGNATURE_PARTS 2
#define SIGNATURE_PARTS (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS)
#define MINIMUM_SIGNATURE_COUNT 1
#define MAXIMUM_SIGNATURE_COUNT (SIGNATURES_PER_RECORD * (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS) + SIGNATURES_IN_MAIN_PART)

#define PUB_KEY_LENGTH ed25519_pubkey_SIZE
#define PRIV_KEY_LENGTH ed25519_privkey_SIZE

#define SIGNATURE_LENGTH 64

#define MEMO_MAIN_SIZE 16
#define MEMO_FULL_SIZE 150

#define GROUP_REGISTER_NAME "group-register"

#define BLOCKCHAIN_CHECKSUM_SIZE SHA_512_SIZE

// group alias field ends with null characters to have exact length and
// have null-terminated string as well
#define GROUP_ALIAS_LENGTH 16

// this is GRADIDO_DECIMAL_CONVERSION_FACTOR; gradidomath.h not included
// to keep things separated
#define GRADIDO_VALUE_DECIMAL_AMOUNT_BASE 1000000

#define SB_MEMO_LENGTH 256
#define SB_SUBCLUSTER_NAME_LENGTH 64
#define SB_CA_CHAIN_PER_RECORD 1

#define SB_ADMIN_NAME_LENGTH 64
#define SB_EMAIL_LENGTH 64
#define SB_SIGNATURES_PER_RECORD 10

// current version is set to this by default and incremented as per sb
#define DEFAULT_VERSION_NUMBER 1

#if 0
// experimental; this reduces size of blocks on HDD by ~15%
#define PACKED_STRUCT __attribute__((__packed__))
#else
#define PACKED_STRUCT
#endif

// some structs (mostly those defined in this file) are ment to be
// erased by zeroing them
// TODO: verify memset is not optimized out, so it can be used for
// security reasons as well
#define ZERO_THIS volatile void* p = memset(this, 0, sizeof(*this));

// ca chain is mentioned only once per blockchain, just after header 
// record; no need to pack them tightly
#define CA_CHAIN_PER_RECORD 1

struct PACKED_STRUCT PubKeyEntity {
    uint8_t pub_key[PUB_KEY_LENGTH];
    PubKeyEntity() { ZERO_THIS; }
};

struct PACKED_STRUCT GradidoValue {
    uint64_t amount;
    uint32_t decimal_amount;
    GradidoValue() { ZERO_THIS; }
    GradidoValue operator+(const GradidoValue& v) {
        GradidoValue res;
        res.amount = amount + v.amount;
        res.decimal_amount = decimal_amount + v.decimal_amount;

        if (res.decimal_amount >= GRADIDO_VALUE_DECIMAL_AMOUNT_BASE) {
            res.decimal_amount -= GRADIDO_VALUE_DECIMAL_AMOUNT_BASE;
            res.amount++;    
        }
        return res;
    }
    GradidoValue operator-(const GradidoValue& v) {
        GradidoValue res;
        if (*this < v)
            throw std::runtime_error("GradidoValue less than zero");
        res.amount = amount - v.amount;
        if (decimal_amount >= v.decimal_amount) {
            res.decimal_amount = decimal_amount - v.decimal_amount;
        } else {
            res.decimal_amount = GRADIDO_VALUE_DECIMAL_AMOUNT_BASE -
                (v.decimal_amount - decimal_amount);
            res.amount--;
        }
        return res;
    }
    GradidoValue& operator+=(const GradidoValue& v) {
        *this = *this + v;
        return *this;
    }

    bool operator<(uint64_t v) {
        return amount < v;
    }
    bool operator<(const GradidoValue& v) {
        return amount < v.amount || (amount == v.amount && 
                                     decimal_amount < v.decimal_amount);
    }
};

struct PACKED_STRUCT User {
    uint8_t user[PUB_KEY_LENGTH];
    User() { ZERO_THIS; }
};

struct PACKED_STRUCT UserState {
    // these fields are updated only if transaction is successful
    GradidoValue new_balance;
    uint64_t prev_transfer_rec_num;
    UserState() { ZERO_THIS; }
};

#define SIGNATURE_RECORD_LENGTH PUB_KEY_LENGTH + SIGNATURE_LENGTH
// don't add more members without updating macro
struct PACKED_STRUCT Signature {
    uint8_t pubkey[PUB_KEY_LENGTH];
    uint8_t signature[SIGNATURE_LENGTH];
    Signature() { ZERO_THIS; }
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
    OUTBOUND_TRANSFER,
    
    HEADER,
    ADD_ADMIN,
    REMOVE_ADMIN,
    ADD_NODE,
    REMOVE_NODE,
    INITIALIZATION_DONE
};

struct PACKED_STRUCT HederaTimestamp {
    int64_t seconds;
    int32_t nanos;

    HederaTimestamp() { ZERO_THIS; }

    bool operator<(const HederaTimestamp& ts) const {
        return ts.seconds > seconds || (ts.seconds == seconds && 
                                        ts.nanos > nanos);
    }
    bool operator==(const HederaTimestamp& ts) const {
        return ts.seconds == seconds && ts.nanos == nanos;
    }
    HederaTimestamp operator-(const HederaTimestamp& v) {
        
        HederaTimestamp res;
        res.seconds = seconds - v.seconds;
        if (nanos >= v.nanos) {
            res.nanos = nanos - v.nanos;
        } else {
            res.nanos = 1000000000 - (v.nanos - nanos);
            res.seconds--;
        }
        return res;
    }

};

struct PACKED_STRUCT HederaAccountID {
    int64_t shardNum;
    int64_t realmNum;
    int64_t accountNum;
    HederaAccountID() { ZERO_THIS; }
};

struct PACKED_STRUCT HederaTopicID {
    int64_t shardNum;
    int64_t realmNum;
    int64_t topicNum;
    HederaTopicID() { ZERO_THIS; }
};

struct PACKED_STRUCT HederaTransactionID  {
    HederaTimestamp transactionValidStart;
    HederaAccountID accountID;
    HederaTransactionID() { ZERO_THIS; }
};

struct PACKED_STRUCT HederaTransaction {
    HederaTimestamp consensusTimestamp;
    uint8_t runningHash[HEDERA_RUNNING_HASH_LENGTH];
    uint64_t sequenceNumber;

    // TODO: remove, along with renaming Hedera
    uint64_t runningHashVersion;
    HederaTransaction() { ZERO_THIS; }
};

struct PACKED_STRUCT AbstractTransferOp {
    GradidoValue amount;
    AbstractTransferOp() { ZERO_THIS; }
};

struct PACKED_STRUCT UserTransfer : public User, UserState {};

struct PACKED_STRUCT LocalTransfer : public AbstractTransferOp {
    UserTransfer sender;
    UserTransfer receiver;
    LocalTransfer() { ZERO_THIS; }
};

struct PACKED_STRUCT PairedTransaction {
    HederaTimestamp paired_transaction_id;
    uint8_t other_group[GROUP_ALIAS_LENGTH];
    PairedTransaction() { ZERO_THIS; }
};

struct PACKED_STRUCT InboundTransfer : public AbstractTransferOp, 
    PairedTransaction {
    User sender;
    UserTransfer receiver;
    InboundTransfer() { ZERO_THIS; }
};
struct PACKED_STRUCT OutboundTransfer : public AbstractTransferOp, 
    PairedTransaction {
    UserTransfer sender;
    User receiver;
    OutboundTransfer() { ZERO_THIS; }
};

struct PACKED_STRUCT FriendUpdate {
    uint8_t group[GROUP_ALIAS_LENGTH];
    FriendUpdate() { ZERO_THIS; }
};

// first transaction is ADD_USER with user_id of group's creator
struct PACKED_STRUCT AddUser : public User {}; 
struct PACKED_STRUCT MoveUser : public User, PairedTransaction, UserState {};

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

struct PACKED_STRUCT GradidoCreation : public UserTransfer, AbstractTransferOp {};

struct PACKED_STRUCT TransactionCommonHeader {
    // starts with 1; it may affect the way how transaction is parsed and
    // validated; version_number is the same for all parts of single 
    // transaction; for structurally_bad_message it may be 0, if 
    // version number cannot be obtained from data
    uint32_t version_number;

    // other parts may follow this record; if not, then parts == 1;
    // cannot be larger than MAX_RECORD_PARTS for structurally validated
    // transaction (one which is therefore translated); for 
    // structurally_bad_message it is unlimited
    uint8_t parts;

    HederaTransaction hedera_transaction;
    TransactionCommonHeader() { ZERO_THIS; }
};

struct PACKED_STRUCT GradidoHeader {
    uint8_t alias[GROUP_ALIAS_LENGTH];
    uint8_t chain_length;
    uint8_t ordering_node_pub_key[PUB_KEY_LENGTH];
    GradidoHeader() { ZERO_THIS; }
};

struct PACKED_STRUCT Transaction : public TransactionCommonHeader {

    // additional signatures can be stored in more parts
    Signature signature;
    uint8_t signature_count;

    uint8_t transaction_type; // enum TransactionType
    union PACKED_STRUCT {
        GradidoCreation gradido_creation;
        FriendUpdate add_group_friend;
        FriendUpdate remove_group_friend;
        AddUser add_user;
        MoveUser move_user_inbound;
        MoveUser move_user_outbound;
        LocalTransfer local_transfer;
        InboundTransfer inbound_transfer;
        OutboundTransfer outbound_transfer;

        GradidoHeader header;
        PubKeyEntity add_admin;
        PubKeyEntity remove_admin;
        PubKeyEntity add_node;
        PubKeyEntity remove_node;
        PubKeyEntity ca_chain[CA_CHAIN_PER_RECORD];
    };

    uint8_t result; // enum TransactionResult
    

    // first part of memo, to avoid multi-parts, if possible (assuming,
    // usually memos are small); has to be null-terminated string;
    // memo is first of possible parts; if this->memo doesn't end with
    // \0, then it is considered multi-part memo
    uint8_t memo[MEMO_MAIN_SIZE];

    Transaction() { ZERO_THIS; }
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

struct PACKED_STRUCT StructurallyBadMessage : 
    public TransactionCommonHeader {
    uint8_t result; // enum StructurallyBadMessageResult
    uint64_t length; // total length of message in bytes; contents is
                     // provided in following parts
    HederaTransaction hedera_transaction;
    StructurallyBadMessage() { ZERO_THIS; }
};

// idea is that most transactions are supposed to be transfers; those
// will consist of single part (if memo is short enough); size of other
// types of transactions is less relevant
struct PACKED_STRUCT GradidoRecord {
    uint8_t record_type; // enum GradidoRecordType
    union PACKED_STRUCT {
        Transaction transaction;
        uint8_t memo[MEMO_PART_SIZE];
        Signature signature[SIGNATURES_PER_RECORD];
        StructurallyBadMessage structurally_bad_message;
        uint8_t raw_message[RAW_MESSAGE_PART_SIZE];
    };
    GradidoRecord() { ZERO_THIS; }
};

struct PACKED_STRUCT GroupRecord {
    uint8_t alias[GROUP_ALIAS_LENGTH];
    HederaTopicID topic_id;
    bool success;
    HederaTransaction hedera_transaction;
    Signature signature;
    GroupRecord() { ZERO_THIS; }
};

enum PACKED_STRUCT GroupRegisterRecordType {
    // TODO: 1
    GROUP_RECORD=0,
    GR_STRUCTURALLY_BAD_MESSAGE,
    GR_RAW_MESSAGE
};

struct PACKED_STRUCT GroupRegisterRecord {
    uint8_t record_type; // enum GroupRegisterRecordType
    union PACKED_STRUCT {
        GroupRecord group_record;
        StructurallyBadMessage structurally_bad_message;
        uint8_t raw_message[RAW_MESSAGE_PART_SIZE];
    };
    GroupRegisterRecord() { ZERO_THIS; }
};

enum class SbRecordType {
    SB_HEADER=1,
    SB_ADD_ADMIN,
    SB_ADD_NODE,
    ENABLE_ADMIN,
    DISABLE_ADMIN,
    ENABLE_NODE,
    DISABLE_NODE,
    ADD_BLOCKCHAIN,
    ADD_NODE_TO_BLOCKCHAIN,
    REMOVE_NODE_FROM_BLOCKCHAIN,
    SIGNATURES
};

enum class SbNodeType {
    ORDERING=1,
    GRADIDO,
    LOGIN,
    BACKUP,
    LOGGER,
    PINGER
};

enum class SbInitialKeyType {
    SELF_SIGNED=1,
    CA_SIGNED
};

// TODO: include version number to allow verification
struct PACKED_STRUCT SbHeader {
    uint8_t subcluster_name[SB_SUBCLUSTER_NAME_LENGTH];
    uint8_t initial_key_type; // SbInitialKeyType
    uint8_t pub_key[PUB_KEY_LENGTH];
    uint8_t ca_pub_key_chain_length; // including pub_key
    SbHeader() { ZERO_THIS; }
};

struct PACKED_STRUCT SbAdmin {
    uint8_t pub_key[PUB_KEY_LENGTH];
    uint8_t name[SB_ADMIN_NAME_LENGTH];
    uint8_t email[SB_EMAIL_LENGTH];
    SbAdmin() { ZERO_THIS; }
};

struct PACKED_STRUCT SbNode {
    uint8_t node_type; // SbNodeType 
    uint8_t pub_key[PUB_KEY_LENGTH];
    SbNode() { ZERO_THIS; }
};

struct PACKED_STRUCT SbNodeBlockchain {
    uint8_t node_pub_key[PUB_KEY_LENGTH];
    uint8_t blockchain_pub_key[PUB_KEY_LENGTH];
    uint8_t alias[GROUP_ALIAS_LENGTH];
    SbNodeBlockchain() { ZERO_THIS; }
};

enum class SbTransactionResult {
    SUCCESS = 0,
    DUPLICATE_PUB_KEY,
    BAD_ADMIN_SIGNATURE,
    NON_MAJORITY,
    DUPLICATE_ALIAS,
    NODE_ALREADY_ADDED_TO_BLOCKCHAIN
};

struct PACKED_STRUCT SbRecord {
    uint32_t version_number;
    HederaTransaction hedera_transaction;
    uint8_t record_type; // enum SbRecordType
    uint8_t memo[SB_MEMO_LENGTH];
    uint8_t signature_count;
    uint8_t result; // enum SbTransactionResult
    union PACKED_STRUCT {
        SbHeader header;
        PubKeyEntity ca_chain[SB_CA_CHAIN_PER_RECORD];
        SbAdmin admin;
        PubKeyEntity disable_admin;
        PubKeyEntity enable_admin;
        SbNode add_node;
        PubKeyEntity disable_node;
        PubKeyEntity enable_node;
        PubKeyEntity add_blockchain;
        SbNodeBlockchain add_node_to_blockchain;
        SbNodeBlockchain remove_node_from_blockchain;
        // for this field version_number, hedera_transaction, memo, 
        // result, signature_count are expected to be 0 (version_number 
        // is inherited from main record)
        Signature signatures[SB_SIGNATURES_PER_RECORD];
    };
    SbRecord() { ZERO_THIS; }
};


}

#endif
