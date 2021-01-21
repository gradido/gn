#include "gradido_interfaces.h"
#include <time.h>

#define QUOTES2(x) #x
#define QUOTES(x) QUOTES2(x)

namespace gradido {

pthread_mutex_t gradido_logger_lock;

std::string sanitize_for_log(std::string s) {
    std::replace(s.begin(), s.end(), (char)0, ' ');
    std::replace(s.begin(), s.end(), '\n', ' ');
    if (s.length() == 0)
        s = " "; // empty string could be interpreted as end-of-stream,
                 // if followed by a newline (could be added by log 
                 // function); preventing it here
    // TODO: escape all special characters
    return s;
}

std::string get_gradido_version_string() {
    return "v" + std::to_string(GRADIDO_VERSION_MAJOR) + "." + 
        std::to_string(GRADIDO_VERSION_MINOR) + ":git_hash=" + 
        QUOTES(GIT_HASH_VERSION);
}


// TODO: quiet start optional
bool init_logging() {
    SAFE_PT(pthread_mutex_init(&gradido_logger_lock, 0));
    LOG("gradido base " + get_gradido_version_string());
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

proto::Timestamp get_current_time() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    proto::Timestamp res;
    res.set_seconds(t.tv_sec);
    res.set_nanos(t.tv_nsec);
    return res;
}


    
}
