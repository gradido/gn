#include "group_register.h"

namespace gradido {

    bool GroupRegister::is_valid(const GroupRegisterRecord* rec, 
                                 uint32_t count) {

    }

    void GroupRegister::init() {}

    void GroupRegister::continue_with_transactions() {}
    void GroupRegister::continue_validation() {}

    void GroupRegister::exec_once_validated(ITask* task) {}
    uint64_t GroupRegister::get_transaction_count() {}
    void GroupRegister::require_blocks(std::vector<std::string> endpoints) {}


    GroupRegister::GradidoGroupBlockchain(Poco::Path root_folder,
                                          IGradidoFacade* gf,
                                          HederaTopicID topic_id) {
    }

    void GroupRegister::add_transaction(const GroupRecord& rec) {
    }

    void GroupRegister::add_multipart_transaction(
                 const GroupRegisterRecord* rec, 
                 size_t rec_count) {
    }

}
