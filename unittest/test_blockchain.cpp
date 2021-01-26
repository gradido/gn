#include "test_common.h"

#define BLOCKCHAIN_NAME "test-bc"
#define BLOCKCHAIN_FOLDER "/tmp/test-bc"
#define GROUP_REGISTER_FOLDER "/tmp/test-greg"

///////////////////////////
#include <time.h>
#include <algorithm>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/RegularExpression.h>
#include "ed25519/ed25519.h"
#include "file_tile.h"

class SimpleValidator {
public:
    bool is_valid(const SimpleRec* rec, uint32_t count) {
        return true;
    }

};


template<typename T, typename Child>
class AbstractGradidoBlockchain {
private:
    ValidatedMultipartBlockchain<T, Child, 1000> blockchain;
    IGradidoFacade* gf;
    HederaTopicID topic_id;    

public:
    AbstractGradidoBlockchain(
               std::string name, std::string storage_root,
               uint16_t optimal_cache_size,
               uint16_t cache_auto_flush_min_interval_sec) : 
        blockchain(name, storage_root, optimal_cache_size,
                   cache_auto_flush_min_interval_sec, 
                   *((Child*)this)) {}

};

class GroupRegisterBlockchain : public AbstractGradidoBlockchain<
    GroupRegisterRecord, GroupRegisterBlockchain> {
public:
    GroupRegisterBlockchain(std::string storage_root) : 
        AbstractGradidoBlockchain<GroupRegisterRecord, 
                                  GroupRegisterBlockchain>
        ("group-register", storage_root, 10, 5*60) {}
    bool is_valid(const SimpleRec* rec, uint32_t count) {
        return true;
    }

    

};

///////////////////////////

void prepare_folders(std::string folder) {
    erase_tmp_folder(folder);
    Poco::File bf(folder);
    bf.createDirectories();
}

TEST(GradidoBlockchain, smoke) {
    prepare_folders(BLOCKCHAIN_FOLDER);

}


TEST(ValidatedMultipartBlockchain, validated) {
    prepare_folders(BLOCKCHAIN_FOLDER);

    typedef ValidatedMultipartBlockchain<SimpleRec, SimpleValidator,
                                         1000> SB;
    SimpleValidator sv;

    SB bc(BLOCKCHAIN_NAME, 
          BLOCKCHAIN_FOLDER,
          100, 60 * 1, sv);
}

TEST(GroupRegisterBlockchain, append_record) {
    prepare_folders(GROUP_REGISTER_FOLDER);

    typedef Blockchain<SimpleRec, 5> SB;

    SB bc(BLOCKCHAIN_NAME, 
          BLOCKCHAIN_FOLDER,
          100, 60 * 1);
    SimpleRec payload;
    payload.index = 13;
    SB::ExitCode ec;
    bc.append(payload, ec);

    SB::Record* rec = bc.get_block(0, ec);

    ASSERT_EQ(rec->type == (uint8_t)SB::RecordType::PAYLOAD, true);
    ASSERT_EQ(rec->payload.index, payload.index);
    ASSERT_EQ(bc.get_rec_count(), 2);

}


TEST(Blockchain, empty_bc) {
    prepare_folders(BLOCKCHAIN_FOLDER);

    typedef Blockchain<SimpleRec, 1000> SB;

    SB bc(BLOCKCHAIN_NAME, 
          BLOCKCHAIN_FOLDER,
          100, 60 * 1);
}


TEST(Blockchain, single_record_is_appended) {
    prepare_folders(BLOCKCHAIN_FOLDER);

    typedef Blockchain<SimpleRec, 5> SB;

    SB bc(BLOCKCHAIN_NAME, 
          BLOCKCHAIN_FOLDER,
          100, 60 * 1);
    SimpleRec payload;
    payload.index = 13;
    SB::ExitCode ec;
    bc.append(payload, ec);

    SB::Record* rec = bc.get_block(0, ec);

    ASSERT_EQ(rec->type == (uint8_t)SB::RecordType::PAYLOAD, true);
    ASSERT_EQ(rec->payload.index, payload.index);
    ASSERT_EQ(bc.get_rec_count(), 2);
}

TEST(Blockchain, append_and_reopen) {
    prepare_folders(BLOCKCHAIN_FOLDER);
    typedef Blockchain<SimpleRec, 1000> SB;
    int the_index = 13;
    SB::ExitCode ec;

    {
        SB bc(BLOCKCHAIN_NAME, 
              BLOCKCHAIN_FOLDER,
              100, 60 * 1);
        SimpleRec payload;
        payload.index = the_index;
        bc.append(payload, ec);

        SB::Record* rec = bc.get_block(0, ec);
        ASSERT_EQ(rec->type == (uint8_t)SB::RecordType::PAYLOAD, true);
        ASSERT_EQ(rec->payload.index, payload.index);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
    {
        SB bc(BLOCKCHAIN_NAME, 
              BLOCKCHAIN_FOLDER,
              100, 60 * 1);
        ASSERT_EQ(bc.validate_next_checksum(ec), true);

        SB::Record* rec = bc.get_block(0, ec);

        ASSERT_EQ(rec->type == (uint8_t)SB::RecordType::PAYLOAD, true);
        ASSERT_EQ(rec->payload.index, the_index);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
}

///////////////////////////


/*

#define BLOCKCHAIN_NAME "test-bc"
#define BLOCKCHAIN_FOLDER "/tmp/test-bc"

struct SimpleRec {
    SimpleRec() : index(0) {}
    int index;
};

void erase_folders() {
    ASSERT_EQ(std::string(BLOCKCHAIN_FOLDER).rfind("/tmp", 0) == 0, true);
    ASSERT_EQ(std::string(BLOCKCHAIN_FOLDER).size() > 4, true);
    Poco::File bf(BLOCKCHAIN_FOLDER);
    bf.remove(true);
}

void prepare_folders() {
    erase_folders();
    Poco::File bf(BLOCKCHAIN_FOLDER);
    bf.createDirectories();
}

TEST(Blockchain, single_record_is_appended) 
{
    prepare_folders();

    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(1).        
        WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

    Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                             BLOCKCHAIN_FOLDER, 10, &mbv);
    SimpleRec payload;
    payload.index = 13;
    bc.append(payload);
    SimpleRec payload2 = bc.get_rec(0);
    ASSERT_EQ(payload2.index, payload.index);
    ASSERT_EQ(bc.get_rec_count(), 1);
}


TEST(Blockchain, single_existing_record_validated) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(1).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        ASSERT_EQ(bc.get_rec_count(), 1);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(1).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        bc.validate_next_batch(10);
        SimpleRec payload = bc.get_rec(0);
        ASSERT_EQ(payload.index, 13);
        ASSERT_EQ(bc.get_rec_count(), 1);
    }
}

TEST(Blockchain, two_records_are_appended) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        payload.index = 17;
        bc.append(payload);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        bc.validate_next_batch(10);
        SimpleRec payload = bc.get_rec(0);
        ASSERT_EQ(payload.index, 13);
        payload = bc.get_rec(1);
        ASSERT_EQ(payload.index, 17);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
}

TEST(Blockchain, validate_batch_two_parts) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        payload.index = 17;
        bc.append(payload);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        bc.validate_next_batch(1);
        bc.validate_next_batch(1);
        SimpleRec payload = bc.get_rec(0);
        ASSERT_EQ(payload.index, 13);
        payload = bc.get_rec(1);
        ASSERT_EQ(payload.index, 17);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
}

TEST(Blockchain, validate_batch_half) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        payload.index = 17;
        bc.append(payload);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(1).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        bc.validate_next_batch(1);
        ASSERT_EQ(bc.get_rec_count(), 1);
    }
}

TEST(Blockchain, validate_batch_more_than_records) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        payload.index = 17;
        bc.append(payload);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 10, &mbv);
        bc.validate_next_batch(10);
        ASSERT_EQ(bc.get_rec_count(), 2);
    }
}

TEST(Blockchain, validate_batch_on_empty) 
{
    prepare_folders();

    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(0);

    Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                             BLOCKCHAIN_FOLDER, 10, &mbv);
    bc.validate_next_batch(10);
    ASSERT_EQ(bc.get_rec_count(), 0);
}


TEST(Blockchain, one_record_per_block_append_two) 
{
    prepare_folders();
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 1, &mbv);
        SimpleRec payload;
        payload.index = 13;
        bc.append(payload);
        payload.index = 17;
        bc.append(payload);
    }
    {
        NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
        EXPECT_CALL(mbv, validate(_)).
            Times(2).
            WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER, 1, &mbv);
        bc.validate_next_batch(2);
        SimpleRec payload = bc.get_rec(0);
        ASSERT_EQ(payload.index, 13);
        payload = bc.get_rec(1);
        ASSERT_EQ(payload.index, 17);
    }
}

TEST(Blockchain, non_existent_data_folder) 
{
    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(0);
    bool fail = false;
    try {
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 "/tmp/some-nonexistent-folder-0017a", 
                                 10, &mbv);
        fail = true;
    } catch (std::runtime_error& e) {
        ASSERT_EQ(std::string(e.what()).compare("storage root doesn't exist"), 0);
    }
    if (fail)
        PRECISE_THROW("folder was considered ok");
}

TEST(Blockchain, cannot_write_storage_root) 
{
    prepare_folders();
    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(0);
    bool fail = false;

    Poco::File sr(BLOCKCHAIN_FOLDER);
    sr.setWriteable(false);
    try {
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER,
                                 10, &mbv);
        fail = true;
    } catch (std::runtime_error& e) {
        ASSERT_EQ(std::string(e.what()).compare("storage root cannot be written"), 0);
    }
    if (fail)
        PRECISE_THROW("folder was considered ok");
}

TEST(Blockchain, storage_root_not_a_folder) 
{
    erase_folders();
    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(0);
    bool fail = false;

    Poco::File sr(BLOCKCHAIN_FOLDER);
    std::fstream file;
    file.open(BLOCKCHAIN_FOLDER, std::ios::out);
    file << "should be a folder, but here is a file";
    file.close();
    
    try {
        Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                                 BLOCKCHAIN_FOLDER,
                                 10, &mbv);
        fail = true;
    } catch (std::runtime_error& e) {
        ASSERT_EQ(std::string(e.what()).compare("storage root is not a folder"), 0);
    }
    if (fail)
        PRECISE_THROW("folder was considered ok");
}


TEST(Blockchain, get_rec_index_out_of_bounds) 
{
    prepare_folders();

    NiceMock<MockBlockchainValidator<SimpleRec>> mbv;
    EXPECT_CALL(mbv, validate(_)).
        Times(1).        
        WillRepeatedly(Return(Blockchain<SimpleRec>::RecordValidationResult::VALID));

    Blockchain<SimpleRec> bc(BLOCKCHAIN_NAME, 
                             BLOCKCHAIN_FOLDER, 10, &mbv);
    SimpleRec payload;
    payload.index = 13;
    bc.append(payload);
    bool fail = false;
    try {
        SimpleRec payload2 = bc.get_rec(1);
        fail = true;
    } catch (std::runtime_error& e) {
        ASSERT_EQ(std::string(e.what()).compare("rec_num out of bounds: 1"), 0);
    }
    if (fail)
        PRECISE_THROW("index was considered ok");
}

*/

