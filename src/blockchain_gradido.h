#ifndef BLOCKCHAIN_GRADIDO_H
#define BLOCKCHAIN_GRADIDO_H

#include <map>
#include "gradido_interfaces.h"
#include "blockchain_base.h"

namespace gradido {

class BlockchainTransactionCompare {
public:
    bool operator()(const Batch<GradidoRecord>& lhs, 
                    const Batch<GradidoRecord>& rhs) const {
        // TODO: structurally bad message
        return 
            lhs.buff[0].transaction.hedera_transaction.sequenceNumber < 
            rhs.buff[0].transaction.hedera_transaction.sequenceNumber;
    }
};

class BlockchainGradido :
     public BlockchainBase<
     GradidoRecord, BlockchainGradido, IBlockchain, 
     BlockchainTransactionCompare>,
     IGradidoFacade::PairedTransactionListener {
 public:
    virtual void on_paired_transaction_done(Transaction t);

 public:
    using Parent = BlockchainBase<
        GradidoRecord, BlockchainGradido, IBlockchain, 
        BlockchainTransactionCompare>;

    bool is_valid(const GradidoRecord* rec, uint32_t count);

 protected:

    virtual bool get_latest_transaction_id(uint64_t& res);
    virtual bool get_transaction_id(uint64_t& res, 
                                    const Batch<GradidoRecord>& rec);
    virtual bool calc_fields_and_update_indexes_for(
                 Batch<GradidoRecord>& b);
    virtual void clear_indexes();
    virtual bool add_index_data();

 private:
    struct UserInfo {
        GradidoValue current_balance;
        uint64_t last_record_with_balance;
    };

    struct FriendGroupInfo {
    };

    std::map<std::string, UserInfo> user_index;
    std::map<std::string, FriendGroupInfo> friend_group_index;
    std::map<HederaTimestamp, uint64_t> outbound_transactions;

    std::string other_group;
    HederaTimestamp paired_transaction;
    Transaction paired_tr;
    
    bool waiting_for_paired;

 public:
    BlockchainGradido(std::string name,
                      Poco::Path root_folder,
                      IGradidoFacade* gf,
                      HederaTopicID topic_id);

    virtual bool get_paired_transaction(HederaTimestamp hti, 
                                        uint64_t& seq_num);
    virtual bool get_transaction(uint64_t seq_num, Transaction& t);
    virtual bool get_transaction(uint64_t seq_num, GradidoRecord& t);

    virtual std::vector<std::string> get_users();

};
 
}

#endif
