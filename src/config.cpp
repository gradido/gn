#include "config.h"

namespace gradido {
    
    std::vector<GroupInfo> Config::get_group_info_list() {
        // for now hardcoded
        GroupInfo gi;
        gi.group_id = 1;
        gi.topic_id.shardNum = 0;
        gi.topic_id.realmNum = 0;
        gi.topic_id.topicNum = 113344;
        std::vector<GroupInfo> res;
        res.push_back(gi);
        return res;
    }

    int Config::get_worker_count() {
        return 10;
    }

    int Config::get_io_worker_count() {
        return 1;
    }
    
    void Config::init() {
    }

    int Config::get_number_of_concurrent_blockchain_initializations() {
        return 5;
    }
    
    std::string Config::get_data_root_folder() {
        return "/tmp/gradido-test-root";
    }

    std::string Config::get_hedera_mirror_endpoint() {
        return "hcs.testnet.mirrornode.hedera.com:5600";
    }

    int Config::blockchain_append_batch_size() {
        return 10;
    }
    
}
