#include "test_common.h"
#include "facades_impl.h"

#define NODE_0_FOLDER "/tmp/test-system/node-0"
#define NODE_1_FOLDER "/tmp/test-system/node-1"

void prepare_system_folders(std::string folder, int endpoint_port,
                            std::vector<std::string> siblings) {
    erase_tmp_folder(folder);
    Poco::File bf(folder);
    bf.createDirectories();

    std::string sibling_file = folder + "/sibling_nodes.txt";

    {
        std::fstream file;
        file.open(folder + "/gradido.conf", std::ios::out);


        file << "worker_count = 10" << std::endl;
        file << "io_worker_count = 1" << std::endl;
        file << "data_root_folder = " << folder << std::endl;
        file << "hedera_mirror_endpoint = hcs.testnet.mirrornode.hedera.com:5600" << std::endl;
        file << "sibling_node_file = " + sibling_file << std::endl;
        file << "grpc_endpoint = 0.0.0.0:" << endpoint_port << std::endl;
        file << "blockchain_append_batch_size = 5" << std::endl;
        file << "blochchain_init_batch_size = 1000" << std::endl;
        file << "block_record_outbound_batch_size = 100" << std::endl;
        file << "group_register_topic_id = 0.0.79574" << std::endl;
        file << "topic_reset_allowed = 1" << std::endl;

        file.close();
    }
    {
        std::fstream file;
        file.open(sibling_file, std::ios::out);
        for (auto i : siblings)
            file << i << std::endl;
        file.close();
    }
}

class DelayedTask : public ITask {
public:
    virtual void run() {
        LOG("time... ");
    }
};
/*
TEST(GradidoSystem, delay_task) {
    
    prepare_system_folders(NODE_0_FOLDER, 13000, 
                           std::vector<std::string>());

    GradidoFacade gf0;
    {    
        std::vector<std::string> params;
        params.push_back("unneeded param");
        params.push_back(std::string(NODE_0_FOLDER) + "/gradido.conf");
        gf0.init(params);
    }

    gf0.push_task(new DelayedTask(), 1);
    sleep(2);
}
*/

TEST(GradidoSystem, smoke) {

    // checking if group contents is properly fetched from another node
    prepare_system_folders(NODE_0_FOLDER, 13000, 
                           std::vector<std::string>());
    GradidoNodeDeprecated gf0;
    {    
        std::vector<std::string> params;
        params.push_back("unneeded param");
        params.push_back(std::string(NODE_0_FOLDER) + "/gradido.conf");
        gf0.init(params);
    }
    HederaTopicID tid;
    tid.shardNum = 0;
    tid.realmNum = 0;
    tid.topicNum = 79606;
    gf0.get_group_register()->add_record("some-group", tid);
    sleep(1);

    std::vector<std::string> siblings1;
    siblings1.push_back("0.0.0.0:" + std::to_string(13000));
    prepare_system_folders(NODE_1_FOLDER, 13001, siblings1);
    GradidoNodeDeprecated gf1;
    {    
        std::vector<std::string> params;
        params.push_back("unneeded param");
        params.push_back(std::string(NODE_1_FOLDER) + "/gradido.conf");
        gf1.init(params);
    }
    sleep(1);

    auto gr = gf1.get_group_register();
    ASSERT_EQ(gr->get_transaction_count(), 1);
    std::vector<GroupInfo> gis = gr->get_groups();
    ASSERT_EQ(gis.size(), 1);


    char alias[GROUP_ALIAS_LENGTH];
    memset(alias, 0, GROUP_ALIAS_LENGTH);
    const char* alias_name = "some-group";
    strcpy(alias, alias_name);
    GroupInfo gii = gis[0];

    ASSERT_EQ(strncmp(alias, gii.alias, GROUP_ALIAS_LENGTH), 0);
    ASSERT_EQ(tid.shardNum, gii.topic_id.shardNum);
    ASSERT_EQ(tid.realmNum, gii.topic_id.realmNum);
    ASSERT_EQ(tid.topicNum, gii.topic_id.topicNum);
    LOG("group data exchange complete");

}


