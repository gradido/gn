#include <string>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include "utils.h"
#include "config.h"
#include "blockchain.h"
#include "blockchain_base.h"
#include "blockchain_gradido_def.h"
#include "main_def.h"

using namespace gradido;                            
                                                    
int main(int argc, char** argv) {
    GRADIDO_CMD_UTIL;
    if (params.size() != 2) {
        LOG("parameters: name");
        return 1;
    }
    std::string sb_name = params[1];

    KpIdentity kpi;
    std::vector<std::string> empty;
    try {
        kpi.init(empty);
    } catch (std::runtime_error& e) {
        LOG("cannot open key pair identity: " << e.what());
        return 2;
    }    

    if (!kpi.kp_identity_has()) {
        LOG("no key pair identity in current folder");
        return 3;
    }

    std::string signature;

    try {
        LOG("creating sb");

        std::string name = kpi.kp_get_pub_key();
        Poco::Path root_folder(Poco::Path::current());
        std::string storage_root = ensure_blockchain_folder(root_folder, 
                                                            name);

        Blockchain<SbRecord, GRADIDO_BLOCK_SIZE> blockchain(name, 
                                                            storage_root,
                                                            100,
                                                            100);
        Blockchain<SbRecord, GRADIDO_BLOCK_SIZE>::ExitCode ec;

        do {
            {
                SbRecord rec;
                rec.record_type = (uint8_t)SbRecordType::SB_HEADER;
                rec.version_number = 1;
                rec.signature_count = 1;

                // TODO: move this to versioned
                memcpy(rec.header.subcluster_name, sb_name.c_str(), 
                       sb_name.length());
                rec.header.initial_key_type = 
                    (uint8_t)SbInitialKeyType::SELF_SIGNED;
                char pk[PUB_KEY_LENGTH];
                std::vector<char> cc = hex_to_bytes(name);
                for (int i = 0; i < cc.size(); i++)
                    pk[i] = cc[i];
                memcpy(rec.header.pub_key, pk, PUB_KEY_LENGTH);
                rec.header.ca_pub_key_chain_length = 1;

                signature = sign((uint8_t*)&rec, sizeof(rec),
                                 kpi.kp_get_pub_key(),
                                 kpi.kp_get_priv_key());
                blockchain.append(rec, ec);
                if ((uint8_t)ec != 0)
                    break;
            }

            {
                SbRecord rec;
                rec.record_type = (uint8_t)SbRecordType::SIGNATURES;

                std::vector<char> cp = 
                    hex_to_bytes(kpi.kp_get_pub_key());
                std::vector<char> cs = 
                    hex_to_bytes(signature);
                for (int i = 0; i < PUB_KEY_LENGTH; i++)
                    rec.signatures[0].pubkey[i] = cp[i];
                for (int i = 0; i < SIGNATURE_LENGTH; i++)
                    rec.signatures[0].signature[i] = cs[i];
                blockchain.append(rec, ec);
                if ((uint8_t)ec != 0)
                    break;
            }
        } while (0);

        if ((uint8_t)ec == 0) {
            LOG("sb created");
        } else {
            LOG("sb wasn't created, ec: " << (uint8_t)ec);
            return 4;
        }
    } catch (std::runtime_error& e) {
        LOG("sb wasn't created: " << e.what());
        return 5;
    }
    return 0;
}

