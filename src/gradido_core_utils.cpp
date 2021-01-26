#include "gradido_core_utils.h"
#include <time.h>
#include <string.h>

namespace gradido {

#define QUOTES2(x) #x
#define QUOTES(x) QUOTES2(x)

volatile bool gradido_strict_not_supported = true;
bool set_gradido_strict_not_supported(bool val) {
    gradido_strict_not_supported = val;
    return true;
}

pthread_mutex_t gradido_logger_lock;

inline char to_hex_4_bit(unsigned char c) {
    c = c & 0x0f;
    if (c < 10)
        return c + 48;
    else return c + 87;
}

std::string sanitize_for_log(std::string s) {
#define LOG_SANITIZE_BUFF_LEN 1024
    std::string res;
    char buff[LOG_SANITIZE_BUFF_LEN];
    int bp = 0;
    for (int i = 0; i < s.length(); i++) {
        unsigned char c = s[i];
        if (c == '\n' || c == '\t') {
            buff[bp++] = ' ';
        } else if (c < 32 || c >= 127) {
            buff[bp++] = '/';
            buff[bp++] = to_hex_4_bit(c >> 4);
            buff[bp++] = to_hex_4_bit(c);
        } else
            buff[bp++] = c;
        if (bp == LOG_SANITIZE_BUFF_LEN - 1) {
            buff[bp] = 0;
            bp = 0;
            res += std::string(buff);
        }
    }
    if (bp > 0) {
        buff[bp] = 0;
        res += std::string(buff);
    }

    if (res.length() == 0)
        res = " "; // empty string could be interpreted as end-of-stream,
                   // if followed by a newline (could be added by log 
                   // function); preventing it here
    return res;
}

std::string get_gradido_version_string() {
    return "v" + std::to_string(GRADIDO_VERSION_MAJOR) + "." + 
        std::to_string(GRADIDO_VERSION_MINOR) + ":git_hash=" + 
        QUOTES(GIT_HASH_VERSION);
}

bool volatile logging_include_line_header = false;
bool volatile logging_show_precise_throw_origin = false;

    // TODO: calc usual folder length to remove folder from __file__
    // TODO: infos, warnings, errors

bool init_logging(bool include_line_header, bool init_shows_version,
                  bool show_precise_throw_origin) {
    logging_include_line_header = include_line_header;
    logging_show_precise_throw_origin = show_precise_throw_origin;
    SAFE_PT(pthread_mutex_init(&gradido_logger_lock, 0));
    if (init_shows_version)
        LOG("gradido base " + get_gradido_version_string());
    return true;
}


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

std::vector<char> hex_to_bytes(const std::string& hex) {
    std::vector<char> bytes;
    bytes.reserve(hex.length() / 2);

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

// copied from libsodium: https://github.com/jedisct1/libsodium

char *
sodium_bin2hex(char* hex, const size_t hex_maxlen,
               const unsigned char *const bin, const size_t bin_len)
{
    size_t       i = (size_t) 0U;
    unsigned int x;
    int          b;
    int          c;

    if (bin_len >= SIZE_MAX / 2 || hex_maxlen <= bin_len * 2U) {
//        sodium_misuse(); /* LCOV_EXCL_LINE */
          return NULL;
    }
    while (i < bin_len) {
        c = bin[i] & 0xf;
        b = bin[i] >> 4;
        x = (unsigned char) (87U + c + (((c - 10U) >> 8) & ~38U)) << 8 |
            (unsigned char) (87U + b + (((b - 10U) >> 8) & ~38U));
        hex[i * 2U] = (char) x;
        x >>= 8;
        hex[i * 2U + 1U] = (char) x;
        i++;
    }
    hex[i * 2U] = 0U;

    return hex;
}

bool is_hex(std::string str) {
    for (int i = 0; i < str.length(); i++) {
        if (!((str[i] >= '0' && str[i] <= '9') ||
              (str[i] >= 'a' && str[i] <= 'f') ||
              (str[i] >= 'A' && str[i] <= 'F')))
            return false;
    }
    return true;
}

void dump_in_hex(const char* in, char* out, size_t in_len) {
    sodium_bin2hex(out, (in_len * 2)+1, (const unsigned char*)in, in_len);
}

void dump_in_hex(const char* in, std::string& out, size_t in_len) {
    int blen = in_len * 2 + 1;
    char buff[blen];
    memset(buff, 0, blen);
    dump_in_hex(in, buff, in_len);
    out = std::string(buff);
}


}
