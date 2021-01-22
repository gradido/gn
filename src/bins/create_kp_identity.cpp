#include <string>
#include <iostream>
#include "utils.h"
#include "main_def.h"

using namespace gradido;                            
                                                    
int main(int argc, char** argv) {
    GRADIDO_CMD_UTIL;
    const std::string err_msg = "couldn't create key pair identity";
    std::string priv;
    std::string pub;
    if (create_kp_identity(priv, pub)) {
        try {
            save_key_to_file(priv, KP_IDENTITY_PRIV_NAME, true);
            save_key_to_file(pub, KP_IDENTITY_PUB_NAME, false);
        } catch (std::exception& e) {
            std::cerr << err_msg << ": " << e.what() << std::endl;
            return 1;
        }
        std::cerr << "keys created in " << KP_IDENTITY_PRIV_NAME << 
            " and " << KP_IDENTITY_PRIV_NAME << std::endl;
        return 0;
    } else {
        std::cerr << err_msg << std::endl;
    }
    return 2;
}

