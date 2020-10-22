#include "config.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/DirectoryIterator.h>
#include <fstream>
#include <Poco/TemporaryFile.h>
#include "utils.h"

namespace gradido {

    Config::Config() {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }
    Config::~Config() {
        pthread_mutex_destroy(&main_lock);
    }

    
    std::vector<GroupInfo> Config::get_group_info_list() {
        return gis;
    }

    int Config::get_worker_count() {
        return pfc->getInt("worker_count");
    }

    int Config::get_io_worker_count() {
        return pfc->getInt("io_worker_count");
    }
    
    void Config::init(const std::vector<std::string>& params) {
        try {
            std::string conf_file = "gradido.conf";
            if (params.size() > 1)
                conf_file = params[1];
            pfc = new Poco::Util::PropertyFileConfiguration(conf_file);
        } catch (Poco::Exception& e) {
            throw std::runtime_error("Couldn't open configuration file: " + std::string(e.what()));
        }

        std::string grti = pfc->getString("group_register_topic_id");

        int res = sscanf(grti, "%d.%d.%d", 
                         &group_register_topic_id.shardNum,
                         &group_register_topic_id.realmNum,
                         &group_register_topic_id.accountNum);
        if (res == EOF)
            throw std::runtime_error("config: bad group register topic id");
            
        reload_sibling_file();
        reload_group_infos();
    }

    int Config::get_blockchain_init_batch_size() {
        return pfc->getInt("blochchain_init_batch_size");
    }
    
    std::string Config::get_data_root_folder() {
        return pfc->getString("data_root_folder");
    }

    std::string Config::get_hedera_mirror_endpoint() {
        return pfc->getString("hedera_mirror_endpoint");
    }

    int Config::get_blockchain_append_batch_size() {
        return pfc->getInt("blockchain_append_batch_size");
    }

    int Config::get_block_record_outbound_batch_size() {
        return pfc->getInt("block_record_outbound_batch_size");
    }

    void Config::add_blockchain(GroupInfo gi) {
        MLock lock(main_lock);
        Poco::Path data_root(get_data_root_folder());
        Poco::Path bp = data_root.append(gi.get_directory_name());
        Poco::File bf(bp);
        bf.createDirectory();
        gis.push_back(gi);
    }

    bool Config::add_sibling_node(std::string endpoint) {
        MLock lock(main_lock);
        for (auto i : siblings)
            if (i.compare(endpoint) == 0)
                return false;
        siblings.push_back(endpoint);
        write_siblings();
        return true;
    }
    
    bool Config::remove_sibling_node(std::string endpoint) {
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

    std::vector<std::string> Config::get_sibling_nodes() {
        MLock lock(main_lock);
        return siblings;
    }

    void Config::write_siblings() {
        try {
            Poco::TemporaryFile tmp;
            std::string new_sibling_file = tmp.path();
            std::ofstream out(new_sibling_file);
            for (auto i : siblings)
                out << i << std::endl;
            out.close();
            std::string sibling_file = pfc->getString("sibling_node_file");
            tmp.moveTo(sibling_file);
        } catch (std::exception& e) {
            throw std::runtime_error("Couldn't save sibling file: " + std::string(e.what()));
        }
    }
    std::string Config::get_group_requests_endpoint() {
        return pfc->getString("group_requests_endpoint");
    }
    std::string Config::get_record_requests_endpoint() {
        return pfc->getString("record_requests_endpoint");
    }
    std::string Config::get_manage_network_requests_endpoint() {
        return pfc->getString("manage_network_requests_endpoint");
    }

    void Config::reload_sibling_file() {
        siblings.clear();
        try {
            std::string sibling_file = pfc->getString("sibling_node_file");
            std::ifstream in(sibling_file);
            std::string str;
            while (std::getline(in, str)) {
                if (str.size() > 0)
                    siblings.push_back(str);
            }            
            in.close();
        } catch (std::exception& e) {
            throw std::runtime_error("Couldn't open sibling file: " + std::string(e.what()));
        }
    }

    void Config::reload_group_infos() {
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
                        GroupInfo gi = {0};
                        if (ss[1].size() >= GROUP_ALIAS_LENGTH - 1)
                            throw std::runtime_error("blockchain name too long: " + ss[1]);
                        memcpy(gi.alias, ss[1].c_str(), ss[1].size());
                        gi.topic_id.shardNum = static_cast<uint64_t>(std::stoul(ss[2]));
                        gi.topic_id.realmNum = static_cast<uint64_t>(std::stoul(ss[3]));
                        gi.topic_id.topicNum = static_cast<uint64_t>(std::stoul(ss[4]));
                        gis.push_back(gi);
                    }
                }
            }
        } catch (std::exception& e) {
            throw std::runtime_error("Couldn't init blockchain groups: " + std::string(e.what()));
        }
    }

    HederaTopicID Config::get_group_register_topic_id() {
        return group_register_topic_id;
    }

    
}
