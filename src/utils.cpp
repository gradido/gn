#include "utils.h"

namespace gradido {

void dump_transaction(const Transaction& t, std::ostream& out) {
    // TODO: more elaborate
    out << "seq_num: " << t.hedera_transaction.sequenceNumber <<
        "; transaction_type: " << t.transaction_type;
}

void dump_in_hex(const char* in, char* out, size_t in_len) {
    size_t i = 0;
    for (i = 0; i < in_len; i++)
        sprintf(out + (i * 2), "%02X", in[i]);
    out[i * 2 + 1] = 0;
}


void dump_transaction_in_json(const Transaction& t, std::ostream& out) {

    char buff[1024];
    dump_in_hex(t.reserved, buff, 32);
    std::string reserved(buff);

    dump_in_hex(t.hedera_transaction.runningHash, buff, 64);
    std::string running_hash(buff);

    out << "{" << std::endl
        << "  \"hedera_transaction\": {" << std::endl
        << "    \"consensusTimestamp\": {" << std::endl
        << "      \"seconds\": " << t.hedera_transaction.consensusTimestamp.seconds << ", " << std::endl
        << "      \"nanos\": " << t.hedera_transaction.consensusTimestamp.nanos  << std::endl
        << "    }," << std::endl
        << "    \"runningHash\": \"" << running_hash << "\", " << std::endl
        << "    \"sequenceNumber\": " << t.hedera_transaction.sequenceNumber << ", " << std::endl
        << "    \"runningHashVersion\": " << t.hedera_transaction.runningHashVersion << std::endl
        << "  }," << std::endl
        << "  \"transaction_type\": " << (int)t.transaction_type << ", " << std::endl;

    switch (t.transaction_type) {
    case GRADIDO_TRANSACTION: {
        dump_in_hex(t.data.gradido_transaction.sender_signature, buff, 64);
        std::string sender_signature(buff);

        out << "  \"gradido_transaction\": {" << std::endl
            << "    \"gradido_transfer_type\": " << t.data.gradido_transaction.gradido_transfer_type << ", "  << std::endl;

        switch (t.data.gradido_transaction.gradido_transfer_type) {
        case LOCAL: {
            out << "    \"local\": {" << std::endl
                << "      \"user_from\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.local.user_from.user_id << ", " << std::endl
                << "        \"prev_user_rec_num\": " << t.data.gradido_transaction.data.local.user_from.prev_user_rec_num << ", " << std::endl
                << "        \"new_balance\": " << t.data.gradido_transaction.data.local.user_from.new_balance << std::endl
                << "      }," << std::endl
                << "      \"user_to\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.local.user_to.user_id << ", " << std::endl
                << "        \"prev_user_rec_num\": " << t.data.gradido_transaction.data.local.user_to.prev_user_rec_num << ", " << std::endl
                << "        \"new_balance\": " << t.data.gradido_transaction.data.local.user_to.new_balance << std::endl
                << "      }" << std::endl
                << "    },"  << std::endl;
            break;
        }
        case INBOUND: {
            out << "    \"outbound\": {" << std::endl
                << "      \"user_from\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.inbound.user_from.user_id << ", " << std::endl
                << "        \"group_id\": " << t.data.gradido_transaction.data.inbound.user_from.group_id << std::endl
                << "      }," << std::endl
                << "      \"user_to\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.inbound.user_to.user_id << ", " << std::endl
                << "        \"prev_user_rec_num\": " << t.data.gradido_transaction.data.inbound.user_to.prev_user_rec_num << ", " << std::endl
                << "        \"new_balance\": " << t.data.gradido_transaction.data.inbound.user_to.new_balance << std::endl
                << "      }," << std::endl
                << "      \"paired_transaction_id\": {" << std::endl
                << "        \"seconds\": " << t.data.gradido_transaction.data.inbound.paired_transaction_id.seconds << ", " << std::endl
                << "        \"nanos\": " << t.data.gradido_transaction.data.inbound.paired_transaction_id.nanos  << std::endl
                << "        }" << std::endl
                << "      }" << std::endl
                << "    },"  << std::endl;
            break;
        }
        case OUTBOUND: {

            out << "    \"inbound\": {" << std::endl
                << "      \"user_from\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.outbound.user_from.user_id << ", " << std::endl
                << "        \"prev_user_rec_num\": " << t.data.gradido_transaction.data.outbound.user_from.prev_user_rec_num << ", " << std::endl
                << "        \"new_balance\": " << t.data.gradido_transaction.data.outbound.user_from.new_balance << std::endl
                << "      }," << std::endl
                << "      \"user_to\": {" << std::endl
                << "        \"user_id\": " << t.data.gradido_transaction.data.outbound.user_to.user_id << ", " << std::endl
                << "        \"group_id\": " << t.data.gradido_transaction.data.outbound.user_to.group_id << std::endl
                << "      }," << std::endl
                << "      \"paired_transaction_id\": {" << std::endl
                << "        \"seconds\": " << t.data.gradido_transaction.data.inbound.paired_transaction_id.seconds << ", " << std::endl
                << "        \"nanos\": " << t.data.gradido_transaction.data.inbound.paired_transaction_id.nanos  << std::endl
                << "        }" << std::endl
                << "      }" << std::endl
                << "    }," << std::endl;
            break;
        }
        }

        out << "    \"amount\": " << t.data.gradido_transaction.amount << ", "  << std::endl
            << "    \"amount_with_deduction\": " << t.data.gradido_transaction.amount_with_deduction << ", " << std::endl
            << "    \"sender_signature\": \"" << sender_signature << "\"" << std::endl
            << "  }," << std::endl;
        break;
    }
    case GROUP_UPDATE: {
        // TODO
        break;
    }
    case GROUP_FRIENDS_UPDATE: {
        // TODO
        break;
    }
    case GROUP_MEMBER_UPDATE: {
        dump_in_hex(t.data.group_member_update.public_key, buff, 32);
        std::string public_key(buff);

        dump_in_hex(t.data.group_member_update.update_author_signature, buff, 64);
        std::string update_author_signature(buff);


        out << "  \"group_member_update\": {" << std::endl
            << "    \"user_id\": " << t.data.group_member_update.user_id << ", " << std::endl
            << "    \"member_status\": " << (int)t.data.group_member_update.member_status << ", " << std::endl
            << "    \"member_update_type\": " << t.data.group_member_update.member_update_type << ", " << std::endl
            << "    \"is_disabled\": " << t.data.group_member_update.is_disabled << ", " << std::endl
            << "    \"public_key\": \"" << public_key << "\", " << std::endl
            << "    \"update_author_user_id\": " << t.data.group_member_update.update_author_user_id << ", " << std::endl
            << "    \"update_author_signature\": \"" << update_author_signature << "\", " << std::endl
            << "  }," << std::endl;
        break;
    }
    }        

    out << "  \"result\": " << (int)t.result << ", " << std::endl
        << "  \"version_number\": " << (int)t.version_number << ", " << std::endl
        << "  \"reserved\": \"" << reserved << "\"" << std::endl
        << "}";
}


}
