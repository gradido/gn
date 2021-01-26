#include <string>
#include <iostream>
#include "utils.h"
#include "config.h"
#include "main_def.h"

using namespace gradido;                            
                                                    
int main(int argc, char** argv) {
    GRADIDO_CMD_UTIL;

    if (params.size() != 3) {
        LOG("parameters: name email");
        return 1;
    }
    std::string name = params[1];
    std::string email = params[2];

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

    LOG("creating admin enquiry");

    std::string mat = name + email + kpi.kp_get_pub_key();
    std::string signature = sign(mat, kpi.kp_get_pub_key(),
                                 kpi.kp_get_priv_key());
    
    std::ofstream ofs("admin-enquiry.conf", std::ofstream::trunc);
    ofs << "name = " << name << std::endl;
    ofs << "email = " << email << std::endl;
    ofs << "pub_key = " << kpi.kp_get_pub_key() << std::endl;
    ofs << "signature = " << signature << std::endl;
    ofs.close();
    LOG("enquiry created");

    return 0;
}

