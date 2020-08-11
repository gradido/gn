#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <pthread.h>
#include <queue>
#include <stdexcept>
#include <stack>
#include "gradido_interfaces.h"
#include "utils.h"

namespace gradido {


class WorkerPool final {
private:
    std::queue<ITask*> queue;
    std::vector<pthread_t> workers;    
    pthread_mutex_t main_lock;
    pthread_cond_t queue_cond;

    static void* run_entry(void* arg);
    bool shutdown;
    
public:
    WorkerPool();
    void init(int worker_count);
    virtual ~WorkerPool();
    void push(ITask* task);
    size_t get_worker_count();
    void join();

};
    
    
}

#endif
