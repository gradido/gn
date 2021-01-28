#ifndef VERSIONED_H
#define VERSIONED_H

#include "gradido_interfaces.h"

namespace gradido {

// should keep subclasses independent from IGradidoFacade; this 
// simplifies maintenance

class EmptyVersioned : public IVersioned {
public:
    virtual IVersionedGrpc* get_request_sig_validator() {NOT_SUPPORTED;}
    virtual IVersionedGrpc* get_response_sig_validator() {NOT_SUPPORTED;}
    virtual void sign(grpr::Transaction* t) {NOT_SUPPORTED;}
    virtual bool translate_log_message(grpr::Transaction t, 
                                       std::string& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake0& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake1& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake2& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, grpr::Handshake3& out) {NOT_SUPPORTED;}
    virtual bool translate(grpr::Transaction t, 
                           Batch<SbRecord>& out) {NOT_SUPPORTED;}
    
    virtual bool translate_sb_transaction(std::string bytes,
                                          grpr::Transaction& out) {NOT_SUPPORTED;}


    virtual bool prepare_h0(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string endpoint) {NOT_SUPPORTED;}
    virtual bool prepare_h1(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string kp_pub_key) {NOT_SUPPORTED;}
    virtual bool prepare_h2(proto::Timestamp ts, 
                            std::string type,
                            grpr::Transaction& out,
                            std::string kp_pub_key) {NOT_SUPPORTED;}
    virtual bool prepare_h3(proto::Timestamp ts, 
                            grpr::Transaction sb,
                            grpr::Transaction& out) {NOT_SUPPORTED;}
    virtual ITransactionFactory* get_transaction_factory() {NOT_SUPPORTED;}
    virtual ISbTransactionFactory* get_sb_transaction_factory() {NOT_SUPPORTED;}
};

class RequestSigValidator_1 : public IVersionedGrpc {
public:
    RequestSigValidator_1() {}
    virtual bool create_node_handshake0(grpr::Transaction t);
    virtual bool create_node_handshake2(grpr::Transaction t);
    virtual bool create_ordering_node_handshake2(grpr::Transaction t);
    virtual bool submit_message(grpr::Transaction t);
    virtual bool subscribe_to_blockchain(grpr::Transaction t);
    virtual bool submit_log_message(grpr::Transaction t);
    virtual bool ping(grpr::Transaction t);
};

class ResponseSigValidator_1 : public IVersionedGrpc {
public:
    ResponseSigValidator_1() {}
    virtual bool create_node_handshake0(grpr::Transaction t);
    virtual bool create_node_handshake2(grpr::Transaction t);
    virtual bool create_ordering_node_handshake2(grpr::Transaction t);
    virtual bool submit_message(grpr::Transaction t);
    virtual bool subscribe_to_blockchain(grpr::Transaction t);
    virtual bool submit_log_message(grpr::Transaction t);
    virtual bool ping(grpr::Transaction t);
};

class FactoryBase : public ITransactionFactoryBase {
private:
    const int vnum;

protected:
    FactoryBase(int vnum) : vnum(vnum) {}

    ExitCode check_id(std::string user);
    ExitCode prepare_memo_recs(std::string memo, 
                               Transaction** res,
                               uint8_t** out, 
                               uint32_t* out_length);

};

class TransactionFactory_1 : private FactoryBase,
     public ITransactionFactory {
 public:
    TransactionFactory_1() : FactoryBase(1) {}
    virtual ExitCode create_gradido_creation(std::string user,
                                             uint64_t amount,
                                             std::string memo,
                                             uint8_t** out, 
                                             uint32_t* out_length);
    virtual ExitCode create_local_transfer(std::string user0,
                                           std::string user1,
                                           uint64_t amount,
                                           std::string memo,
                                           uint8_t** out, 
                                           uint32_t* out_length);
    virtual ExitCode create_inbound_transfer(std::string user0,
                                             std::string user1,
                                             std::string other_group,
                                             uint64_t amount,
                                             std::string memo,
                                             uint8_t** out, 
                                             uint32_t* out_length);
    virtual ExitCode create_outbound_transfer(std::string user0,
                                              std::string user1,
                                              std::string other_group,
                                              uint64_t amount,
                                              std::string memo,
                                              uint8_t** out, 
                                              uint32_t* out_length);
    virtual ExitCode add_group_friend(std::string id,
                                      std::string memo,
                                      uint8_t** out, 
                                      uint32_t* out_length);
    virtual ExitCode remove_group_friend(std::string id,
                                         std::string memo,
                                         uint8_t** out, 
                                         uint32_t* out_length);
    
    virtual ExitCode add_user(std::string id,
                              std::string memo,
                              uint8_t** out, 
                              uint32_t* out_length);
    virtual ExitCode move_user_inbound(std::string id,
                                       std::string other_group,
                                       std::string memo,
                                       uint8_t** out, 
                                       uint32_t* out_length);
    virtual ExitCode move_user_outbound(std::string id,
                                        std::string other_group,
                                        std::string memo,
                                        uint8_t** out, 
                                        uint32_t* out_length);

};

class SbTransactionFactory_1 : private FactoryBase,
    public ISbTransactionFactory {
public:
    SbTransactionFactory_1() : FactoryBase(1) {}

    // TODO: pull things from utilities
         
    
};



class Versioned_1 : public EmptyVersioned {
private:
    RequestSigValidator_1 req_val;
    ResponseSigValidator_1 resp_val;
    TransactionFactory_1 transaction_factory;
    SbTransactionFactory_1 sb_transaction_factory;
    const int vnum = 1;
public:
    Versioned_1() {}
    virtual IVersionedGrpc* get_request_sig_validator();
    virtual IVersionedGrpc* get_response_sig_validator();
    virtual void sign(grpr::Transaction* t);
    virtual bool translate_log_message(grpr::Transaction t, 
                                       std::string& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake0& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake1& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake2& out);
    virtual bool translate(grpr::Transaction t, grpr::Handshake3& out);
    virtual bool translate(grpr::Transaction t, 
                           Batch<SbRecord>& out);
    virtual bool translate(grpr::Transaction t,
                           grpr::OrderingRequest& ore);
    
    virtual bool translate_sb_transaction(std::string bytes,
                                          grpr::Transaction& out);

    virtual bool prepare_h0(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string endpoint);
    virtual bool prepare_h1(proto::Timestamp ts,
                            grpr::Transaction& out,
                            std::string kp_pub_key);
    virtual bool prepare_h2(proto::Timestamp ts, 
                            std::string type,
                            grpr::Transaction& out,
                            std::string kp_pub_key);
    virtual bool prepare_h3(proto::Timestamp ts, 
                            grpr::Transaction sb,
                            grpr::Transaction& out);
    virtual bool prepare_add_node(proto::Timestamp ts, 
                                  std::string type,
                                  grpr::Transaction& out);
    virtual ITransactionFactory* get_transaction_factory();
    virtual ISbTransactionFactory* get_sb_transaction_factory();

};
}

#endif
