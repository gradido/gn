#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <pthread.h>
#include <queue>
#include <stdexcept>
#include <stack>
#include "gradido_interfaces.h"
#include "utils.h"

namespace gradido {

/*
  only acceptable call sequence is as follows:
  - constructor()
  - init(), single time
  - ...
  - join(), single time; this may be omitted
  - ...
  - destructor()
  
  any other scenario leads to undefined behaviour
  - calling join() while init() is not finished
  - calling join() multiple times
  - etc.

  this is to have things simple in implementation; also, expecting a
  "parent" thread doing initialization, which would allow things to
  happen in a certain sequence
*/

class WorkerPool final {
private:
    IGradidoFacade* gf;
    std::string name;
    std::queue<ITask*> queue;
    std::vector<pthread_t> workers;    
    pthread_mutex_t main_lock;
    pthread_cond_t queue_cond;

    static void* run_entry(void* arg);
    bool shutdown;
    size_t busy_workers;
public:
    WorkerPool(IGradidoFacade* gf);
    WorkerPool(IGradidoFacade* gf, std::string name);
    void init(int worker_count);
    virtual ~WorkerPool();

    // takes ownership of task
    void push(ITask* task);
    size_t get_worker_count();

    // only way for this method to exit once called is by calling
    // init_shutdown()
    void join();
    size_t get_task_queue_size();
    size_t get_busy_worker_count();

    // after this is called (more precisely, exited), push() won't have 
    // an effect
    void init_shutdown();
};
    
    
}

#endif
