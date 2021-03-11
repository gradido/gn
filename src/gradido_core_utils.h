#ifndef GRADIDO_CORE_UTILS_H
#define GRADIDO_CORE_UTILS_H

#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <vector>

namespace gradido {

// this header file is not dependent on any other gradido headers

#define GRADIDO_VERSION_MAJOR 0
#define GRADIDO_VERSION_MINOR 1

extern pthread_mutex_t gradido_logger_lock;
extern volatile bool logging_include_line_header;
extern volatile bool logging_show_precise_throw_origin;

std::string get_gradido_version_string();

// env variable; if set to anything longer than 0 before startup, will 
// cause extensive logging; it is a runtime flag, which can be changed 
// while node is running (TODO: with help of signals)
#define EXTENSIVE_LOGGING_ENABLED "EXTENSIVE_LOGGING_ENABLED"

extern volatile bool gradido_strict_not_supported;
#define NOT_SUPPORTED LOG("not supported"); if (gradido_strict_not_supported) throw std::runtime_error("not supported");
bool set_gradido_strict_not_supported(bool val);

template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

template<typename T>
T min(T a, T b) {
    return a < b ? a : b;
}

std::string get_time();

bool init_logging(bool include_line_header, bool init_shows_version,
                  bool show_precise_throw_origin);

std::string sanitize_for_log(std::string s);

// it is expected that all executable-generated messages are sent through
// LOG() calls; logging-related global variables can be adjusted to
// have different output format for tools and nodes, for example

// complexity is needed to avoid badly formatted logs; keep in one line
// to simplify debugging
#define LOG(msg) {std::stringstream _logger_stream; std::string _logger_string; try {_logger_stream << msg; _logger_string = sanitize_for_log(_logger_stream.str());} catch (std::exception& e) {_logger_string = std::string("log expression throws exception: ") + e.what(); } catch (...) {_logger_string = "log expression throws exception"; } pthread_mutex_lock(&gradido_logger_lock); if (logging_include_line_header) {std::cerr << __FILE__ << ":" << __LINE__ << ":#" << (uint64_t)pthread_self() << ":" << get_time() << ": ";} std::cerr << _logger_string << std::endl; pthread_mutex_unlock(&gradido_logger_lock);}


#define PRECISE_THROW2(msg, ecl) {if (logging_show_precise_throw_origin) LOG(msg); std::string msg2; try {std::stringstream precise_throw_ss; precise_throw_ss << msg; msg2 = precise_throw_ss.str();} catch (...) {msg2 = "bad msg";} throw ecl(msg2);}
#define PRECISE_THROW(msg) PRECISE_THROW2(msg, std::runtime_error)

#define SAFE_PT(expr) if (expr != 0) PRECISE_THROW("couldn't " << #expr)


#ifdef GN_MUTEX_LOG

class MLock final {
private:
    pthread_mutex_t& m;
    std::string desc;
public:
    // TODO: remove
    MLock(pthread_mutex_t& m) : m(m), desc("unknown") {
        LOG(desc << " " << &m << " locking");
        pthread_mutex_lock(&m);
        LOG(desc << " " << &m << " locked");
    }
    MLock(pthread_mutex_t& m, std::string desc) : m(m), desc(desc) {
        LOG(desc << " " << &m << " locking");
        pthread_mutex_lock(&m);
        LOG(desc << " " << &m << " locked");
    }
    ~MLock() {
        LOG(desc << " " << &m << " unlocking");
        pthread_mutex_unlock(&m);
        LOG(desc << " " << &m << " unlocked");
    }
};

#define MLOCK(m) MLock lock(m, std::string(__FILE__) + ":" + std::to_string(__LINE__));
#define MINIT(m) SAFE_PT(pthread_mutex_init(&m, 0)); LOG("mutex initialized " << &m);
#define MDESTROY(m) pthread_mutex_destroy(&m); LOG("mutex destroyed " << &m);

#else

 
class MLock final {
private:
    pthread_mutex_t& m;
public:
    MLock(pthread_mutex_t& m) : m(m) {
        pthread_mutex_lock(&m);
    }
    ~MLock() {
        pthread_mutex_unlock(&m);
    }
};

#define MLOCK(m) MLock lock(m);
#define MINIT(m) SAFE_PT(pthread_mutex_init(&m, 0));
#define MDESTROY(m) pthread_mutex_destroy(&m);


#endif


bool ends_with(std::string const & value, std::string const & ending);

bool is_hex(std::string str);
void dump_in_hex(const char* in, char* out, size_t in_len);
void dump_in_hex(const char* in, std::string& out, size_t in_len);
std::vector<char> hex_to_bytes(const std::string& hex);

// some structs are ment to be erased by zeroing them; NOTE: never
// use it on classes with virtual methods
// TODO: verify memset is not optimized out, so it can be used for
// security reasons as well
#define ZERO_THIS volatile void* p = memset(this, 0, sizeof(*this));

// to ensure compatibility across various platforms
// little endian is gradido standard of storing data
#define PACKED_STRUCT __attribute__((__packed__))


bool contains_null(std::string str);


}

#endif
