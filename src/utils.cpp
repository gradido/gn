#include "utils.h"
#include <time.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <Poco/File.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Poco/Random.h>
#include <ed25519/ed25519/ed25519.h>

namespace gradido {


std::string get_as_str(TransactionType r) {
    switch (r) {
    case TransactionType::GRADIDO_CREATION:
        return "GRADIDO_CREATION";
    case TransactionType::ADD_GROUP_FRIEND:
        return "ADD_GROUP_FRIEND";
    case TransactionType::REMOVE_GROUP_FRIEND:
        return "REMOVE_GROUP_FRIEND";
    case TransactionType::ADD_USER:
        return "ADD_USER";
    case TransactionType::MOVE_USER_INBOUND:
        return "MOVE_USER_INBOUND";
    case TransactionType::MOVE_USER_OUTBOUND:
        return "MOVE_USER_OUTBOUND";
    case TransactionType::LOCAL_TRANSFER:
        return "LOCAL_TRANSFER";
    case TransactionType::INBOUND_TRANSFER:
        return "INBOUND_TRANSFER";
    case TransactionType::OUTBOUND_TRANSFER:
        return "OUTBOUND_TRANSFER";
    }
}

std::string get_as_str(TransactionResult r) {
    switch (r) {
    case TransactionResult::SUCCESS:
        return "SUCCESS";
    case TransactionResult::UNKNOWN:
        return "UNKNOWN";
    case TransactionResult::NOT_ENOUGH_GRADIDOS:
        return "NOT_ENOUGH_GRADIDOS";
    case TransactionResult::UNKNOWN_LOCAL_USER:
        return "UNKNOWN_LOCAL_USER";
    case TransactionResult::UNKNOWN_REMOTE_USER:
        return "UNKNOWN_REMOTE_USER";
    case TransactionResult::UNKNOWN_GROUP:
        return "UNKNOWN_GROUP";
    case TransactionResult::OUTBOUND_TRANSACTION_FAILED:
        return "OUTBOUND_TRANSACTION_FAILED";
    case TransactionResult::USER_ALREADY_EXISTS:
        return "USER_ALREADY_EXISTS";
    case TransactionResult::GROUP_IS_ALREADY_FRIEND:
        return "GROUP_IS_ALREADY_FRIEND";
    case TransactionResult::GROUP_IS_NOT_FRIEND:
        return "GROUP_IS_NOT_FRIEND";
    }
}

std::string get_as_str(StructurallyBadMessageResult r) {
    switch (r) {
    case StructurallyBadMessageResult::UNDESERIALIZABLE:
        return "UNDESERIALIZABLE";
    case StructurallyBadMessageResult::INVALID_SIGNATURE:
        return "INVALID_SIGNATURE";
    case StructurallyBadMessageResult::MISSING_SIGNATURE:
        return "MISSING_SIGNATURE";
    case StructurallyBadMessageResult::TOO_LARGE:
        return "TOO_LARGE";
    case StructurallyBadMessageResult::NEGATIVE_AMOUNT:
        return "NEGATIVE_AMOUNT";
    case StructurallyBadMessageResult::BAD_MEMO_CHARACTER:
        return "BAD_MEMO_CHARACTER";
    case StructurallyBadMessageResult::BAD_ALIAS_CHARACTER:
        return "BAD_ALIAS_CHARACTER";
    case StructurallyBadMessageResult::KEY_SIZE_NOT_CORRECT:
        return "KEY_SIZE_NOT_CORRECT";
    }
}

std::string get_as_str(GradidoRecordType r) {
    switch (r) {
    case GradidoRecordType::BLANK:
        return "BLANK";
    case GradidoRecordType::GRADIDO_TRANSACTION:
        return "GRADIDO_TRANSACTION";
    case GradidoRecordType::MEMO:
        return "MEMO";
    case GradidoRecordType::SIGNATURES:
        return "SIGNATURES";
    case GradidoRecordType::STRUCTURALLY_BAD_MESSAGE:
        return "STRUCTURALLY_BAD_MESSAGE";
    case GradidoRecordType::RAW_MESSAGE:
        return "RAW_MESSAGE";
    }
}

std::string get_as_str(GroupRegisterRecordType r) {
    switch (r) {
    case GroupRegisterRecordType::GROUP_RECORD:
        return "GROUP_RECORD";
    case GroupRegisterRecordType::GR_STRUCTURALLY_BAD_MESSAGE:
        return "GR_STRUCTURALLY_BAD_MESSAGE";
    case GroupRegisterRecordType::GR_RAW_MESSAGE:
        return "GR_RAW_MESSAGE";
    }
}

std::string get_as_str(SbRecordType r) {
    switch (r) {
    case SbRecordType::SB_HEADER:
        return "SB_HEADER";
    case SbRecordType::SB_ADD_ADMIN:
        return "SB_ADD_ADMIN";
    case SbRecordType::SB_ADD_NODE:
        return "SB_ADD_NODE";
    case SbRecordType::ENABLE_ADMIN:
        return "ENABLE_ADMIN";
    case SbRecordType::DISABLE_ADMIN:
        return "DISABLE_ADMIN";
    case SbRecordType::ENABLE_NODE:
        return "ENABLE_NODE";
    case SbRecordType::DISABLE_NODE:
        return "DISABLE_NODE";
    case SbRecordType::ADD_BLOCKCHAIN:
        return "ADD_BLOCKCHAIN";
    case SbRecordType::ADD_NODE_TO_BLOCKCHAIN:
        return "ADD_NODE_TO_BLOCKCHAIN";
    case SbRecordType::REMOVE_NODE_FROM_BLOCKCHAIN:
        return "REMOVE_NODE_FROM_BLOCKCHAIN";
    case SbRecordType::SIGNATURES:
        return "SIGNATURES";
    }
}

std::string get_as_str(SbTransactionResult r) {
    switch (r) {
    case SbTransactionResult::SUCCESS:
        return "SUCCESS";
    case SbTransactionResult::DUPLICATE_PUB_KEY:
        return "DUPLICATE_PUB_KEY";
    case SbTransactionResult::BAD_ADMIN_SIGNATURE:
        return "BAD_ADMIN_SIGNATURE";
    case SbTransactionResult::NON_MAJORITY:
        return "NON_MAJORITY";
    case SbTransactionResult::DUPLICATE_ALIAS:
        return "DUPLICATE_ALIAS";
    case SbTransactionResult::NODE_ALREADY_ADDED_TO_BLOCKCHAIN:
        return "NODE_ALREADY_ADDED_TO_BLOCKCHAIN";
    }
}
std::string get_as_str(SbNodeType r) {
    switch (r) {
    case SbNodeType::ORDERING:
        return "ORDERING";
    case SbNodeType::GRADIDO:
        return "GRADIDO";
    case SbNodeType::LOGIN:
        return "LOGIN";
    case SbNodeType::BACKUP:
        return "BACKUP";
    case SbNodeType::LOGGER:
        return "LOGGER";
    case SbNodeType::PINGER:
        return "PINGER";
    }
}
std::string get_as_str(SbInitialKeyType r) {
    switch (r) {
    case SbInitialKeyType::SELF_SIGNED:
        return "SELF_SIGNED";
    case SbInitialKeyType::CA_SIGNED:
        return "CA_SIGNED";
    }
}


// TODO: probably should refactor those functions, code is repeating;
// on the other hand, it is very clear what comes out from them
void dump_transaction_in_json(const GradidoRecord& t, std::ostream& out) {

    std::string record_type = get_as_str((GradidoRecordType)t.record_type);
    out << "{" << std::endl
        << "  \"record_type\": \"" << record_type << "\", " << std::endl;

    switch ((GradidoRecordType)t.record_type) {
    case GradidoRecordType::GRADIDO_TRANSACTION: {
        const Transaction& u = t.transaction;

        char buff[1024];
        dump_in_hex((char*)u.hedera_transaction.runningHash, buff, 48);
        std::string running_hash(buff);

        dump_in_hex((char*)u.signature.pubkey, buff, PUB_KEY_LENGTH);
        std::string sig_pubkey(buff);

        dump_in_hex((char*)u.signature.signature, buff, SIGNATURE_LENGTH);
        std::string sig_sig(buff);

        std::string memo = u.memo[MEMO_MAIN_SIZE - 1] == 0 ? 
            std::string((char*)u.memo) : std::string((char*)u.memo, MEMO_MAIN_SIZE);

        out << "  \"transaction\": {" << std::endl;
        out << "    \"version_number\": " << (int)u.version_number << ", " << std::endl;
        out << "    \"signature\": {" << std::endl;
        out << "      \"pubkey\": \"" << sig_pubkey << "\", " << std::endl;
        out << "      \"signature\": \"" << sig_sig << "\"" << std::endl;
        out << "    }, " << std::endl;
        out << "    \"signature_count\": " << (int)u.signature_count << ", " << std::endl;
        out << "    \"hedera_transaction\": {" << std::endl
            << "      \"consensusTimestamp\": {" << std::endl
            << "        \"seconds\": " << u.hedera_transaction.consensusTimestamp.seconds << ", " << std::endl
            << "        \"nanos\": " << u.hedera_transaction.consensusTimestamp.nanos  << std::endl
            << "      }," << std::endl
            << "      \"runningHash\": \"" << running_hash << "\", " << std::endl
            << "      \"sequenceNumber\": " << u.hedera_transaction.sequenceNumber << ", " << std::endl
            << "      \"runningHashVersion\": " << u.hedera_transaction.runningHashVersion << std::endl;
        out << "    }," << std::endl;

        std::string ttype = get_as_str((TransactionType)u.transaction_type);

        out << "    \"transaction_type\": \"" << ttype << "\", " << std::endl;

        switch ((TransactionType)u.transaction_type) {
        case TransactionType::GRADIDO_CREATION: {
            const GradidoCreation& v = u.gradido_creation;
            dump_in_hex((char*)v.user, buff, PUB_KEY_LENGTH);
            std::string user(buff);
            out << "    \"gradido_creation\": {" << std::endl;
            out << "      \"user\": \"" << user << "\", " << std::endl;
            out << "      \"new_balance\": {" << std::endl;
            out << "        \"amount\": " << v.new_balance.amount << ", " << std::endl;
            out << "        \"decimal_amount\": " << v.new_balance.decimal_amount << std::endl;
            out << "      }," << std::endl;
            out << "      \"prev_transfer_rec_num\": " << v.prev_transfer_rec_num << ", " << std::endl;
            out << "      \"amount\": {" << std::endl;
            out << "        \"amount\": " << v.amount.amount << ", " << std::endl;
            out << "        \"decimal_amount\": " << v.amount.decimal_amount << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::ADD_GROUP_FRIEND: {
            std::string group((char*)u.add_group_friend.group);
            out << "    \"add_group_friend\": {" << std::endl;
            out << "      \"group\": \"" << group << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::REMOVE_GROUP_FRIEND: {
            std::string group((char*)u.add_group_friend.group);
            out << "    \"remove_group_friend\": {" << std::endl;
            out << "      \"group\": \"" << group << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::ADD_USER: {
            dump_in_hex((char*)u.add_user.user, buff, PUB_KEY_LENGTH);
            std::string user(buff);
            out << "    \"add_user\": {" << std::endl;
            out << "      \"user\": \"" << user << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::MOVE_USER_INBOUND: {
            dump_in_hex((char*)u.move_user_inbound.user, buff, 
                        PUB_KEY_LENGTH);
            std::string user(buff);
            std::string other_group((char*)u.move_user_inbound.other_group);
            HederaTimestamp ts = u.move_user_inbound.paired_transaction_id;
            out << "    \"move_user_inbound\": {" << std::endl;
            out << "      \"user\": \"" << user << "\"," << std::endl;
            out << "      \"other_group\": \"" << other_group << "\"," << std::endl;
            out << "      \"paired_transaction_id\": {" << std::endl;
            out << "        \"seconds\": " << ts.seconds  << ", " << std::endl;
            out << "        \"nanos\": " << ts.nanos << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::MOVE_USER_OUTBOUND: {
            dump_in_hex((char*)u.move_user_outbound.user, buff, 
                        PUB_KEY_LENGTH);
            std::string user(buff);
            std::string other_group((char*)u.move_user_outbound.other_group);
            HederaTimestamp ts = u.move_user_outbound.paired_transaction_id;
            out << "    \"move_user_outbound\": {" << std::endl;
            out << "      \"user\": \"" << user << "\"," << std::endl;
            out << "      \"other_group\": \"" << other_group << "\"," << std::endl;
            out << "      \"paired_transaction_id\": {" << std::endl;
            out << "        \"seconds\": " << ts.seconds  << ", " << std::endl;
            out << "        \"nanos\": " << ts.nanos << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::LOCAL_TRANSFER: {
            const LocalTransfer& tt = u.local_transfer;
            dump_in_hex((char*)tt.sender.user, buff, 
                        PUB_KEY_LENGTH);
            std::string sender(buff);
            dump_in_hex((char*)tt.receiver.user, buff, 
                        PUB_KEY_LENGTH);
            std::string receiver(buff);

            out << "    \"local_transfer\": {" << std::endl;
            out << "      \"sender\": {" << std::endl;
            out << "        \"user\": \"" << sender << "\", " << std::endl;
            out << "        \"new_balance\": {" << std::endl;
            out << "          \"amount\": " << tt.sender.new_balance.amount << ", " << std::endl;
            out << "          \"decimal_amount\": " << tt.sender.new_balance.decimal_amount << std::endl;
            out << "        }," << std::endl;

            out << "        \"prev_transfer_rec_num\": " << tt.sender.prev_transfer_rec_num << std::endl;
            out << "      }," << std::endl;
            out << "      \"receiver\": {" << std::endl;
            out << "        \"user\": \"" << receiver << "\", " << std::endl;
            out << "        \"new_balance\": {" << std::endl;
            out << "          \"amount\": " << tt.receiver.new_balance.amount << ", " << std::endl;
            out << "          \"decimal_amount\": " << tt.receiver.new_balance.decimal_amount << std::endl;
            out << "        }," << std::endl;

            out << "        \"prev_transfer_rec_num\": " << tt.receiver.prev_transfer_rec_num << std::endl;
            out << "      }," << std::endl;
            out << "      \"amount\": {" << std::endl;
            out << "        \"amount\": " << tt.amount.amount << ", " << std::endl;
            out << "        \"decimal_amount\": " << tt.amount.decimal_amount << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::INBOUND_TRANSFER: {
            const InboundTransfer& tt = u.inbound_transfer;
            dump_in_hex((char*)tt.sender.user, buff, 
                        PUB_KEY_LENGTH);
            std::string sender(buff);
            dump_in_hex((char*)tt.receiver.user, buff, 
                        PUB_KEY_LENGTH);
            std::string receiver(buff);

            std::string other_group((char*)tt.other_group);
            HederaTimestamp ts = tt.paired_transaction_id;

            out << "    \"inbound_transfer\": {" << std::endl;
            out << "      \"sender\": {" << std::endl;
            out << "        \"user\": \"" << sender << "\"" << std::endl;
            out << "      }," << std::endl;
            out << "      \"receiver\": {" << std::endl;
            out << "        \"user\": \"" << receiver << "\", " << std::endl;
            out << "        \"new_balance\": {" << std::endl;
            out << "          \"amount\": " << tt.receiver.new_balance.amount << ", " << std::endl;
            out << "          \"decimal_amount\": " << tt.receiver.new_balance.decimal_amount << std::endl;
            out << "        }," << std::endl;

            out << "        \"prev_transfer_rec_num\": " << tt.receiver.prev_transfer_rec_num << std::endl;
            out << "      }," << std::endl;
            out << "      \"amount\": {" << std::endl;
            out << "        \"amount\": " << tt.amount.amount << ", " << std::endl;
            out << "        \"decimal_amount\": " << tt.amount.decimal_amount << std::endl;
            out << "      }," << std::endl;
            out << "      \"other_group\": \"" << other_group << "\"," << std::endl;
            out << "      \"paired_transaction_id\": {" << std::endl;
            out << "        \"seconds\": " << ts.seconds  << ", " << std::endl;
            out << "        \"nanos\": " << ts.nanos << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case TransactionType::OUTBOUND_TRANSFER: {
            const OutboundTransfer& tt = u.outbound_transfer;
            dump_in_hex((char*)tt.sender.user, buff, 
                        PUB_KEY_LENGTH);
            std::string sender(buff);
            dump_in_hex((char*)tt.receiver.user, buff, 
                        PUB_KEY_LENGTH);
            std::string receiver(buff);
            std::string other_group((char*)tt.other_group);
            HederaTimestamp ts = tt.paired_transaction_id;

            out << "    \"outbound_transfer\": {" << std::endl;
            out << "      \"sender\": {" << std::endl;
            out << "        \"user\": \"" << sender << "\", " << std::endl;
            out << "        \"new_balance\": {" << std::endl;
            out << "          \"amount\": " << tt.sender.new_balance.amount << ", " << std::endl;
            out << "          \"decimal_amount\": " << tt.sender.new_balance.decimal_amount << std::endl;
            out << "        }," << std::endl;
            out << "        \"prev_transfer_rec_num\": " << tt.sender.prev_transfer_rec_num << std::endl;
            out << "      }," << std::endl;
            out << "      \"receiver\": {" << std::endl;
            out << "        \"user\": \"" << receiver << "\"" << std::endl;
            out << "      }," << std::endl;
            out << "      \"amount\": {" << std::endl;
            out << "        \"amount\": " << tt.amount.amount << ", " << std::endl;
            out << "        \"decimal_amount\": " << tt.amount.decimal_amount << std::endl;
            out << "      }," << std::endl;
            out << "      \"other_group\": \"" << other_group << "\"," << std::endl;
            out << "      \"paired_transaction_id\": {" << std::endl;
            out << "        \"seconds\": " << ts.seconds  << ", " << std::endl;
            out << "        \"nanos\": " << ts.nanos << std::endl;
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        }

        std::string result_str = get_as_str((TransactionResult)u.result);

        out << "    \"result\": \"" << result_str << "\", " << std::endl;
        out << "    \"parts\": " << (int)u.parts << ", " << std::endl;
        out << "    \"memo\": \"" << memo << "\"" << std::endl;        
        out << "  }" << std::endl;
        break;
    }

    case GradidoRecordType::MEMO: {
        out << "  \"memo\": \"" << std::string((char*)t.memo) << "\" " << std::endl;
        break;
    }
    case GradidoRecordType::SIGNATURES: {
        for (int i = 0; i < SIGNATURES_PER_RECORD; i++) {
            const Signature* s = t.signature + i;
            bool zeroes = true;
            for (int i = 0; i < PUB_KEY_LENGTH; i++)
                if (s->pubkey[0] != 0) {
                    zeroes = false;
                    break;
                }
            if (zeroes)
                break;
            char buff[1024];
            dump_in_hex((char*)s->pubkey, buff, PUB_KEY_LENGTH);
            std::string pubkey(buff);
            dump_in_hex((char*)s->signature, buff, SIGNATURE_LENGTH);
            std::string signature(buff);
            out << "  \"signature\": [" << std::endl;
            out << "    {" << std::endl;
            out << "      \"pubkey\": \"" << pubkey << "\", " << std::endl;
            out << "      \"signature\": \"" << signature << "\"" << std::endl;
            out << "    }" << std::endl;
            out << "  ]" << std::endl;
        }
        break;
    }
    case GradidoRecordType::STRUCTURALLY_BAD_MESSAGE: {
        // TODO: refactor
        const StructurallyBadMessage& u = t.structurally_bad_message;
        char buff[1024];
        dump_in_hex((char*)u.hedera_transaction.runningHash, buff, 48);
        std::string running_hash(buff);
        std::string result_str = get_as_str((TransactionResult)u.result);
        out << "    \"result\": \"" << result_str << "\", " << std::endl;
        out << "    \"parts\": " << (int)u.parts << ", " << std::endl;
        out << "    \"length\": " << u.length << ", " << std::endl;
        out << "    \"hedera_transaction\": {" << std::endl
            << "      \"consensusTimestamp\": {" << std::endl
            << "        \"seconds\": " << u.hedera_transaction.consensusTimestamp.seconds << ", " << std::endl
            << "        \"nanos\": " << u.hedera_transaction.consensusTimestamp.nanos  << std::endl
            << "      }," << std::endl
            << "      \"runningHash\": \"" << running_hash << "\", " << std::endl
            << "      \"sequenceNumber\": " << u.hedera_transaction.sequenceNumber << ", " << std::endl
            << "      \"runningHashVersion\": " << u.hedera_transaction.runningHashVersion << std::endl;
        out << "    }" << std::endl;
        
        break;
    }
    }
    out << "}" << std::endl;
}

bool ends_with(std::string const & value, std::string const & ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

    
void dump_transaction_in_json(const GroupRegisterRecord& t, std::ostream& out) {
    const GroupRecord& u = t.group_record;
    char buff[1024];

    out << "{" << std::endl;

    // TODO: raw message + structurally bad message
    out << "    \"record_type\": \"" << get_as_str((GroupRegisterRecordType)t.record_type) << "\","<< std::endl;

    std::string alias((char*)u.alias);
    out << "    \"alias\": \"" << alias << "\","<< std::endl;
    out << "    \"topic_id\": {" << std::endl;
    {
        out << "        \"shardNum\": " << u.topic_id.shardNum << "," << std::endl;
        out << "        \"realmNum\": " << u.topic_id.realmNum << "," << std::endl;
        out << "        \"topicNum\": " << u.topic_id.topicNum << std::endl;
    }
    out << "    }," << std::endl;
    out << "    \"success\": " << u.success << "," << std::endl;

    out << "    \"hedera_transaction\": {" << std::endl;
    {
        char buff[1024];
        dump_in_hex((char*)u.hedera_transaction.runningHash, buff, 48);
        std::string running_hash(buff);
        out << "      \"consensusTimestamp\": {" << std::endl
            << "        \"seconds\": " << u.hedera_transaction.consensusTimestamp.seconds << ", " << std::endl
            << "        \"nanos\": " << u.hedera_transaction.consensusTimestamp.nanos  << std::endl
            << "      }," << std::endl
            << "      \"runningHash\": \"" << running_hash << "\", " << std::endl
            << "      \"sequenceNumber\": " << u.hedera_transaction.sequenceNumber << ", " << std::endl
            << "      \"runningHashVersion\": " << u.hedera_transaction.runningHashVersion << std::endl;
    }
    out << "    }," << std::endl;

    dump_in_hex((char*)u.signature.pubkey, buff, PUB_KEY_LENGTH);
    std::string sig_pubkey(buff);

    dump_in_hex((char*)u.signature.signature, buff, SIGNATURE_LENGTH);
    std::string sig_sig(buff);

    out << "    \"signature\": {" << std::endl;
    out << "      \"pubkey\": \"" << sig_pubkey << "\", " << std::endl;
    out << "      \"signature\": \"" << sig_sig << "\"" << std::endl;
    out << "    } " << std::endl;

    out << "}" << std::endl;
    
}

void dump_transaction_in_json(const SbRecord& t, std::ostream& out) {
    out << "{" << std::endl;
    char buff[1024];

    dump_in_hex((char*)t.hedera_transaction.runningHash, buff, 48);
    std::string running_hash(buff);
    std::string memo = std::string((char*)t.memo);
    std::string record_type = get_as_str((SbRecordType)t.record_type);
    std::string result_str = get_as_str((SbTransactionResult)t.result);


    out << "  \"version_number\": " << (int)t.version_number << ", " << std::endl;
    out << "  \"hedera_transaction\": {" << std::endl
        << "    \"consensusTimestamp\": {" << std::endl
        << "      \"seconds\": " << t.hedera_transaction.consensusTimestamp.seconds << ", " << std::endl
        << "      \"nanos\": " << t.hedera_transaction.consensusTimestamp.nanos  << std::endl
        << "    }," << std::endl
        << "    \"runningHash\": \"" << running_hash << "\", " << std::endl
        << "    \"sequenceNumber\": " << t.hedera_transaction.sequenceNumber << ", " << std::endl
        << "    \"runningHashVersion\": " << t.hedera_transaction.runningHashVersion << std::endl;
    out << "  }," << std::endl;

    out << "  \"record_type\": \"" << record_type << "\", " << std::endl;
    out << "  \"memo\": \"" << memo << "\", " << std::endl;        
    out << "  \"signature_count\": " << (int)t.signature_count << ", " << std::endl;
    out << "  \"result\": \"" << result_str << "\", " << std::endl;


    switch ((SbRecordType)t.record_type) {
    case SbRecordType::SB_HEADER: {
        dump_in_hex((char*)t.header.pub_key, buff, 
                    PUB_KEY_LENGTH);

        out << "  \"header\": {" << std::endl;
        out << "    \"subcluster_name\": \"" << std::string((char*)t.header.subcluster_name) << "\", "
            << std::endl;
        out << "    \"initial_key_type\": \"" <<  
            get_as_str((SbInitialKeyType)t.header.initial_key_type)
            << "\", " << std::endl;
        out << "    \"pub_key\": \"" << std::string(buff) << "\", "
            << std::endl;
        out << "    \"ca_pub_key_chain_length\": " << 
            t.header.ca_pub_key_chain_length << std::endl;
        out << "  }" << std::endl;

        break;
    }
    case SbRecordType::SB_ADD_ADMIN: {
        out << "  \"admin\": {" << std::endl;
        dump_in_hex((char*)t.admin.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "    \"pub_key\": \"" << std::string(buff) << "\", "
            << std::endl;
        out << "    \"name\": \"" << std::string((char*)t.admin.name)
            << "\", " << std::endl;
        out << "    \"email\": \"" << std::string((char*)t.admin.email)
            << "\"" << std::endl;
        out << "  }" << std::endl;
        break;
    }
    case SbRecordType::SB_ADD_NODE: {
        dump_in_hex((char*)t.add_node.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"add_node\": {" << std::endl;
        out << "    \"node_type\": \"" << 
            get_as_str((SbNodeType)t.add_node.node_type) << "\", "
            << std::endl;
        out << "    \"pub_key\": \"" << std::string(buff) << "\""
            << std::endl;
        out << "  }" << std::endl;
        break;
    }
    case SbRecordType::ENABLE_ADMIN: {
        dump_in_hex((char*)t.enable_admin.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"enable_admin\": \"" << std::string(buff) << "\""
            << std::endl;
        break;
    }
    case SbRecordType::DISABLE_ADMIN: {
        dump_in_hex((char*)t.disable_admin.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"disable_admin\": \"" << std::string(buff) << "\""
            << std::endl;
        break;
    }
    case SbRecordType::ENABLE_NODE: {
        dump_in_hex((char*)t.enable_node.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"enable_node\": \"" << std::string(buff) << "\""
            << std::endl;
        break;
    }
    case SbRecordType::DISABLE_NODE: {
        dump_in_hex((char*)t.disable_node.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"disable_node\": \"" << std::string(buff) << "\""
            << std::endl;
        break;
    }
    case SbRecordType::ADD_BLOCKCHAIN: {
        dump_in_hex((char*)t.add_blockchain.pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "  \"add_blockchain\": \"" << std::string(buff) << "\""
            << std::endl;
        break;
    }
    case SbRecordType::ADD_NODE_TO_BLOCKCHAIN: {
        out << "  \"add_node_to_blockchain\": {" << std::endl;
        dump_in_hex((char*)t.add_node_to_blockchain.node_pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "    \"node_pub_key\": \"" << std::string(buff) << "\", "
            << std::endl;
        dump_in_hex((char*)t.add_node_to_blockchain.blockchain_pub_key, 
                    buff, 
                    PUB_KEY_LENGTH);
        out << "    \"blockchain_pub_key\": \"" << std::string(buff)
            << "\", " << std::endl;
        out << "    \"alias\": \"" << 
            std::string((char*)t.add_node_to_blockchain.alias)
            << "\"" << std::endl;
        out << "  }" << std::endl;
        break;
    }
    case SbRecordType::REMOVE_NODE_FROM_BLOCKCHAIN: {
        out << "  \"remove_node_from_blockchain\": {" << std::endl;
        dump_in_hex((char*)t.remove_node_from_blockchain.node_pub_key, buff, 
                    PUB_KEY_LENGTH);
        out << "    \"node_pub_key\": \"" << std::string(buff) << "\", "
            << std::endl;
        dump_in_hex((char*)t.remove_node_from_blockchain.blockchain_pub_key, 
                    buff, 
                    PUB_KEY_LENGTH);
        out << "    \"blockchain_pub_key\": \"" << std::string(buff)
            << "\", " << std::endl;
        out << "    \"alias\": \"" << 
            std::string((char*)t.remove_node_from_blockchain.alias)
            << "\"" << std::endl;
        out << "  }" << std::endl;
        break;
    }
    case SbRecordType::SIGNATURES: {
        for (int i = 0; i < SB_SIGNATURES_PER_RECORD; i++) {
            const Signature* s = t.signatures + i;
            bool zeroes = true;
            for (int i = 0; i < PUB_KEY_LENGTH; i++)
                if (s->pubkey[0] != 0) {
                    zeroes = false;
                    break;
                }
            if (zeroes)
                break;
            dump_in_hex((char*)s->pubkey, buff, PUB_KEY_LENGTH);
            std::string pubkey(buff);
            dump_in_hex((char*)s->signature, buff, SIGNATURE_LENGTH);
            std::string signature(buff);

            out << "  \"signatures\": [" << std::endl;
            out << "    {" << std::endl;
            out << "      \"pubkey\": \"" << pubkey << "\", " << std::endl;
            out << "      \"signature\": \"" << signature << "\"" << std::endl;
            out << "    }" << std::endl;
            out << "  ]" << std::endl;
        }
        break;
    }
    }
    out << "}" << std::endl;
}


std::string read_key_from_file(std::string file_name) {
    std::ifstream t(file_name);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());    
    if (str.length() != KEY_LENGTH_HEX || !is_hex(str))
        PRECISE_THROW("cannot read, not a key: " << file_name);
    return str;
}

void save_key_to_file(std::string key, std::string file_name, bool set_permissions) {
    if (key.length() != KEY_LENGTH_HEX || !is_hex(key))
        PRECISE_THROW("cannot write, not a key: " << key);

    Poco::File pk(file_name);
    // not overwriting for safety; key files have to be removed manually
    if (pk.exists())
        PRECISE_THROW("cannot write, key exists " <<
                      file_name);

    std::ofstream ofs(file_name, std::ofstream::trunc);
    ofs << key;
    ofs.close();

    if (set_permissions)
        chmod(file_name.c_str(), S_IRUSR);
}

bool create_kp_identity(std::string& priv, std::string& pub) {
    private_key_t sk = {0};
    public_key_t pk = {0};
    if (create_keypair(&sk, &pk) != 0) {
        dump_in_hex((char*)sk.data, priv, ed25519_pubkey_SIZE);
        dump_in_hex((char*)pk.data, pub, ed25519_privkey_SIZE);
        return true;
    } else return false;
}

int create_keypair(private_key_t *sk, public_key_t *pk) {
    Poco::Random rng;
    // TODO: consider cryptographical safety
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    uint32_t seed = (uint32_t)t.tv_sec ^ (uint32_t)t.tv_nsec;
    rng.seed(seed);

    char* csk = (char*)sk;
    for (int i = 0; i < ed25519_privkey_SIZE; i++)
        csk[i] = rng.nextChar();
    ed25519_derive_public_key(sk, pk);
    return ED25519_SUCCESS;
}

std::string sign(uint8_t* mat, 
                 uint32_t mat_len,
                 const std::string& pub_key,
                 const std::string& priv_key) {
    signature_t sig;
    public_key_t pk;
    std::vector<char> pk0 = hex_to_bytes(pub_key);
    for (int i = 0; i < pk0.size(); i++)
        pk.data[i] = pk0[i];
    private_key_t sk;
    std::vector<char> sk0 = hex_to_bytes(priv_key);
    for (int i = 0; i < sk0.size(); i++)
        sk.data[i] = sk0[i];
    ed25519_sign(&sig, (const unsigned char*)mat, 
                 mat_len, 
                 &pk, &sk);
    std::string res;
    dump_in_hex((char*)sig.data, res, ed25519_signature_SIZE);
    return res;
}

std::string sign(std::string material, 
                 const std::string& pub_key,
                 const std::string& priv_key) {
    return sign((uint8_t*)material.c_str(), material.length(),
                pub_key, priv_key);
}

HederaTimestamp get_timestamp() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    HederaTimestamp res;
    res.seconds = t.tv_sec;
    res.nanos = t.tv_nsec;
    return res;
}


}
