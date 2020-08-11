#ifndef GRADIDO_FACADE_H
#define GRADIDO_FACADE_H

#include "gradido_interfaces.h"
#include <stack>
#include <pthread.h>
#include "config.h"
#include "worker_pool.h"
#include "communications.h"
#include <map>

namespace gradido {

class GradidoFacade : public IGradidoFacade {
 private:
    std::stack<IBlockchain*> uninitialized_gis;
    pthread_mutex_t main_lock;
    Config config;
    WorkerPool worker_pool;
    CommunicationLayer communication_layer;
    std::map<uint64_t, IBlockchain*> blockchains;
 public:
    GradidoFacade();
    virtual ~GradidoFacade();
    virtual void init();
    virtual void join();
    virtual IBlockchain* pop_to_initialize_blockchain();
    virtual IBlockchain* get_group_blockchain(uint64_t group_id);
    virtual void push_task(ITask* task);
    virtual IGradidoConfig* get_conf();
};
 
}

#endif
