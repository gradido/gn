#ifndef GROUP_REGISTER_H
#define GROUP_REGISTER_H

#include <Poco/Path.h>
#include <pthread.h>
#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "blockchain.h"

namespace gradido {

class GroupRegister : public IGroupRegisterBlockchain,
    ICommunicationLayer::BlockChecksumReceiver,
    ICommunicationLayer::BlockRecordReceiver {
 public:
    virtual void on_block_record(grpr::BlockRecord br);
    virtual void on_block_checksum(grpr::BlockChecksum br);

 public:
    bool is_valid(const GroupRegisterRecord* rec, uint32_t count);
 private:
    // state can return to INITIAL if something goes wrong; this can
    // happen from any other state
    enum State {
        INITIAL=0,
        FETCHING_CHECKSUMS,
        FETCHING_BLOCKS,
        VALIDATING,
        READY
    };

    State state;
    IGradidoFacade* gf;
    HederaTopicID topic_id;
    typedef ValidatedMultipartBlockchain<GroupRegisterRecord, GroupRegister, GRADIDO_BLOCK_SIZE> StorageType; 

    std::string data_storage_root;
    StorageType storage;

    static std::string ensure_storage_root(const Poco::Path& sr);

    pthread_mutex_t main_lock;
    std::string fetch_endpoint;
    std::vector<std::string> fetched_checksums;
    uint64_t fetched_record_count;
    bool fetched_checksum_expecting_last;

    uint32_t block_to_fetch;
    uint64_t fetched_data_count;

    void fetch_next_block();
    void reset_validation();
    void validate_next_batch();
    bool add_index_data();

    // facade's continue_init_after_group_register_done() should be
    // called only once; but there can be revalidations during the
    // lifetime of this object
    bool first_good_validation;
    uint64_t indexed_blocks;
    std::map<std::string, GroupRecord> aliases;

 public:
    GroupRegister(Poco::Path root_folder,
                  IGradidoFacade* gf,
                  HederaTopicID topic_id);

    virtual void init();

    virtual void continue_validation();    
    virtual void continue_with_transactions();

    virtual uint64_t get_transaction_count();
    virtual void require_blocks(std::vector<std::string> endpoints);


    virtual void add_transaction(const GroupRecord& rec);
    virtual void add_multipart_transaction(
                 const GroupRegisterRecord* rec, 
                 size_t rec_count);

    virtual uint32_t get_block_count();

    virtual void get_checksums(std::vector<BlockInfo>& res);
    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::BlockRecord& res);

    virtual void add_record(std::string alias, HederaTopicID tid);
    virtual std::vector<GroupInfo> get_groups();
};

}

#endif
