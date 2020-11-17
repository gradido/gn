#ifndef BLOCKCHAIN_GRADIDO_TRANSLATE_H
#define BLOCKCHAIN_GRADIDO_TRANSLATE_H

#include "gradido_messages.h"
#include "gradido_interfaces.h"

namespace gradido {

class TranslationException : public std::exception
{
public:
    explicit TranslationException(const char* message) : msg_(message) {}
    explicit TranslationException(const std::string& message) : msg_(message) {}
    virtual ~TranslationException() throw () {}
    virtual const char* what() const throw () {return msg_.c_str();}
protected:
    std::string msg_;
};

class StructuralException : public TranslationException {
public:
   StructuralException(std::string message) : 
    TranslationException(message) {}
};

#define TRANSLATION_EXCEPTION(name, message)               \
class name : public TranslationException {                 \
public:                                                    \
   name() : TranslationException(message) {}               \
};

#define STRUCTURAL_EXCEPTION(name, message)                \
class name : public StructuralException {                  \
public:                                                    \
   name() : StructuralException(message) {}                \
};

STRUCTURAL_EXCEPTION(NotEnoughSignatures, "need at least one signature for any transaction");
STRUCTURAL_EXCEPTION(TooManySignatures, "too many signatures");
STRUCTURAL_EXCEPTION(RecCountZero, "rec_count == 0");
STRUCTURAL_EXCEPTION(RecCountTooLarge, "rec_count > MAX_RECORD_PARTS");
STRUCTURAL_EXCEPTION(BadFirstRecType, "first rec is not of type GRADIDO_TRANSACTION");

class BadRecType : public StructuralException {
public:                                                    
 BadRecType(int i) : StructuralException("bad rec type at " + 
                                          std::to_string(i)) {}
};                                                      

STRUCTURAL_EXCEPTION(ExpectedLongMemo, "expected long memo");
STRUCTURAL_EXCEPTION(OmittedSignatureInRecord, "omitted signature in record")

STRUCTURAL_EXCEPTION(EmptySignature, "empty signature")
STRUCTURAL_EXCEPTION(SignatureCountNotCorrect, "signature count not correct")

class TransactionUtils {

public:
    // TODO: optimize with move or putting result as a parameter
    // (although it is likely that compiler does the thing already)
    // TODO: check enum overflows

    static HederaTimestamp translate_Timestamp_from_pb(const ::proto::Timestamp& ts);
    static ::proto::Timestamp translate_Timestamp_to_pb(const HederaTimestamp& ts);
    static HederaAccountID translate_AccountID_from_pb(const ::proto::AccountID& val);
    static ::proto::AccountID translate_AccountID_to_pb(const HederaAccountID& val);
    static HederaTopicID translate_TopicID_from_pb(const ::proto::TopicID& val);
    static ::proto::TopicID translate_TopicID_to_pb(const HederaTopicID& val);
    static HederaTransactionID translate_TransactionID_from_pb(const ::proto::TransactionID& val);
    static ::proto::TransactionID translate_TransactionID_to_pb(const HederaTransactionID& val);
    static HederaTransaction translate_HederaTransaction_from_pb(const ConsensusTopicResponse& val);
    static ConsensusTopicResponse translate_HederaTransaction_to_pb(const HederaTransaction& val);


    static Signature translate_Signature_from_pb(grpr::SignaturePair sig_pair);

    static void translate_Signature_to_pb(grpr::SignatureMap* map,
                                          const Signature& signature);

public:

    static void translate_from_ctr(const ConsensusTopicResponse& t, 
                                   GroupRegisterRecordBatch& batch);

    static void translate_from_ctr(const ConsensusTopicResponse& t, 
                                   GradidoRecordBatch& batch);
    

    /*
    static void translate_from_ctr(const ConsensusTopicResponse& t, 
                                   MultipartTransaction& mt);
    static void translate_from_br(const grpr::BlockRecord& br, 
                                  HashedMultipartTransaction& hmt);

    static void translate_to_ctr(const MultipartTransaction& mt,
                                 ConsensusTopicResponse& ctr);

    static void check_structure(const MultipartTransaction& mt);
    */    
    static void check_structure(const ConsensusTopicResponse& t);

    static bool is_empty(uint8_t* str, size_t size);
    static bool is_empty(const Signature& signature);

    // TODO: protected
    static void check_alias(const uint8_t* str);
    static void check_memo(const uint8_t* str, const uint8_t* str2);

};
 
}

#endif
