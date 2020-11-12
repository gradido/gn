#include "group_register.h"
#include <Poco/File.h>
#include "gradido_events.h"
#include "utils.h"


namespace gradido {

    bool GroupRegister::is_valid(const GroupRegisterRecord* rec, 
                                 uint32_t count) {
        // TODO
        return true;
    }

    void GroupRegister::init() {
        // nothing here
    }

    void GroupRegister::continue_validation() {
        MLock lock(main_lock);

        switch (state) {
        case INITIAL: {
            if (gf->get_random_sibling_endpoint(fetch_endpoint)) {
                state = FETCHING_CHECKSUMS;
                fetched_record_count = 0;
                fetched_checksums.clear();
                fetched_checksum_expecting_last = false;
                grpr::GroupDescriptor gd;
                gd.set_group(GROUP_REGISTER_NAME);
                gf->get_communications()->require_block_checksums(
                                          fetch_endpoint, gd, this);
            } else {
                state = VALIDATING;
                aliases.clear();
                indexed_blocks = 0;
                gf->push_task(new ValidateGroupRegister(gf));
            }
            break;
        }
        case VALIDATING:
            validate_next_batch();
            break;
        case FETCHING_CHECKSUMS:
        case FETCHING_BLOCKS:
        case READY:
            throw std::runtime_error("group register init(): wrong state");
        }
    }    

    bool GroupRegister::add_index_data() {
        StorageType::ExitCode ec;
        for (uint32_t i = indexed_blocks; 
             i < storage.get_checksum_valid_block_count(); i++) {
            StorageType::Record* block = storage.get_block(i, ec);
            for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                StorageType::Record* rec = block + j;
                if (rec->type == StorageType::RecordType::CHECKSUM)
                    break;
                GroupRegisterRecord* pp = &rec->payload;

                if (pp->record_type == 
                    GroupRegisterRecordType::GROUP_RECORD && 
                    pp->group_record.success) {

                    // TODO: check false negatives

                    std::string alias((char*)pp->group_record.alias);
                    if (aliases.find(alias) != aliases.end())
                        return false;
                    aliases.insert({alias, pp->group_record});
                }
            }
        }
        return true;
    }


    void GroupRegister::validate_next_batch() {
        if (storage.get_checksum_valid_block_count() ==
            storage.get_block_count()) {
            state = READY;
            if (add_index_data()) {
                if (first_good_validation) {
                    first_good_validation = false;
                    gf->push_task(new ContinueFacadeInit(gf));
                }
            } else
                reset_validation();
        } else {
            StorageType::ExitCode ec;
            bool res = storage.validate_next_checksum(ec);
            if (!res)
                reset_validation();
            else {
                if (add_index_data())
                    gf->push_task(new ValidateGroupRegister(gf));
                else
                    reset_validation();
            }
        }
    }


    void GroupRegister::continue_with_transactions() {
        MLock lock(main_lock);
    }


    uint64_t GroupRegister::get_transaction_count() {
        MLock lock(main_lock);

        return 0;
    }
    void GroupRegister::require_blocks(std::vector<std::string> endpoints) {}

    GroupRegister::GroupRegister(Poco::Path root_folder,
                                 IGradidoFacade* gf,
                                 HederaTopicID topic_id) : 
        state(INITIAL), gf(gf),
        data_storage_root(ensure_storage_root(root_folder)),
        storage(GROUP_REGISTER_NAME, data_storage_root, 100, 100, 
               *this),
        first_good_validation(true) {
        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }

    void GroupRegister::add_transaction(const GroupRecord& rec) {
        MLock lock(main_lock);

    }

    void GroupRegister::add_multipart_transaction(
                 const GroupRegisterRecord* rec, 
                 size_t rec_count) {
        MLock lock(main_lock);

    }

    void GroupRegister::get_checksums(std::vector<BlockInfo>& res) {
        MLock lock(main_lock);
        res.clear();

        for (uint32_t i = 0; i < storage.get_block_count(); i++) {
            BlockInfo bi;
            StorageType::ExitCode ec;
            StorageType::Record* rec = storage.get_block(i, ec);
            if (i == storage.get_block_count() - 1) {
                for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                    StorageType::Record* rec2 = rec + j;
                    if (rec2->type == 
                        StorageType::RecordType::CHECKSUM) {
                        bi.size = j + 1;
                        memset(bi.checksum, 0, BLOCKCHAIN_CHECKSUM_SIZE);
                        strcpy((char*)bi.checksum, 
                               (char*)rec2[j].checksum);
                        res.push_back(bi);
                        return;
                    }
                }                
            } else {
                bi.size = GRADIDO_BLOCK_SIZE;
                memset(bi.checksum, 0, BLOCKCHAIN_CHECKSUM_SIZE);
                strcpy((char*)bi.checksum, 
                       (char*)rec[GRADIDO_BLOCK_SIZE - 1].checksum);
                res.push_back(bi);
            }
        }
    }

    void GroupRegister::add_record(std::string alias, 
                                   HederaTopicID tid) {
        MLock lock(main_lock);
        GroupRegisterRecord rec1;
        memset(&rec1, 0, sizeof(GroupRegisterRecord));
        rec1.record_type = GroupRegisterRecordType::GROUP_RECORD;
        GroupRecord& rec0 = rec1.group_record;
        memset(&rec0, 0, sizeof(GroupRecord));
        strcpy((char*)rec0.alias, (char*)alias.c_str());
        rec0.topic_id = tid;
        rec0.success = aliases.find(alias) == aliases.end();
        if (rec0.success)
            aliases.insert({alias, rec0});
        StorageType::ExitCode ec;
        storage.append(&rec1, 1, ec);
    }

    std::vector<GroupInfo> GroupRegister::get_groups() {
        MLock lock(main_lock);
        std::vector<GroupInfo> res;
        if (state == READY) {
            StorageType::ExitCode ec;

            for (uint32_t i = 0; i < storage.get_block_count(); i++) {
                StorageType::Record* block = storage.get_block(i, ec);
                for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                    StorageType::Record* rec = block + j;
                    if (rec->type == StorageType::RecordType::CHECKSUM)
                        break;
                    GroupRegisterRecord* pp = &rec->payload;
                    if (pp->record_type == 
                        GroupRegisterRecordType::GROUP_RECORD && 
                        pp->group_record.success) {
                        GroupInfo gi;
                        memcpy(gi.alias, pp->group_record.alias, 
                               GROUP_ALIAS_LENGTH);
                        gi.topic_id = pp->group_record.topic_id;
                        res.push_back(gi);
                    }
                }
            }
        }
        return res;
    }


    std::string GroupRegister::ensure_storage_root(const Poco::Path& sr) {
        Poco::Path p0(sr);
        p0.append(GROUP_REGISTER_NAME);
        Poco::File srf(p0.absolute());
        if (!srf.exists())
            srf.createDirectories();
        if (!srf.exists() || !srf.isDirectory())
            throw std::runtime_error("group register: cannot create folder");
        return p0.absolute().toString();
    }

    void GroupRegister::on_block_checksum(grpr::BlockChecksum bc) {
        MLock lock(main_lock);

        if (state != FETCHING_CHECKSUMS)
            throw std::runtime_error("group register on_block_checksum(): wrong state");

        if (!bc.success()) {
            state = FETCHING_BLOCKS;
            block_to_fetch = 0;
            fetched_data_count = 0;
            fetch_next_block();
        } else {
            // TODO: pick between local and remote data
            if (fetched_checksum_expecting_last) {
                // TODO: this will change when there is clarity how
                // to handle situations when siblings send bad data; for
                // now just retry, with possibly other sibling
                reset_validation();
            } else {
                if (bc.block_size() < GRADIDO_BLOCK_SIZE)
                    fetched_checksum_expecting_last = true;
                fetched_record_count += bc.block_size();
                fetched_checksums.push_back(bc.checksum());
            }
        }
    }

    void GroupRegister::fetch_next_block() {
        grpr::BlockDescriptor bd;
        grpr::GroupDescriptor* gd = bd.mutable_group();
        gd->set_group(GROUP_REGISTER_NAME);
        bd.set_block_id(block_to_fetch);
        gf->get_communications()->require_block_data(
                                  fetch_endpoint, bd, this);
    }

    void GroupRegister::reset_validation() {
        LOG("GroupRegister: reset_validation() called");
        state = INITIAL;
        storage.truncate(0);
        aliases.clear();
        indexed_blocks = 0;
        gf->push_task(new ValidateGroupRegister(gf), 5);
    }

    void GroupRegister::on_block_record(grpr::BlockRecord br) {
        MLock lock(main_lock);

        if (state != FETCHING_BLOCKS)
            throw std::runtime_error("group register on_block_record(): wrong state");
        if (!br.success()) {
            bool last_block = block_to_fetch == fetched_checksums.size();
            if (last_block) {
                if (fetched_data_count != fetched_record_count) {
                    reset_validation();
                } else {
                    state = VALIDATING;
                    aliases.clear();
                    indexed_blocks = 0;
                    gf->push_task(new ValidateGroupRegister(gf));
                }
            } else {
                if (fetched_data_count != (block_to_fetch + 1) * 
                    GRADIDO_BLOCK_SIZE) 
                    reset_validation(); 
                else {
                    block_to_fetch++;
                    fetch_next_block();
                }
            }
        } else {
            GroupRegisterRecord rec;
            memcpy(&rec, br.record().c_str(), 
                   sizeof(GroupRegisterRecord));
            StorageType::ExitCode ec;
            storage.append(&rec, 1, ec);
            fetched_data_count++;
            // TODO: check exit code
        }

    }

    bool GroupRegister::get_block_record(uint64_t seq_num, 
                                         grpr::BlockRecord& res) {
        MLock lock(main_lock);

        uint32_t block_num = (uint32_t)(seq_num / GRADIDO_BLOCK_SIZE);
        
        if (block_num >= storage.get_block_count())
            return false;

        uint32_t seq_index = (uint32_t)(seq_num % GRADIDO_BLOCK_SIZE);

        StorageType::ExitCode ec;
        StorageType::Record* rec = 
            storage.get_block(block_num, ec) + seq_index;

        if (rec->type == StorageType::RecordType::EMPTY)
            return false;
        res.set_success(true);
        if (rec->type == StorageType::RecordType::PAYLOAD)
            res.set_record(std::string((char*)(&rec->payload), 
                                       sizeof(GroupRegisterRecord)));
        else if (rec->type == StorageType::RecordType::CHECKSUM)
            res.set_record(std::string((char*)rec->checksum, 
                                       sizeof(GroupRegisterRecord)));
        return true;
    }

    uint32_t GroupRegister::get_block_count() {
        MLock lock(main_lock);
        return storage.get_block_count();
    }


}
