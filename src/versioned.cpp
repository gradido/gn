#include "versioned.h"

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
                                 grpr::Transaction& out) {
        grpr::Handshake0 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        IGradidoConfig* conf = gf->get_conf();
        std::string ep = conf->get_grpc_endpoint();
        z.set_starter_endpoint(ep);

        out.set_version_number(vnum);
        std::string buff;
        z.SerializeToString(&buff);
        out.set_body_bytes(buff);
        out.set_success(true);
        return true;
    }

    bool Versioned_1::prepare_h1(proto::Timestamp ts,
                                 grpr::Transaction& out) {
        grpr::Handshake1 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        IGradidoConfig* conf = gf->get_conf();
        z.set_pub_key(conf->kp_get_pub_key());

        out.set_version_number(vnum);
        std::string buff;
        z.SerializeToString(&buff);
        out.set_body_bytes(buff);
        out.set_success(true);
        return true;
    }
    bool Versioned_1::prepare_h2(proto::Timestamp ts, 
                                 std::string type,
                                 grpr::Transaction& out) {
        grpr::Handshake2 z;
        auto ts0 = z.mutable_transaction_id();
        *ts0 = ts;
        IGradidoConfig* conf = gf->get_conf();
        z.set_pub_key(conf->kp_get_pub_key());
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
        IGradidoConfig* conf = gf->get_conf();
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

}
