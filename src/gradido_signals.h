#ifndef GRADIDO_SIGNALS_H
#define GRADIDO_SIGNALS_H

#include <signal.h>
#include <pthread.h>
#include "gradido_interfaces.h"
#include "gradido_events.h"

namespace gradido {

// TODO: should allow turning on / off extensive logging on a working
// node

class GradidoSignals {
private:
    static GradidoSignals* signals;

    pthread_t worker;
    pthread_mutex_t main_lock;
    pthread_cond_t queue_cond;
    bool shutdown;
    IGradidoFacade* facade;
    volatile ReloadConfigTask* reload_config_task;

    static void* run_entry(void* arg);
    int last_signal;

public:
    static void init(IGradidoFacade* facade);
    static GradidoSignals* get_signals();
    static void sig_handler(int signum);
    GradidoSignals(IGradidoFacade* facade);
    void notify(int signum);
    void do_shutdown();
};
}

#endif
