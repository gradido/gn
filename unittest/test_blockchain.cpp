#include "test_common.h"
#include "blockchain.h"

#define BLOCKCHAIN_NAME "test-bc"
#define BLOCKCHAIN_FOLDER "/tmp/test-bc"

struct SimpleRec {
    SimpleRec() : index(0) {}
    int index;
};

void erase_folders() {
    ASSERT_EQ(std::string(BLOCKCHAIN_FOLDER).rfind("/tmp", 0) == 0, true);
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
        throw std::runtime_error("folder was considered ok");
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
        throw std::runtime_error("folder was considered ok");
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
        throw std::runtime_error("folder was considered ok");
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
        throw std::runtime_error("index was considered ok");
}



