#ifndef BLOCKCHAIN_GRADIDO_TRANSLATE_H
#define BLOCKCHAIN_GRADIDO_TRANSLATE_H

#include "gradido_messages.h"
#include "gradido_interfaces.h"

namespace gradido {
    
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

public:

    static void translate_from_ctr(const ConsensusTopicResponse& t, 
                                   MultipartTransaction& mt);
    static void translate_from_br(const grpr::BlockRecord& br, 
                                  HashedMultipartTransaction& hmt);
    static void translate_to_ctr(const MultipartTransaction& mt,
                                 ConsensusTopicResponse& t);
    
};
 
}

#endif
