#include "versioned.h"
#include "utils.h"

namespace gradido {

    bool RequestSigValidator_1::create_node_handshake0(
                                grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::create_node_handshake2(
                                grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::create_ordering_node_handshake2(
                                grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::submit_message(grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::subscribe_to_blockchain(
                                grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::submit_log_message(grpr::Transaction t) {
        return true;
    }
    bool RequestSigValidator_1::ping(grpr::Transaction t) {
        return true;
    }

    bool ResponseSigValidator_1::create_node_handshake0(
                                grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::create_node_handshake2(
                                grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::create_ordering_node_handshake2(
                                grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::submit_message(grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::subscribe_to_blockchain(
                                grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::submit_log_message(grpr::Transaction t) {
        return true;
    }
    bool ResponseSigValidator_1::ping(grpr::Transaction t) {
        return true;
    }


    IVersionedGrpc* Versioned_1::get_request_sig_validator() {
        return &req_val;
    }
    IVersionedGrpc* Versioned_1::get_response_sig_validator() {
        return &resp_val;
    }
    void Versioned_1::sign(grpr::Transaction* t) {
    }
    bool Versioned_1::translate_log_message(grpr::Transaction t, 
                                            std::string& out) {
    }

    bool Versioned_1::translate(grpr::Transaction t, 
                                grpr::Handshake0& out) {
        // TODO: handle exceptions
        out.ParseFromString(t.body_bytes());

        return true;
    }

    bool Versioned_1::translate(grpr::Transaction t, 
                                grpr::Handshake1& out) {
        return true;
    }

    bool Versioned_1::translate(grpr::Transaction t, 
                                grpr::Handshake2& out) {
        return true;
    }

    bool Versioned_1::translate(grpr::Transaction t, 
                                grpr::Handshake3& out) {
        return true;
    }
    bool Versioned_1::translate(grpr::Transaction t, 
                                Batch<SbRecord>& out) {
        return true;
    }

    bool Versioned_1::translate(grpr::Transaction t,
                                grpr::OrderingRequest& ore) {
        return true;
    }
    
    bool Versioned_1::translate_sb_transaction(std::string bytes,
                                               grpr::Transaction& out) {
        return true;
    }

    bool Versioned_1::prepare_h0(proto::Timestamp ts,
                                 grpr::Transaction& out,
                                 std::string endpoint) {
        grpr::Handshake0 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        z.set_starter_endpoint(endpoint);

        out.set_version_number(vnum);
        std::string buff;
        z.SerializeToString(&buff);
        out.set_body_bytes(buff);
        out.set_success(true);
        return true;
    }

    bool Versioned_1::prepare_h1(proto::Timestamp ts,
                                 grpr::Transaction& out,
                                 std::string kp_pub_key) {
        grpr::Handshake1 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        z.set_pub_key(kp_pub_key);

        out.set_version_number(vnum);
        std::string buff;
        z.SerializeToString(&buff);
        out.set_body_bytes(buff);
        out.set_success(true);
        return true;
    }
    bool Versioned_1::prepare_h2(proto::Timestamp ts, 
                                 std::string type,
                                 grpr::Transaction& out,
                                 std::string kp_pub_key) {
        grpr::Handshake2 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        z.set_pub_key(kp_pub_key);
        z.set_type(type);

        out.set_version_number(vnum);
        std::string buff;
        z.SerializeToString(&buff);
        out.set_body_bytes(buff);
        out.set_success(true);
        return true;
    }
    bool Versioned_1::prepare_h3(proto::Timestamp ts, 
                                 grpr::Transaction sb,
                                 grpr::Transaction& out) {
        grpr::Handshake3 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        std::string buff;
        sb.SerializeToString(&buff);
        z.set_sb_transaction(buff);

        out.set_version_number(vnum);
        std::string buff2;
        z.SerializeToString(&buff2);
        out.set_body_bytes(buff2);
        out.set_success(true);
        return true;
    }

    bool Versioned_1::prepare_add_node(proto::Timestamp ts, 
                                       std::string type,
                                       grpr::Transaction& out) {
        return true;
    }

    ITransactionFactory* Versioned_1::get_transaction_factory() {
        return &transaction_factory;
    }

    ISbTransactionFactory* Versioned_1::get_sb_transaction_factory() {
        return &sb_transaction_factory;
    }

    ITransactionFactoryBase::ExitCode 
    FactoryBase::check_id(std::string user) {
        if (user.length() != PUB_KEY_LENGTH_HEX)
            return ExitCode::ID_BAD_LENGTH;

        if (!is_hex(user))
            return ExitCode::ID_NOT_A_HEX;
    }

    ITransactionFactoryBase::ExitCode 
    FactoryBase::prepare_memo_recs(
                               std::string memo, 
                               Transaction** res,
                               uint8_t** out, 
                               uint32_t* out_length) {
        if (memo.length() > MAX_TOTAL_MEMO_LENGTH)
            return ExitCode::MEMO_TOO_LONG;

        if (contains_null(memo))
            return ExitCode::MEMO_CONTAINS_ZERO_CHAR;

        int memo_parts = memo.length() < MEMO_MAIN_SIZE ? 0 : 
            (memo.length() - MEMO_MAIN_SIZE) / MEMO_FULL_SIZE + 1;

        *res = new Transaction[1 + memo_parts];
        Transaction* r = *res;
        r[0].version_number = 1;
        r[0].transaction_id = get_timestamp();

        if (memo_parts == 0) {
            memcpy(r[0].memo, memo.c_str(), memo.length());
        } else {
            memcpy(r[0].memo, memo.c_str(), MEMO_MAIN_SIZE);
            int cp = MEMO_MAIN_SIZE;
            for (int i = 0; i < memo_parts; i++) {
                int cp2 = fmin(MEMO_FULL_SIZE, memo.length() - cp);
                memcpy(r[i + 1].memo, memo.c_str() + cp, cp2);
                cp += cp2;
            }
        }

        *out = (uint8_t*)r;
        *out_length = sizeof(Transaction) * (1 + memo_parts);
        return ExitCode::OK;
    }

    TransactionFactory_1::ExitCode 
    TransactionFactory_1::create_gradido_creation(
                            std::string user,
                            uint64_t amount,
                            std::string memo,
                            uint8_t** out, uint32_t* out_length) {

        ExitCode ec;
        ec = check_id(user);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].gradido_creation.user, user.c_str(), 
               PUB_KEY_LENGTH_HEX);
        res[0].gradido_creation.amount.amount = amount;
        
        return ExitCode::OK;
    }

    TransactionFactory_1::ExitCode TransactionFactory_1::create_local_transfer(
                                           std::string user0,
                                           std::string user1,
                                           uint64_t amount,
                                           std::string memo,
                                           uint8_t** out, 
                                           uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(user0);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(user1);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].local_transfer.sender.user, user0.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].local_transfer.receiver.user, user1.c_str(), 
               PUB_KEY_LENGTH_HEX);

        res[0].local_transfer.amount.amount = amount;
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::create_inbound_transfer(
                                             std::string user0,
                                             std::string user1,
                                             std::string other_group,
                                             uint64_t amount,
                                             std::string memo,
                                             uint8_t** out, 
                                             uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(user0);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(user1);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(other_group);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].inbound_transfer.sender.user, user0.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].inbound_transfer.receiver.user, user1.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].inbound_transfer.other_group, other_group.c_str(), 
               PUB_KEY_LENGTH_HEX);

        res[0].inbound_transfer.amount.amount = amount;
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::create_outbound_transfer(
                                              std::string user0,
                                              std::string user1,
                                              std::string other_group,
                                              uint64_t amount,
                                              std::string memo,
                                              uint8_t** out, 
                                              uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(user0);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(user1);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(other_group);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].outbound_transfer.sender.user, user0.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].outbound_transfer.receiver.user, user1.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].outbound_transfer.other_group, other_group.c_str(), 
               PUB_KEY_LENGTH_HEX);

        res[0].outbound_transfer.amount.amount = amount;
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::add_group_friend(
                                      std::string id,
                                      std::string memo,
                                      uint8_t** out, 
                                      uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(id);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].add_group_friend.group, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::remove_group_friend(
                                         std::string id,
                                         std::string memo,
                                         uint8_t** out, 
                                         uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(id);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].remove_group_friend.group, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        
        return ExitCode::OK;
    }
    
    TransactionFactory_1::ExitCode TransactionFactory_1::add_user(
                              std::string id,
                              std::string memo,
                              uint8_t** out, 
                              uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(id);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].add_user.user, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::move_user_inbound(
                                       std::string id,
                                       std::string other_group,
                                       std::string memo,
                                       uint8_t** out, 
                                       uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(id);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(other_group);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].move_user_inbound.user, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].move_user_inbound.other_group, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        
        return ExitCode::OK;
    }
    TransactionFactory_1::ExitCode TransactionFactory_1::move_user_outbound(
                                        std::string id,
                                        std::string other_group,
                                        std::string memo,
                                        uint8_t** out, 
                                        uint32_t* out_length) {
        ExitCode ec;
        ec = check_id(id);
        if (ec != ExitCode::OK)
            return ec;
        ec = check_id(other_group);
        if (ec != ExitCode::OK)
            return ec;

        Transaction* res;
        ec = prepare_memo_recs(memo, &res, out, out_length);
        if (ec != ExitCode::OK)
            return ec;
        
        memcpy(res[0].move_user_outbound.user, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        memcpy(res[0].move_user_outbound.other_group, id.c_str(), 
               PUB_KEY_LENGTH_HEX);
        
        return ExitCode::OK;
    }
    

}
