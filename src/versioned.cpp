#include "versioned.h"

namespace gradido {

    bool RequestSigValidator_1::create_node_handshake0(
                                grpr::Transaction t) {
    }
    bool RequestSigValidator_1::create_node_handshake2(
                                grpr::Transaction t) {
    }
    bool RequestSigValidator_1::create_ordering_node_handshake2(
                                grpr::Transaction t) {
    }
    bool RequestSigValidator_1::submit_message(grpr::Transaction t) {
    }
    bool RequestSigValidator_1::subscribe_to_blockchain(
                                grpr::Transaction t) {
    }
    bool RequestSigValidator_1::submit_log_message(grpr::Transaction t) {
    }
    bool RequestSigValidator_1::ping(grpr::Transaction t) {
    }

    bool ResponseSigValidator_1::create_node_handshake0(
                                grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::create_node_handshake2(
                                grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::create_ordering_node_handshake2(
                                grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::submit_message(grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::subscribe_to_blockchain(
                                grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::submit_log_message(grpr::Transaction t) {
    }
    bool ResponseSigValidator_1::ping(grpr::Transaction t) {
    }


    IVersionedGrpc* Versioned_1::get_request_sig_validator() {
    }
    IVersionedGrpc* Versioned_1::get_response_sig_validator() {
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
    }

    bool Versioned_1::translate(grpr::Transaction t, 
                                grpr::Handshake3& out) {
    }
    bool Versioned_1::translate(grpr::Transaction t, 
                                Batch<SbRecord>& out) {
    }

    bool Versioned_1::translate(grpr::Transaction t,
                                grpr::OrderingRequest& ore) {
    }
    
    bool Versioned_1::translate_sb_transaction(std::string bytes,
                                               grpr::Transaction& out) {
    }

    bool Versioned_1::prepare_h1(proto::Timestamp ts,
                                 grpr::Transaction& out) {
    }
    bool Versioned_1::prepare_h2(proto::Timestamp ts, 
                                 grpr::Transaction& out) {
    }

}
