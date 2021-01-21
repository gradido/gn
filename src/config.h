#ifndef GRADIDO_CONFIG_H
#define GRADIDO_CONFIG_H

#include <pthread.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include "gradido_interfaces.h"
#include "utils.h"


namespace gradido {

    class EmptyConfig : public IGradidoConfig {
        virtual void init(const std::vector<std::string>& params) {NOT_SUPPORTED;}
        virtual std::vector<GroupInfo> get_group_info_list() {NOT_SUPPORTED;}
        virtual int get_worker_count() {NOT_SUPPORTED;}
        virtual int get_io_worker_count() {NOT_SUPPORTED;}
        virtual std::string get_data_root_folder() {NOT_SUPPORTED;}
        virtual std::string get_hedera_mirror_endpoint() {NOT_SUPPORTED;}
        virtual int get_blockchain_append_batch_size() {NOT_SUPPORTED;}
        virtual void add_blockchain(GroupInfo gi) {NOT_SUPPORTED;}
        virtual std::string get_sibling_node_file() {NOT_SUPPORTED;}
        virtual bool add_sibling_node(std::string endpoint) {NOT_SUPPORTED;}
        virtual bool remove_sibling_node(std::string endpoint) {NOT_SUPPORTED;}
        virtual std::vector<std::string> get_sibling_nodes() {NOT_SUPPORTED;}
        virtual std::string get_grpc_endpoint() {NOT_SUPPORTED;}
        virtual void reload_sibling_file() {NOT_SUPPORTED;}
        virtual void reload_group_infos() {NOT_SUPPORTED;}
        virtual std::string get_group_register_topic_id_as_str() {NOT_SUPPORTED;}
        virtual HederaTopicID get_group_register_topic_id() {NOT_SUPPORTED;}
        virtual bool is_topic_reset_allowed() {NOT_SUPPORTED;}
        virtual int get_json_rpc_port() {NOT_SUPPORTED;}
        virtual int get_general_batch_size() {NOT_SUPPORTED;}
        virtual std::string get_sb_ordering_node_endpoint() {NOT_SUPPORTED;}
        virtual std::string get_sb_pub_key() {NOT_SUPPORTED;}
        virtual bool is_sb_host() {NOT_SUPPORTED;}
        virtual std::string get_sb_ordering_node_pub_key() {NOT_SUPPORTED;}
        virtual bool launch_token_is_present() {NOT_SUPPORTED;}
        virtual std::string launch_token_get_admin_key() {NOT_SUPPORTED;}

        virtual bool kp_identity_has() {NOT_SUPPORTED;}
        virtual std::string kp_get_priv_key() {NOT_SUPPORTED;}
        virtual std::string kp_get_pub_key() {NOT_SUPPORTED;}
        virtual void kp_store(std::string priv_key, std::string pub_key) {NOT_SUPPORTED;}
        virtual std::string get_launch_node_endpoint() {NOT_SUPPORTED;}
    };

    class FileConfig : public EmptyConfig {
    private:
        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pfc;
        const std::string config_file_name = "gradido.conf";
    public:
        virtual void init(const std::vector<std::string>& params);
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual std::string get_data_root_folder();
        virtual std::string get_hedera_mirror_endpoint();
        virtual int get_blockchain_append_batch_size();
        virtual std::string get_sibling_node_file();
        virtual std::string get_grpc_endpoint();
        virtual std::string get_group_register_topic_id_as_str();
        virtual bool is_topic_reset_allowed();
        virtual int get_json_rpc_port();
        virtual int get_general_batch_size();
        virtual std::string get_sb_ordering_node_endpoint();
        virtual std::string get_sb_pub_key();
        virtual bool is_sb_host();
        virtual std::string get_sb_ordering_node_pub_key();
    };

    class GradidoNodeConfigDeprecated : public EmptyConfig {
    private:
        FileConfig fc;
        std::vector<GroupInfo> gis;
        std::vector<std::string> siblings;
        
        void write_siblings();
        pthread_mutex_t main_lock;
        HederaTopicID group_register_topic_id;
    public:
        GradidoNodeConfigDeprecated();
        virtual ~GradidoNodeConfigDeprecated();
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

    class LaunchToken : public EmptyConfig {
    private:
        const std::string launch_token_name = "launch-token.conf";
        bool present;
        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pfc;
    public:
        LaunchToken() : present(false) {};
        virtual void init(const std::vector<std::string>& params);
        virtual bool launch_token_is_present();
        virtual std::string launch_token_get_admin_key();
    };

    class KpIdentity : public EmptyConfig {
    private:
        const std::string identity_priv_name = KP_IDENTITY_PRIV_NAME;
        const std::string identity_pub_name = KP_IDENTITY_PUB_NAME;
        bool present = false;
        std::string priv_key;
        std::string pub_key;
    public:
        virtual void init(const std::vector<std::string>& params);
        virtual bool kp_identity_has();
        virtual std::string kp_get_priv_key();
        virtual std::string kp_get_pub_key();
        virtual void kp_store(std::string priv_key, std::string pub_key);
    };

    class AbstractNodeConfig : public EmptyConfig {
    protected:
        LaunchToken lt;
        KpIdentity kp;
        FileConfig fc;
    public:
        virtual void init(const std::vector<std::string>& params);

        virtual bool launch_token_is_present();
        virtual std::string launch_token_get_admin_key();

        virtual bool kp_identity_has();
        virtual std::string kp_get_priv_key();
        virtual std::string kp_get_pub_key();
        virtual void kp_store(std::string priv_key, std::string pub_key);

        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual std::string get_data_root_folder();
        virtual int get_blockchain_append_batch_size();
        virtual std::string get_grpc_endpoint();
        virtual int get_json_rpc_port();
        virtual int get_general_batch_size();
        virtual std::string get_sb_ordering_node_endpoint();
        virtual std::string get_sb_pub_key();
        virtual bool is_sb_host();
        virtual std::string get_sb_ordering_node_pub_key();
    };

    class LoggerConfig : public AbstractNodeConfig {
    public:
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual int get_json_rpc_port();
    };

    class OrderingConfig : public AbstractNodeConfig {
    public:
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual int get_json_rpc_port();
    };

    class PingerConfig : public AbstractNodeConfig {
    public:
        virtual int get_json_rpc_port();
    };

    class NodeLauncherConfig : public EmptyConfig {
    private:
        std::string endpoint;
        std::string own_endpoint;
        KpIdentity kp;
    public:
        virtual void init(const std::vector<std::string>& params);
        virtual int get_worker_count();
        virtual int get_io_worker_count();
        virtual int get_json_rpc_port();
        virtual std::string get_launch_node_endpoint();

        virtual std::string kp_get_priv_key();
        virtual std::string kp_get_pub_key();
        virtual std::string get_grpc_endpoint();
    };

    class AdminEnquiry {
    private:
        bool has_data;
        Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pfc;
    public:
        AdminEnquiry();
        bool has();
        std::string get_pub_key();
        std::string get_name();
        std::string get_email();
        std::string get_signature();
    };

}

#endif
