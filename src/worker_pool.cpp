#include "worker_pool.h"
#include "gradido_interfaces.h"

namespace gradido {

    class SyncedTask : public ITask {
    private:
        ITask* task;
        bool finished_status;
        pthread_mutex_t main_lock;
        pthread_cond_t cond;
    public:
        SyncedTask(ITask* task) : task(task), finished_status(false) {
            SAFE_PT(pthread_mutex_init(&main_lock, 0));
            SAFE_PT(pthread_cond_init(&cond, 0));
        }
        virtual ~SyncedTask() {
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&main_lock);
        }
        virtual void run() {
            task->run();
            {
                MLock lock(main_lock);
                finished_status = true;
                pthread_cond_signal(&cond);
                while (finished_status) 
                    pthread_cond_wait(&cond, &main_lock);
            }
        }
        void join() {
            MLock lock(main_lock);
            while (!finished_status) 
                pthread_cond_wait(&cond, &main_lock);
            // signalling back to not allow premature destructor call
            finished_status = false; 
            pthread_cond_signal(&cond);
        }
    };

    void* WorkerPool::run_entry(void* arg) {
        WorkerPool* wp = (WorkerPool*)arg;
        while (1) {
            ITask* task = 0;
            {
                MLock lock(wp->main_lock);
                wp->busy_workers--;
                while (wp->queue.size() == 0 && !wp->shutdown) 
                    pthread_cond_wait(&wp->queue_cond, &wp->main_lock);
                if (wp->shutdown) {
                    pthread_mutex_unlock(&wp->main_lock);
                    break;
                }
                task = wp->queue.front();
                wp->queue.pop();
                wp->busy_workers++;
            }
            try {
                task->run();
            } catch (std::exception& e) {
                std::string msg = "error in worker thread: " + 
                    std::string(e.what());
                LOG(msg);
                wp->gf->exit(1);
            } catch (...) {
                LOG("unknown error");
                wp->gf->exit(1);
            }
            delete task;
        }
    }

    WorkerPool::WorkerPool(IGradidoFacade* gf, std::string name) : 
        gf(gf), name(name), shutdown(false), busy_workers(0) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
        SAFE_PT(pthread_cond_init(&queue_cond, 0));
    }
    WorkerPool::WorkerPool(IGradidoFacade* gf) : 
        gf(gf), name("unknown"), shutdown(false), busy_workers(0) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
        SAFE_PT(pthread_cond_init(&queue_cond, 0));
    }
    void WorkerPool::init(int worker_count) {
        MLock lock(main_lock);
        busy_workers = worker_count;
        for (int i = 0; i < worker_count; i++) {
            pthread_t t;
            SAFE_PT(pthread_create(&t, 0, &run_entry, (void*)this));
            LOG("worker_pool " + name + " started thread #" + std::to_string((uint64_t)t));
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

        {
            MLock lock(main_lock);
            workers.clear();
        }

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
        } else {
            delete task;
        }
    }

    void WorkerPool::push_and_join(ITask* task) {
        SyncedTask* task2 = 0;
        {
            MLock lock(main_lock);
            if (!shutdown) {
                task2 = new SyncedTask(task);
                queue.push(task2);
                pthread_cond_signal(&queue_cond);
            }
        }
        if (task2)
            task2->join();
        else
            delete task;
    }


    size_t WorkerPool::get_worker_count() {
        MLock lock(main_lock);
        return workers.size();
    }

    void WorkerPool::join() {
        for (auto i : workers)
            pthread_join(i, 0);
        {
            MLock lock(main_lock);
            workers.clear();
        }
    }

    size_t WorkerPool::get_task_queue_size() {
        MLock lock(main_lock);
        return queue.size();
    }

    size_t WorkerPool::get_busy_worker_count() {
        MLock lock(main_lock);
        return busy_workers;
    }

    void WorkerPool::init_shutdown() {
        MLock lock(main_lock);
        shutdown = true;
        pthread_cond_broadcast(&queue_cond);
    }
    
}

