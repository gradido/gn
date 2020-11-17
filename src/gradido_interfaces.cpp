#include "gradido_interfaces.h"
#include <time.h>

namespace gradido {

pthread_mutex_t gradido_logger_lock;

bool init_logging() {
    SAFE_PT(pthread_mutex_init(&gradido_logger_lock, 0));
    LOG("logger initialized");
    return true;
}

    // TODO: calc usual folder length to remove folder from __file__
    // TODO: infos, warnings, errors

static bool gradido_logger_lock_initiated = init_logging();

struct timespec get_now() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts;
}

static struct timespec offset = get_now();

void subtract(timespec& t0, const timespec& t1) {
    t0.tv_sec -= t1.tv_sec;
    t0.tv_nsec -= t1.tv_nsec;
    if (t0.tv_nsec < 0) {
        t0.tv_sec--;
        t0.tv_nsec += 1000000000;
    }
}

std::string get_time() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    subtract(t, offset);
    char buff[100];
    sprintf(buff, "%llu.%09llu", (long long unsigned int)t.tv_sec, 
            (long long unsigned int)t.tv_nsec);
    return std::string(buff);
}

    
}
