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

    AdminEnquiry ae;
    if (!ae.has()) {
        LOG("no admin-enquiry.conf file in current folder");
        return 4;
    }

    std::string signature;

    // TODO: check if admin already doesn't exist
    // TODO: check if all is in bounds, and primary ordering node is
    // not yet added to sb

    try {
        LOG("adding admin");

        std::string pub_key = ae.get_pub_key();
        std::string name = ae.get_name();
        std::string email = ae.get_email();
        std::string admin_signature = ae.get_signature();

        Poco::Path root_folder(Poco::Path::current());
        std::string storage_root = ensure_blockchain_folder(root_folder, 
                                                            kpi.kp_get_pub_key());

        Blockchain<SbRecord, GRADIDO_BLOCK_SIZE> blockchain(kpi.kp_get_pub_key(), 
                                                            storage_root,
                                                            100,
                                                            100);
        Blockchain<SbRecord, GRADIDO_BLOCK_SIZE>::ExitCode ec;
        // TODO: could be more than one block, should handle this
        blockchain.validate_next_checksum(ec);
        if (ec != 
            Blockchain<SbRecord, GRADIDO_BLOCK_SIZE>::ExitCode::OK) {
            LOG("sb blockchain cannot be validated, ec " <<
                (int)ec);
            return 1;
        }

        do {
            {
                SbRecord rec;
                // TODO: move this to versioned
                rec.record_type = (uint8_t)SbRecordType::SB_ADD_ADMIN;
                rec.version_number = 1;
                rec.signature_count = 2;

                char pk[PUB_KEY_LENGTH];
                std::vector<char> cc = hex_to_bytes(pub_key);
                for (int i = 0; i < cc.size(); i++)
                    pk[i] = cc[i];
                memcpy(rec.admin.pub_key, pk, PUB_KEY_LENGTH);
                memcpy(rec.admin.name, name.c_str(), name.length());
                memcpy(rec.admin.email, email.c_str(), email.length());

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

                cp = hex_to_bytes(pub_key);
                cs = hex_to_bytes(admin_signature);
                for (int i = 0; i < PUB_KEY_LENGTH; i++)
                    rec.signatures[1].pubkey[i] = cp[i];
                for (int i = 0; i < SIGNATURE_LENGTH; i++)
                    rec.signatures[1].signature[i] = cs[i];

                blockchain.append(rec, ec);
                if ((uint8_t)ec != 0)
                    break;
            }
        } while (0);

        if ((uint8_t)ec == 0) {
            LOG("admin added");
        } else {
            LOG("admin wasn't added, ec: " << (uint8_t)ec);
            return 5;
        }
    } catch (std::runtime_error& e) {
        LOG("admin wasn't added: " << e.what());
        return 6;
    }
    return 0;
}

