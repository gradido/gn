#ifndef GRADIDO_INTERFACES_H
#define GRADIDO_INTERFACES_H

#include <iostream>
#include <vector>
#include <unistd.h>
#include "blockchain_gradido_def.h"
#include <string>

// TODO: only for linux, to prevent warnings
#include <sys/sysmacros.h>

#define LOG(msg) \
    std::cerr << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl

#define SAFE_PT(expr) if (expr != 0) throw std::runtime_error("couldn't " #expr)


namespace gradido {

class ITask {
public:
    virtual ~ITask() {}
    virtual void run() = 0;
};

class IBlockchain {
 public:
    virtual ~IBlockchain() {}
    virtual void init() = 0;
    enum AddTransactionResult {
        DONE=0,              // no more transactions in queue
        TRY_AGAIN,           // can continue immediately
        TRY_AGAIN_LATER      // waiting for data, continue later
    };
    virtual AddTransactionResult add_transaction(const Transaction& tr) = 0;
    virtual AddTransactionResult continue_with_transactions() = 0;
};

struct GroupInfo {
    uint64_t group_id;
    HederaTopicID topic_id;
    std::string alias;  // used to name blockchain folder
};

class IGradidoConfig {
 public:
    virtual ~IGradidoConfig() {}
    virtual void init() = 0;
    virtual std::vector<GroupInfo> get_group_info_list() = 0;
    virtual int get_worker_count() = 0;
    virtual int get_io_worker_count() = 0;
    virtual int get_number_of_concurrent_blockchain_initializations() = 0;
    virtual std::string get_data_root_folder() = 0;
    virtual std::string get_hedera_mirror_endpoint() = 0;
    virtual int blockchain_append_batch_size() = 0;
};

class IGradidoFacade {
 public:
    virtual ~IGradidoFacade() {}
    virtual void init() = 0;
    virtual void join() = 0;
    virtual IBlockchain* pop_to_initialize_blockchain() = 0;
    virtual IBlockchain* get_group_blockchain(uint64_t group_id) = 0;
    virtual void push_task(ITask* task) = 0;
    virtual IGradidoConfig* get_conf() = 0;
};
 
}

#endif
