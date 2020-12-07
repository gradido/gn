#ifndef GRADIDO_UTILS_H
#define GRADIDO_UTILS_H

#include <iostream>

// don't introduce dependencies to project infrastructure as much as
// possible
#include "blockchain_gradido_def.h"
#include "blockchain.h"


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

bool ends_with(std::string const & value, std::string const & ending);

void dump_in_hex(const char* in, char* out, size_t in_len);


template<typename T>
void dump_transaction_in_json(const typename BlockchainTypes<T>::Record& r, std::ostream& out) {

    typename BlockchainTypes<T>::RecordType rt = (typename BlockchainTypes<T>::RecordType)r.type;
    switch (rt) {
    case BlockchainTypes<T>::RecordType::EMPTY:
        break;
    case BlockchainTypes<T>::RecordType::PAYLOAD:
        dump_transaction_in_json(r.payload, out);
        break;
    case BlockchainTypes<T>::RecordType::CHECKSUM: {
        char buff[BLOCKCHAIN_CHECKSUM_SIZE * 2 + 1];
        dump_in_hex((char*)r.checksum, buff, BLOCKCHAIN_CHECKSUM_SIZE);
        out << "\"" << std::string(buff) << "\"" << std::endl;
        break;
    }
    }
}

void dump_transaction_in_json(const GradidoRecord& t, std::ostream& out);
void dump_transaction_in_json(const GroupRegisterRecord& t, std::ostream& out);

 
}

#endif
