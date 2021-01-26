#ifndef SUBCLUSTER_BLOCKCHAIN_H
#define SUBCLUSTER_BLOCKCHAIN_H

#include "gradido_interfaces.h"
#include "blockchain_base.h"

namespace gradido {

class SbTransactionCompare {
public:
    bool operator()(const Batch<SbRecord>& lhs, 
                    const Batch<SbRecord>& rhs) const {
        return 
            lhs.buff[0].hedera_transaction.sequenceNumber < 
            rhs.buff[0].hedera_transaction.sequenceNumber;
    }
};

class SubclusterBlockchain :
     public BlockchainBase<
     SbRecord, SubclusterBlockchain, ISubclusterBlockchain, 
     SbTransactionCompare> {

public:
    using Parent = BlockchainBase<
        SbRecord, SubclusterBlockchain, ISubclusterBlockchain, 
        SbTransactionCompare>;

    bool is_valid(const SbRecord* rec, uint32_t count);

protected:

    virtual bool get_fetch_endpoint(std::string& res);
    virtual bool get_latest_transaction_id(uint64_t& res);
    virtual bool get_transaction_id(uint64_t& res, 
                                    const Batch<SbRecord>& rec);
    virtual bool calc_fields_and_update_indexes_for(
                 Batch<SbRecord>& b);
    virtual void clear_indexes();
    virtual bool add_index_data();
    virtual void on_blockchain_ready();

private:

    // facade's continue_init_after_sb_done() should be
    // called only once; but there can be revalidations during the
    // lifetime of this object
    bool first_good_validation;

    // TODO: ca chain

    std::string initial_key;
    std::map<std::string, SbAdmin> admins;
    std::map<std::string, SbNode> nodes;
    std::map<SbNodeType, std::map<std::string, SbNode>> nodes_by_type;
    std::map<std::string, PubKeyEntity> blockchains;

    void do_add_index_data(SbRecord* rec);

    ISubclusterBlockchain::INotifier* notifier;

public:
    SubclusterBlockchain(std::string pub_key,
                         std::string ordering_node_endpoint,
                         std::string ordering_node_pub_key,
                         Poco::Path root_folder,
                         IGradidoFacade* gf);
    virtual ~SubclusterBlockchain() {}
    virtual void init();
public:
    virtual std::vector<std::string> get_logger_nodes();

    virtual void on_succ_append(ISubclusterBlockchain::INotifier* n);

    virtual std::vector<std::string> get_all_endpoints();


};

}

#endif
