#ifndef GRADIDO_CONFIG_H
#define GRADIDO_CONFIG_H

#include "gradido_interfaces.h"

namespace gradido {

    class Config : public IGradidoConfig {

    public:
        virtual void init();
        virtual std::vector<GroupInfo> get_group_info_list();
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual int get_number_of_concurrent_blockchain_initializations();
        virtual std::string get_data_root_folder();
        virtual std::string get_hedera_mirror_endpoint();
        virtual int blockchain_append_batch_size();

    };

}

#endif
