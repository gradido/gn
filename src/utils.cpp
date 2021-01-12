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

// copied from libsodium: https://github.com/jedisct1/libsodium

char *
sodium_bin2hex(char* hex, const size_t hex_maxlen,
               const unsigned char *const bin, const size_t bin_len)
{
    size_t       i = (size_t) 0U;
    unsigned int x;
    int          b;
    int          c;

    if (bin_len >= SIZE_MAX / 2 || hex_maxlen <= bin_len * 2U) {
//        sodium_misuse(); /* LCOV_EXCL_LINE */
          return NULL;
    }
    while (i < bin_len) {
        c = bin[i] & 0xf;
        b = bin[i] >> 4;
        x = (unsigned char) (87U + c + (((c - 10U) >> 8) & ~38U)) << 8 |
            (unsigned char) (87U + b + (((b - 10U) >> 8) & ~38U));
        hex[i * 2U] = (char) x;
        x >>= 8;
        hex[i * 2U + 1U] = (char) x;
        i++;
    }
    hex[i * 2U] = 0U;

    return hex;
}

bool is_hex(std::string str) {
    for (int i = 0; i < str.length(); i++) {
        if (!((str[i] >= '0' && str[i] <= '9') ||
              (str[i] >= 'a' && str[i] <= 'f') ||
              (str[i] >= 'A' && str[i] <= 'F')))
            return false;
    }
    return true;
}

void dump_in_hex(const char* in, char* out, size_t in_len) {
    sodium_bin2hex(out, in_len * 2, (const unsigned char*)in, in_len);
/*
    for (size_t i = 0; i < in_len; i++)
        sprintf(out + (i * 2), "%02X", in[i]);
    out[in_len * 2] = 0;*/
}

void dump_in_hex(const char* in, std::string& out, size_t in_len) {
    int blen = in_len * 2 + 1;
    char buff[blen];
    memset(buff, 0, blen);
    dump_in_hex(in, buff, in_len);
    out = std::string(buff);
}

std::string get_as_str(TransactionType r) {
    switch (r) {
    case GRADIDO_CREATION:
        return "GRADIDO_CREATION";
    case ADD_GROUP_FRIEND:
        return "ADD_GROUP_FRIEND";
    case REMOVE_GROUP_FRIEND:
        return "REMOVE_GROUP_FRIEND";
    case ADD_USER:
        return "ADD_USER";
    case MOVE_USER_INBOUND:
        return "MOVE_USER_INBOUND";
    case MOVE_USER_OUTBOUND:
        return "MOVE_USER_OUTBOUND";
    case LOCAL_TRANSFER:
        return "LOCAL_TRANSFER";
    case INBOUND_TRANSFER:
        return "INBOUND_TRANSFER";
    case OUTBOUND_TRANSFER:
        return "OUTBOUND_TRANSFER";
    }
}

std::string get_as_str(TransactionResult r) {
    switch (r) {
    case SUCCESS:
        return "SUCCESS";
    case UNKNOWN:
        return "UNKNOWN";
    case NOT_ENOUGH_GRADIDOS:
        return "NOT_ENOUGH_GRADIDOS";
    case UNKNOWN_LOCAL_USER:
        return "UNKNOWN_LOCAL_USER";
    case UNKNOWN_REMOTE_USER:
        return "UNKNOWN_REMOTE_USER";
    case UNKNOWN_GROUP:
        return "UNKNOWN_GROUP";
    case OUTBOUND_TRANSACTION_FAILED:
        return "OUTBOUND_TRANSACTION_FAILED";
    case USER_ALREADY_EXISTS:
        return "USER_ALREADY_EXISTS";
    case GROUP_IS_ALREADY_FRIEND:
        return "GROUP_IS_ALREADY_FRIEND";
    case GROUP_IS_NOT_FRIEND:
        return "GROUP_IS_NOT_FRIEND";
    }
}

std::string get_as_str(StructurallyBadMessageResult r) {
    switch (r) {
    case UNDESERIALIZABLE:
        return "UNDESERIALIZABLE";
    case INVALID_SIGNATURE:
        return "INVALID_SIGNATURE";
    case MISSING_SIGNATURE:
        return "MISSING_SIGNATURE";
    case TOO_LARGE:
        return "TOO_LARGE";
    case NEGATIVE_AMOUNT:
        return "NEGATIVE_AMOUNT";
    case BAD_MEMO_CHARACTER:
        return "BAD_MEMO_CHARACTER";
    case BAD_ALIAS_CHARACTER:
        return "BAD_ALIAS_CHARACTER";
    case KEY_SIZE_NOT_CORRECT:
        return "KEY_SIZE_NOT_CORRECT";
    }
}

std::string get_as_str(GradidoRecordType r) {
    switch (r) {
    case BLANK:
        return "BLANK";
    case GRADIDO_TRANSACTION:
        return "GRADIDO_TRANSACTION";
    case MEMO:
        return "MEMO";
    case SIGNATURES:
        return "SIGNATURES";
    case STRUCTURALLY_BAD_MESSAGE:
        return "STRUCTURALLY_BAD_MESSAGE";
    case RAW_MESSAGE:
        return "RAW_MESSAGE";
    }
}

std::string get_as_str(GroupRegisterRecordType r) {
    switch (r) {
    case GROUP_RECORD:
        return "GROUP_RECORD";
    case GR_STRUCTURALLY_BAD_MESSAGE:
        return "GR_STRUCTURALLY_BAD_MESSAGE";
    case GR_RAW_MESSAGE:
        return "GR_RAW_MESSAGE";
    }
}

void dump_transaction_in_json(const GradidoRecord& t, std::ostream& out) {

    std::string record_type = get_as_str((GradidoRecordType)t.record_type);
    out << "{" << std::endl
        << "  \"record_type\": \"" << record_type << "\", " << std::endl;

    switch ((GradidoRecordType)t.record_type) {
    case GRADIDO_TRANSACTION: {
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
        case GRADIDO_CREATION: {
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
        case ADD_GROUP_FRIEND: {
            std::string group((char*)u.add_group_friend.group);
            out << "    \"add_group_friend\": {" << std::endl;
            out << "      \"group\": \"" << group << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case REMOVE_GROUP_FRIEND: {
            std::string group((char*)u.add_group_friend.group);
            out << "    \"remove_group_friend\": {" << std::endl;
            out << "      \"group\": \"" << group << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case ADD_USER: {
            dump_in_hex((char*)u.add_user.user, buff, PUB_KEY_LENGTH);
            std::string user(buff);
            out << "    \"add_user\": {" << std::endl;
            out << "      \"user\": \"" << user << "\"" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case MOVE_USER_INBOUND: {
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
        case MOVE_USER_OUTBOUND: {
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
        case LOCAL_TRANSFER: {
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
        case INBOUND_TRANSFER: {
            const InboundTransfer& tt = u.inbound_transfer;
            dump_in_hex((char*)tt.sender.user, buff, 
                        PUB_KEY_LENGTH);
            std::string sender(buff);
            dump_in_hex((char*)tt.receiver.user, buff, 
                        PUB_KEY_LENGTH);
            std::string receiver(buff);

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
            out << "      }" << std::endl;
            out << "    }," << std::endl;
            break;
        }
        case OUTBOUND_TRANSFER: {
            const OutboundTransfer& tt = u.outbound_transfer;
            dump_in_hex((char*)tt.sender.user, buff, 
                        PUB_KEY_LENGTH);
            std::string sender(buff);
            dump_in_hex((char*)tt.receiver.user, buff, 
                        PUB_KEY_LENGTH);
            std::string receiver(buff);

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

    case MEMO: {
        out << "  \"memo\": \"" << std::string((char*)t.memo) << "\" " << std::endl;
        break;
    }
    case SIGNATURES: {
        for (int i = 0; i < SIGNATURES_PER_RECORD; i++) {
            const Signature* s = t.signature + i;
            if (s->pubkey[0] == 0)
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
    case STRUCTURALLY_BAD_MESSAGE: {
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

std::string read_key_from_file(std::string file_name) {
    std::ifstream t(file_name);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());    
    if (str.length() != KEY_LENGTH_HEX || !is_hex(str))
        throw std::runtime_error("cannot read, not a key: " + file_name);
    return str;
}

void save_key_to_file(std::string key, std::string file_name, bool set_permissions) {
    if (key.length() != KEY_LENGTH_HEX || !is_hex(key))
        throw std::runtime_error("cannot write, not a key: " + key);

    Poco::File pk(file_name);
    // not overwriting for safety; key files have to be removed manually
    if (pk.exists())
        throw std::runtime_error("cannot write, key exists " + 
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
    char* csk = (char*)sk;
    for (int i = 0; i < ed25519_privkey_SIZE; i++)
        csk[i] = rng.nextChar();
    ed25519_derive_public_key(sk, pk);
    return ED25519_SUCCESS;
}



}
