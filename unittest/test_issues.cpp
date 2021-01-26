#include "test_common.h"
#include "blockchain_gradido.h"


#include "facades_impl.h"
#include "communications.h"
#include "config.h"

// TODO: better to split between related branches, where possible; 
// extracting and naming utility classes would be too painful
#define ISSUES_TEST_FOLDER "/tmp/gradido-issues-folder"

class TestComm : public EmptyCommunications {
public:
    virtual void receive_hedera_transactions(std::string endpoint,
                                             HederaTopicID topic_id,
                                             TransactionListener* tl) {}

};

class TestConfig : public EmptyConfig {
public:
    virtual int get_blockchain_append_batch_size() {return 10;}
    virtual bool is_topic_reset_allowed() {return true;}
    virtual std::string get_hedera_mirror_endpoint() {return "";}

};

class TestFacade : public EmptyFacade {
private:
    TestComm comm;
    TestConfig conf;
    std::vector<ITask*> tasks;
public:
    virtual ICommunicationLayer* get_communications() {return &comm;}
    virtual IGradidoConfig* get_conf() {return &conf;}
    virtual void push_task(ITask* task) {
        LOG("added task " << task->get_task_info());
        tasks.push_back(task);
    }
    virtual void push_task(ITask* task, uint32_t after_seconds) {
        LOG("adding task " << task->get_task_info() << " after " <<
            after_seconds << " seconds");
        tasks.push_back(task);
    }
    virtual void push_task_and_join(ITask* task) {
        LOG("added task " << task->get_task_info() << " for joining");
        // cannot execute directly because of possible problems with
        // reentrancy; this breaks behaviour, TODO: fix
        tasks.push_back(task);
    }


    virtual bool get_random_sibling_endpoint(std::string& res) {return false;}


    bool process_tasks() {
        std::vector<ITask*> tmp = tasks;
        tasks.clear();
        for (ITask* i : tmp) {
            i->run();
            delete i;
        }
        return tasks.size() == 0;
    }
    
};

void prepare_folder() {
    erase_tmp_folder(ISSUES_TEST_FOLDER);
    Poco::File ff(ISSUES_TEST_FOLDER);
    ff.createDirectories();
}

void create_gradido_add_user_with_two_signatures(
                        Batch<GradidoRecord>& rec, 
                        int seq_num) {
    rec.reset_blockchain = false;
    rec.size = 2;
    rec.buff = new GradidoRecord[2];
    GradidoRecord* buff = rec.buff;
    {
        buff[0].record_type = (uint8_t)GRADIDO_TRANSACTION;
        buff[0].transaction.version_number = 1;
        buff[0].transaction.parts = 2;
        buff[0].transaction.hedera_transaction.consensusTimestamp.seconds = 1;
        buff[0].transaction.hedera_transaction.consensusTimestamp.nanos = 1;
        buff[0].transaction.hedera_transaction.runningHash[0] = 1;
        buff[0].transaction.hedera_transaction.sequenceNumber = seq_num;
        buff[0].transaction.hedera_transaction.runningHashVersion = 1;

        buff[0].transaction.signature.pubkey[0] = 1;
        buff[0].transaction.signature.signature[0] = 1;
        buff[0].transaction.signature_count = 2;
        buff[0].transaction.transaction_type = (uint8_t)ADD_USER;
        buff[0].transaction.add_user.user[0] = seq_num;
        buff[0].transaction.result = (uint8_t)SUCCESS;
    }
    {
        buff[1].record_type = (uint8_t)SIGNATURES;
        buff[1].signature[0].pubkey[0] = 2;
        buff[1].signature[0].signature[0] = 2;
    }
}

TEST(GradidoIssues, two_signature_crash) {
    prepare_folder();
    TestFacade gf;
    HederaTopicID hti;
    BlockchainGradido bg("test", ISSUES_TEST_FOLDER, &gf, hti);
    bg.init();
    bg.continue_validation();
    while (!gf.process_tasks());
    
    {
        Batch<GradidoRecord> rec;
        create_gradido_add_user_with_two_signatures(rec, 1);
        bg.add_transaction(rec);
    }
    {
        Batch<GradidoRecord> rec;
        create_gradido_add_user_with_two_signatures(rec, 2);
        bg.add_transaction(rec);
    }
}
