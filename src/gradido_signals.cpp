#include "gradido_signals.h"
#include "utils.h"

namespace gradido {

    GradidoSignals* GradidoSignals::signals = 0;

    void GradidoSignals::init(IGradidoFacade* facade) {
        signals = new GradidoSignals(facade);
    }

    GradidoSignals* GradidoSignals::get_signals() {
        return signals;
    }

    void GradidoSignals::sig_handler(int signum) {
        GradidoSignals* s = GradidoSignals::get_signals();
        s->notify(signum);
    }

    GradidoSignals::GradidoSignals(IGradidoFacade* facade) : 
        shutdown(false), facade(facade), last_signal(-1) {

        SAFE_PT(pthread_mutex_init(&main_lock, 0));
        SAFE_PT(pthread_cond_init(&queue_cond, 0));
        SAFE_PT(pthread_create(&worker, 0, &run_entry, (void*)this));

        signal(SIGUSR1, sig_handler);
    }

    void GradidoSignals::notify(int signum) {
        MLock lock(main_lock);
        last_signal = signum;
        pthread_cond_broadcast(&queue_cond);
    }

    void GradidoSignals::do_shutdown() {
        MLock lock(main_lock);
        shutdown = true;
        pthread_cond_signal(&queue_cond);
    }

    void* GradidoSignals::run_entry(void* arg) {
        GradidoSignals* gs = (GradidoSignals*)arg;
        while (1) {
            MLock lock(gs->main_lock);
            while (gs->last_signal == -1 && !gs->shutdown) 
                pthread_cond_wait(&gs->queue_cond, &gs->main_lock);
            if (gs->shutdown)
                break;
            if (gs->last_signal != -1) {
                gs->last_signal = 0;
                gs->facade->push_task(new ReloadConfigTask(gs->facade));
            }
        }
    }


}
