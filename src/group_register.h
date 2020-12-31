#ifndef GROUP_REGISTER_H
#define GROUP_REGISTER_H

#include "gradido_interfaces.h"
#include "blockchain_base_deprecated.h"

namespace gradido {

class GroupRegisterTransactionCompare {
public:
    bool operator()(const Batch<GroupRegisterRecord>& lhs, 
                    const Batch<GroupRegisterRecord>& rhs) const {
        return 
            lhs.buff[0].group_record.hedera_transaction.sequenceNumber < 
            rhs.buff[0].group_record.hedera_transaction.sequenceNumber;
    }
};
    
class GroupRegister :
     public BlockchainBaseDeprecated<
     GroupRegisterRecord, GroupRegister, IGroupRegisterBlockchain, 
     GroupRegisterTransactionCompare> {

public:
    using Parent = BlockchainBaseDeprecated<
        GroupRegisterRecord, GroupRegister, IGroupRegisterBlockchain, 
        GroupRegisterTransactionCompare>;

    bool is_valid(const GroupRegisterRecord* rec, uint32_t count);

protected:

    virtual bool get_latest_transaction_id(uint64_t& res);
    virtual bool get_transaction_id(uint64_t& res, 
                                    const Batch<GroupRegisterRecord>& rec);
    virtual bool calc_fields_and_update_indexes_for(
                 Batch<GroupRegisterRecord>& b);
    virtual void clear_indexes();
    virtual void on_blockchain_ready();
    virtual bool add_index_data();

private:

    // facade's continue_init_after_group_register_done() should be
    // called only once; but there can be revalidations during the
    // lifetime of this object
    bool first_good_validation;
    std::map<std::string, GroupRecord> aliases;
public:
    GroupRegister(Poco::Path root_folder,
                  IGradidoFacade* gf,
                  HederaTopicID topic_id);

    virtual bool get_topic_id(std::string alias, HederaTopicID& res);

    virtual void add_record(std::string alias, HederaTopicID tid);
    virtual std::vector<GroupInfo> get_groups();
};

}

#endif
