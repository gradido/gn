#ifndef GROUP_REGISTER_H
#define GROUP_REGISTER_H

#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "blockchain.h"

namespace gradido {

class GroupRegister : public IGroupRegisterBlockchain {
 public:
    bool is_valid(const GroupRegisterRecord* rec, uint32_t count);
 private:
    Poco::Path root_folder;
    IGradidoFacade* gf;
    HederaTopicID topic_id;
    typedef ValidatedMultipartBlockchain<GroupRegisterRecord, GroupRegister, GRADIDO_BLOCK_SIZE> StorageType; 
    StorageType storage;
 public:
    GradidoGroupBlockchain(Poco::Path root_folder,
                           IGradidoFacade* gf,
                           HederaTopicID topic_id);

    virtual void init();

    virtual void continue_with_transactions();
    virtual void continue_validation();

    virtual void exec_once_validated(ITask* task);
    virtual uint64_t get_transaction_count();
    virtual void require_blocks(std::vector<std::string> endpoints);


    virtual void add_transaction(const GroupRecord& rec);
    virtual void add_multipart_transaction(
                 const GroupRegisterRecord* rec, 
                 size_t rec_count);

};

}

#endif
