#ifndef GRADIDO_UTILS_H
#define GRADIDO_UTILS_H

#include <iostream>

// don't introduce dependencies to project infrastructure as much as
// possible
#include "blockchain_gradido_def.h"
#include "blockchain.h"

#define KEY_LENGTH_HEX PUB_KEY_LENGTH * 2
#if PUB_KEY_LENGTH != PRIV_KEY_LENGTH
#error check usages of priv key
#endif

#define KP_IDENTITY_PRIV_NAME "identity-priv.conf"
#define KP_IDENTITY_PUB_NAME "identity-pub.conf"

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
void dump_in_hex(const char* in, std::string& out, size_t in_len);

std::vector<char> hex_to_bytes(const std::string& hex);

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
void dump_transaction_in_json(const SbRecord& t, std::ostream& out);

std::string read_key_from_file(std::string file_name); 
void save_key_to_file(std::string key, std::string file_name, 
                      bool set_permissions); 

bool create_kp_identity(std::string& priv, std::string& pub);
int create_keypair(private_key_t *sk, public_key_t *pk);

std::string sign(std::string material, 
                 const std::string& pub_key,
                 const std::string& priv_key);
std::string sign(uint8_t* mat, 
                 uint32_t mat_len,
                 const std::string& pub_key,
                 const std::string& priv_key);

}

#endif
