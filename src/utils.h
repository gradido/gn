#ifndef GRADIDO_UTILS_H
#define GRADIDO_UTILS_H

#include <iostream>
#include "gradido_core_utils.h"

// should depend on data structures only, not gradido_interfaces.h
#include "blockchain_gradido_def.h"
#include "blockchain.h"

#define KEY_LENGTH_HEX PUB_KEY_LENGTH * 2
#if PUB_KEY_LENGTH != PRIV_KEY_LENGTH
#error check usages of priv key
#endif

#define KP_IDENTITY_PRIV_NAME "identity-priv.conf"
#define KP_IDENTITY_PUB_NAME "identity-pub.conf"

namespace gradido {

std::string get_as_str(SbNodeType r);



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
