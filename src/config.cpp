#include "config.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/DirectoryIterator.h>
#include <fstream>
#include <Poco/TemporaryFile.h>
#include "utils.h"

namespace gradido {

#define SAFE_GET_CONF_ITEM(func, key)                     \
    try {                                                 \
        return pfc->func(key);                            \
    } catch (Poco::NotFoundException& e) {                \
        LOG("config: missing " << key);                   \
        throw e;                                          \
    }

    int FileConfig::get_worker_count() {
        SAFE_GET_CONF_ITEM(getInt, "worker_count");
    }

    int FileConfig::get_io_worker_count() {
        SAFE_GET_CONF_ITEM(getInt, "io_worker_count");
    }
    
    void FileConfig::init(const std::vector<std::string>& params) {
        try {
            pfc = new Poco::Util::PropertyFileConfiguration(
                                  config_file_name);
        } catch (std::exception& e) {
            PRECISE_THROW("Couldn't open configuration file: " + std::string(e.what()));
        }
    }

    std::string FileConfig::get_data_root_folder() {
        SAFE_GET_CONF_ITEM(getString, "data_root_folder");
    }

    std::string FileConfig::get_hedera_mirror_endpoint() {
        SAFE_GET_CONF_ITEM(getString, "hedera_mirror_endpoint");
    }

    int FileConfig::get_blockchain_append_batch_size() {
        SAFE_GET_CONF_ITEM(getInt, "blockchain_append_batch_size");
    }

    std::string FileConfig::get_sibling_node_file() {
        SAFE_GET_CONF_ITEM(getString, "sibling_node_file");
    }

    std::string FileConfig::get_grpc_endpoint() {
        SAFE_GET_CONF_ITEM(getString, "grpc_endpoint");
    }

    std::string FileConfig::get_group_register_topic_id_as_str() {
        SAFE_GET_CONF_ITEM(getString, "group_register_topic_id");
    }

    bool FileConfig::is_topic_reset_allowed() {
        SAFE_GET_CONF_ITEM(getInt, "topic_reset_allowed");
    }

    int FileConfig::get_json_rpc_port() {
        SAFE_GET_CONF_ITEM(getInt, "json_rpc_port");
    }

    int FileConfig::get_general_batch_size() {
        SAFE_GET_CONF_ITEM(getInt, "general_batch_size");
    }

    std::string FileConfig::get_sb_ordering_node_endpoint() {
        SAFE_GET_CONF_ITEM(getString, "sb_ordering_node_endpoint");
    }

    std::string FileConfig::get_sb_pub_key() {
        SAFE_GET_CONF_ITEM(getString, "sb_pub_key");
    }
    bool FileConfig::is_sb_host() {
        SAFE_GET_CONF_ITEM(getInt, "is_sb_host");
    }
    std::string FileConfig::get_sb_ordering_node_pub_key() {
        SAFE_GET_CONF_ITEM(getString, "sb_ordering_node_pub_key");
    }

    void GradidoNodeConfigDeprecated::reload_group_infos() {
        try {
            Poco::File block_root(get_data_root_folder());
            Poco::RegularExpression re("^([a-zA-Z0-9_]+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.bc$");
            for (Poco::DirectoryIterator it(block_root);
                 it != Poco::DirectoryIterator{}; ++it) {
                Poco::File curr(it.path());
                if (curr.isDirectory()) {
                    std::string fname = it.path().getFileName();
                    std::vector<std::string> ss;
                    if (re.split(fname, ss)) {
                        GroupInfo gi;
                        if (ss[1].size() >= GROUP_ALIAS_LENGTH - 1)
                            PRECISE_THROW("blockchain name too long: " 
                                          << ss[1]);
                        memcpy(gi.alias, ss[1].c_str(), ss[1].size());
                        gi.topic_id.shardNum = static_cast<uint64_t>(std::stoul(ss[2]));
                        gi.topic_id.realmNum = static_cast<uint64_t>(std::stoul(ss[3]));
                        gi.topic_id.topicNum = static_cast<uint64_t>(std::stoul(ss[4]));
                        gis.push_back(gi);
                    }
                }
            }
        } catch (std::exception& e) {
            PRECISE_THROW("Couldn't init blockchain groups: " + std::string(e.what()));
        }
    }

    HederaTopicID GradidoNodeConfigDeprecated::get_group_register_topic_id() {
        return group_register_topic_id;
    }

    void GradidoNodeConfigDeprecated::reload_sibling_file() {
        siblings.clear();
        try {
            std::string sibling_file = fc.get_sibling_node_file();
            std::ifstream in(sibling_file);
            std::string str;
            while (std::getline(in, str)) {
                if (str.size() > 0)
                    siblings.push_back(str);
            }            
            in.close();
        } catch (std::exception& e) {
            PRECISE_THROW("Couldn't open sibling file: " + std::string(e.what()));
        }
    }

    bool GradidoNodeConfigDeprecated::add_sibling_node(std::string endpoint) {
        MLock lock(main_lock);
        for (auto i : siblings)
            if (i.compare(endpoint) == 0)
                return false;
        siblings.push_back(endpoint);
        write_siblings();
        return true;
    }
    
    bool GradidoNodeConfigDeprecated::remove_sibling_node(std::string endpoint) {
        MLock lock(main_lock);
        std::vector<std::string> new_siblings;
        bool res = false;
        for (auto i : siblings)
            if (i.compare(endpoint) == 0)
                res = true;
            else
                new_siblings.push_back(i);
        if (res)
            write_siblings();
        return res;
    }

    std::vector<std::string> GradidoNodeConfigDeprecated::get_sibling_nodes() {
        MLock lock(main_lock);
        return siblings;
    }

    void GradidoNodeConfigDeprecated::write_siblings() {
        try {
            Poco::TemporaryFile tmp;
            std::string new_sibling_file = tmp.path();
            std::ofstream out(new_sibling_file);
            for (auto i : siblings)
                out << i << std::endl;
            out.close();
            std::string sibling_file = fc.get_sibling_node_file();
            tmp.moveTo(sibling_file);
        } catch (Poco::NotFoundException& e) {
            LOG("config: missing sibling_node_file");
            throw e;
        } catch (std::exception& e) {
            PRECISE_THROW("Couldn't save sibling file: " + std::string(e.what()));
        }
    }

    void GradidoNodeConfigDeprecated::add_blockchain(GroupInfo gi) {
        MLock lock(main_lock);
        Poco::Path data_root(get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());
        Poco::File bf(bp);
        bf.createDirectory();
        gis.push_back(gi);
    }

    void GradidoNodeConfigDeprecated::init(const std::vector<std::string>& params) {
        fc.init(params);
        std::string grti = fc.get_group_register_topic_id_as_str();
        int res = sscanf(grti.c_str(), 
                         "%ld.%ld.%ld",
                         &group_register_topic_id.shardNum,
                         &group_register_topic_id.realmNum,
                         &group_register_topic_id.topicNum);
        if (res == EOF)
            PRECISE_THROW("config: bad group register topic id");
            
        reload_sibling_file();
        reload_group_infos();
    }

    std::vector<GroupInfo> GradidoNodeConfigDeprecated::get_group_info_list() {
        return gis;
    }

    GradidoNodeConfigDeprecated::GradidoNodeConfigDeprecated() {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    GradidoNodeConfigDeprecated::~GradidoNodeConfigDeprecated() {
        pthread_mutex_destroy(&main_lock);
    }

    int GradidoNodeConfigDeprecated::get_worker_count() {
        return fc.get_worker_count();
    }

    int GradidoNodeConfigDeprecated::get_io_worker_count() {
        return fc.get_io_worker_count();
    }
    
    std::string GradidoNodeConfigDeprecated::get_data_root_folder() {
        return fc.get_data_root_folder();
    }

    std::string GradidoNodeConfigDeprecated::get_hedera_mirror_endpoint() {
        return fc.get_hedera_mirror_endpoint();
    }

    int GradidoNodeConfigDeprecated::get_blockchain_append_batch_size() {
        return fc.get_blockchain_append_batch_size();
    }

    std::string GradidoNodeConfigDeprecated::get_grpc_endpoint() {
        return fc.get_grpc_endpoint();
    }

    bool GradidoNodeConfigDeprecated::is_topic_reset_allowed() {
        return fc.is_topic_reset_allowed();
    }

    int GradidoNodeConfigDeprecated::get_json_rpc_port() {
        return fc.get_json_rpc_port();
    }

    int GradidoNodeConfigDeprecated::get_general_batch_size() {
        return fc.get_general_batch_size();
    }

    void LaunchToken::init(const std::vector<std::string>& params) {
        Poco::File ltf(launch_token_name);
        if (ltf.exists()) {
            try {
                pfc = new Poco::Util::PropertyFileConfiguration(launch_token_name);
                ltf.remove();
                present = true;
            } catch (std::exception& e) {
                PRECISE_THROW("Couldn't open launch token file: " + 
                                         std::string(e.what()));
            }
        }
    }

    bool LaunchToken::launch_token_is_present() {
        return present;
    }
    std::string LaunchToken::launch_token_get_admin_key() {
        SAFE_GET_CONF_ITEM(getString, "admin_key");
    }

    void KpIdentity::init(const std::vector<std::string>& params) {
        Poco::File pr(identity_priv_name);
        bool pr_present = false;
        if (pr.exists()) {
            try {
                priv_key = read_key_from_file(identity_priv_name);
                pr_present = true;
            } catch (std::exception& e) {
                PRECISE_THROW("Couldn't open priv key: " + 
                                         std::string(e.what()));
            }
        }

        Poco::File pu(identity_pub_name);
        bool pu_present = false;
        if (pu.exists()) {
            try {
                pub_key = read_key_from_file(identity_pub_name);
                pu_present = true;
            } catch (std::exception& e) {
                PRECISE_THROW("Couldn't open pub key: " + 
                                         std::string(e.what()));
            }
        }
        if (pr_present && !pu_present ||
            pu_present && !pr_present)
            PRECISE_THROW("one of kp identity files doesn't exist");

        present = pr_present && pu_present;
    }
    bool KpIdentity::kp_identity_has() {
        return present;
    }
    std::string KpIdentity::kp_get_priv_key() {
        return priv_key;
    }
    std::string KpIdentity::kp_get_pub_key() {
        return pub_key;
    }
    void KpIdentity::kp_store(std::string pr, std::string pu) {
        save_key_to_file(pr, identity_priv_name, true);
        save_key_to_file(pu, identity_pub_name, false);
        priv_key = pr;
        pub_key = pu;
        present = true;
    }

    bool AbstractNodeConfig::is_sb_host() {
        return fc.is_sb_host();
    }

    std::string AbstractNodeConfig::get_sb_ordering_node_pub_key() {
        return fc.get_sb_ordering_node_pub_key();
    }

    void AbstractNodeConfig::init(const std::vector<std::string>& params) {
        lt.init(params);
        kp.init(params);
        fc.init(params);
    }

    bool AbstractNodeConfig::launch_token_is_present() {
        return lt.launch_token_is_present();
    }
    std::string AbstractNodeConfig::launch_token_get_admin_key() {
        return lt.launch_token_get_admin_key();
    }

    bool AbstractNodeConfig::kp_identity_has() {
        return kp.kp_identity_has();
    }
    std::string AbstractNodeConfig::kp_get_priv_key() {
        return kp.kp_get_priv_key();
    }
    std::string AbstractNodeConfig::kp_get_pub_key() {
        return kp.kp_get_pub_key();
    }
    void AbstractNodeConfig::kp_store(std::string priv_key, std::string pub_key) {
        kp.kp_store(priv_key, pub_key);
    }

    int AbstractNodeConfig::get_worker_count() {
        return fc.get_worker_count();
    }
    int AbstractNodeConfig::get_io_worker_count() {
        return fc.get_io_worker_count();
    }
    std::string AbstractNodeConfig::get_data_root_folder() {
        return fc.get_data_root_folder();
    }
    int AbstractNodeConfig::get_blockchain_append_batch_size() {
        return fc.get_blockchain_append_batch_size();
    }
    std::string AbstractNodeConfig::get_grpc_endpoint() {
        return fc.get_grpc_endpoint();
    }
    int AbstractNodeConfig::get_json_rpc_port() {
        return fc.get_json_rpc_port();
    }
    int AbstractNodeConfig::get_general_batch_size() {
        return fc.get_general_batch_size();
    }
    std::string AbstractNodeConfig::get_sb_ordering_node_endpoint() {
        return fc.get_sb_ordering_node_endpoint();
    }

    std::string AbstractNodeConfig::get_sb_pub_key() {
        return fc.get_sb_pub_key();
    }

    int LoggerConfig::get_worker_count() {
        return 1;
    }
    int LoggerConfig::get_io_worker_count() {
        return 1;
    }
    int LoggerConfig::get_json_rpc_port() {
        return 0;
    }

    int OrderingConfig::get_worker_count() {
        // NOTE: very important to keep it like this
        return 1;
    }
    int OrderingConfig::get_io_worker_count() {
        return 1;
    }
    int OrderingConfig::get_json_rpc_port() {
        return 0;
    }

    int PingerConfig::get_json_rpc_port() {
        return 0;
    }

    int NodeLauncherConfig::get_worker_count() {
        return 1;
    }

    int NodeLauncherConfig::get_io_worker_count() {
        return 1;
    }

    int NodeLauncherConfig::get_json_rpc_port() {
        return 0;
    }

    void NodeLauncherConfig::init(
         const std::vector<std::string>& params) {
        kp.init(params);
        if (!kp.kp_identity_has())
            PRECISE_THROW("need key pair identity");
        if (params.size() > 1 && params[1].length() > 0 &&
            params[2].length() > 0) {
            own_endpoint = params[1];
            endpoint = params[2];
        } else
            PRECISE_THROW("required parameters: own_endpoint target_endpoint");
    }

    std::string NodeLauncherConfig::get_launch_node_endpoint() {
        return endpoint;
    }

    std::string NodeLauncherConfig::kp_get_priv_key() {
        return kp.kp_get_priv_key();
    }
    std::string NodeLauncherConfig::kp_get_pub_key() {
        return kp.kp_get_pub_key();
    }

    std::string NodeLauncherConfig::get_grpc_endpoint() {
        return own_endpoint;
    }

    AdminEnquiry::AdminEnquiry() : has_data(false) {
        Poco::File ae("admin-enquiry.conf");
        if (ae.exists()) {
            try {
                pfc = new Poco::Util::PropertyFileConfiguration(
                                      "admin-enquiry.conf");
                ae.remove();
                has_data = true;
            } catch (std::exception& e) {
                PRECISE_THROW("Couldn't open admin-enquiry.conf file: " + std::string(e.what()));
            }
        }
    }
    bool AdminEnquiry::has() {
        return has_data;
    }
    std::string AdminEnquiry::get_pub_key() {
        SAFE_GET_CONF_ITEM(getString, "pub_key");
    }
    std::string AdminEnquiry::get_name() {
        SAFE_GET_CONF_ITEM(getString, "name");
    }
    std::string AdminEnquiry::get_email() {
        SAFE_GET_CONF_ITEM(getString, "email");
    }
    std::string AdminEnquiry::get_signature() {
        SAFE_GET_CONF_ITEM(getString, "signature");
    }
    
}
