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

typedef Blockchain<GradidoRecord, GRADIDO_BLOCK_SIZE> GradidoBlockchainType;
typedef GradidoBlockchainType::Record GradidoBlockRec;


typedef Blockchain<GroupRegisterRecord, GRADIDO_BLOCK_SIZE> GradidoGroupBlockchainType;
typedef GradidoGroupBlockchainType::Record GradidoGroupBlockRec;

void dump_in_hex(const char* in, char* out, size_t in_len);


template<typename BType>
void dump_transaction_in_json(const typename BType::Record& r, std::ostream& out) {

    typename BType::RecordType rt = (typename BType::RecordType)r.type;
    switch (rt) {
    case BType::RecordType::EMPTY:
        break;
    case BType::RecordType::PAYLOAD:
        dump_transaction_in_json(r.payload, out);
        break;
    case BType::RecordType::CHECKSUM: {
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
