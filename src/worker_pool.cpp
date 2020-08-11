#include "worker_pool.h"

namespace gradido {

    void* WorkerPool::run_entry(void* arg) {
        WorkerPool* wp = (WorkerPool*)arg;
        while (1) {
            ITask* task = 0;
            {
                MLock lock(wp->main_lock);
                while (wp->queue.size() == 0 && !wp->shutdown) 
                    pthread_cond_wait(&wp->queue_cond, &wp->main_lock);
                if (wp->shutdown) {
                    pthread_mutex_unlock(&wp->main_lock);
                    break;
                }
                task = wp->queue.front();
                wp->queue.pop();
            }
            task->run();
            delete task;
        }
        
    }

    WorkerPool::WorkerPool() : shutdown(false) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
        SAFE_PT(pthread_cond_init(&queue_cond, 0));
    }
    void WorkerPool::init(int worker_count) {
        for (int i = 0; i < worker_count; i++) {
            pthread_t t;
            SAFE_PT(pthread_create(&t, 0, &run_entry, (void*)this));
            workers.push_back(t);
        }
    }
    WorkerPool::~WorkerPool() {
        {
            MLock lock(main_lock);
            shutdown = true;
            pthread_cond_broadcast(&queue_cond);
        }
        for (auto i : workers)
            pthread_join(i, 0);
        while (queue.size() > 0) {
            ITask* task = queue.front();
            queue.pop();
            delete task;
        }
        pthread_cond_destroy(&queue_cond);
        pthread_mutex_destroy(&main_lock);
    }
    void WorkerPool::push(ITask* task) {
        MLock lock(main_lock);
        if (!shutdown) {
            queue.push(task);
            pthread_cond_signal(&queue_cond);
        }
    }

    size_t WorkerPool::get_worker_count() {
        return workers.size();
    }

    void WorkerPool::join() {
        for (auto i : workers)
            pthread_join(i, 0);
    }
    
}

