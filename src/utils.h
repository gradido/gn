#ifndef GRADIDO_UTILS_H
#define GRADIDO_UTILS_H

// don't introduce dependencies to project infrastructure
#include "blockchain_gradido_def.h"
#include <iostream>

namespace gradido {

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

void dump_transaction_in_json(const GradidoRecord& t, std::ostream& out);
void dump_in_hex(const char* in, char* out, size_t in_len);
 
}

#endif
