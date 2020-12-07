#ifndef GRADIDO_CONFIG_H
#define GRADIDO_CONFIG_H

#include "gradido_interfaces.h"
#include <Poco/Util/PropertyFileConfiguration.h>
#include <pthread.h>

namespace gradido {

    class Config : public IGradidoConfig {
    private:
        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pfc;
        std::vector<GroupInfo> gis;
        std::vector<std::string> siblings;
        
        void write_siblings();
        pthread_mutex_t main_lock;
        HederaTopicID group_register_topic_id;
    public:
        Config();
        virtual ~Config();
        virtual void init(const std::vector<std::string>& params);
        virtual std::vector<GroupInfo> get_group_info_list();
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual std::string get_data_root_folder();
        virtual std::string get_hedera_mirror_endpoint();
        virtual int get_blockchain_append_batch_size();
        virtual void add_blockchain(GroupInfo gi);
        virtual bool add_sibling_node(std::string endpoint);
        virtual bool remove_sibling_node(std::string endpoint);
        virtual std::vector<std::string> get_sibling_nodes();
        virtual std::string get_grpc_endpoint();
        virtual void reload_sibling_file();
        virtual void reload_group_infos();
        virtual HederaTopicID get_group_register_topic_id();
        virtual bool is_topic_reset_allowed();
        virtual int get_json_rpc_port();
        virtual int get_general_batch_size();
    };

}

#endif
