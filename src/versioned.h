#ifndef VERSIONED_H
#define VERSIONED_H

#include "gradido_interfaces.h"

namespace gradido {

class EmptyVersioned : public IVersioned {
public:
    virtual IVersionedGrpc* get_request_sig_validator() {NOT_SUPPORTED;}
    virtual IVersionedGrpc* get_response_sig_validator() {NOT_SUPPORTED;}
    virtual void sign(grpr::Transaction* t) {NOT_SUPPORTED;}
    virtual bool translate_log_message(grpr::Transaction t, 
                                       std::string& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake0& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake3& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, 
                           Batch<SbRecord>& out) {NOT_SUPPORTED;}
    
    virtual bool translate_sb_transaction(std::string bytes,
                                          grpr::Transaction& out) {NOT_SUPPORTED;}


    virtual bool prepare_h1(proto::Timestamp ts,
                            grpr::Transaction& out) {NOT_SUPPORTED;}
    virtual bool prepare_h2(proto::Timestamp ts, 
                            grpr::Transaction& out) {NOT_SUPPORTED;}
};

class RequestSigValidator_1 : IVersionedGrpc {
private:
    IGradidoFacade* gf;
public:
    RequestSigValidator_1(IGradidoFacade* gf) : gf(gf) {}
    virtual bool create_node_handshake0(grpr::Transaction t);
    virtual bool create_node_handshake2(grpr::Transaction t);
    virtual bool create_ordering_node_handshake2(grpr::Transaction t);
    virtual bool submit_message(grpr::Transaction t);
    virtual bool subscribe_to_blockchain(grpr::Transaction t);
    virtual bool submit_log_message(grpr::Transaction t);
    virtual bool ping(grpr::Transaction t);
};

class ResponseSigValidator_1 : IVersionedGrpc {
private:
    IGradidoFacade* gf;
public:
    ResponseSigValidator_1(IGradidoFacade* gf) : gf(gf) {}
    virtual bool create_node_handshake0(grpr::Transaction t);
    virtual bool create_node_handshake2(grpr::Transaction t);
    virtual bool create_ordering_node_handshake2(grpr::Transaction t);
    virtual bool submit_message(grpr::Transaction t);
    virtual bool subscribe_to_blockchain(grpr::Transaction t);
    virtual bool submit_log_message(grpr::Transaction t);
    virtual bool ping(grpr::Transaction t);
};


class Versioned_1 : public EmptyVersioned {
private:
    IGradidoFacade* gf;
    RequestSigValidator_1 req_val;
    ResponseSigValidator_1 resp_val;

public:
    Versioned_1(IGradidoFacade* gf) : gf(gf), req_val(gf), 
        resp_val(gf) {}
    virtual IVersionedGrpc* get_request_sig_validator();
    virtual IVersionedGrpc* get_response_sig_validator();
    virtual void sign(grpr::Transaction* t);
    virtual bool translate_log_message(grpr::Transaction t, 
                                       std::string& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake0& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake3& out);
    virtual bool translate(grpr::Transaction t, 
                           Batch<SbRecord>& out);
    virtual bool translate(grpr::Transaction t,
                           grpr::OrderingRequest& ore);
    
    virtual bool translate_sb_transaction(std::string bytes,
                                          grpr::Transaction& out);


    virtual bool prepare_h1(proto::Timestamp ts,
                            grpr::Transaction& out);
    virtual bool prepare_h2(proto::Timestamp ts, 
                            grpr::Transaction& out);
};
}

#endif
