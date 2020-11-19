#include "group_register.h"


namespace gradido {

    bool GroupRegister::is_valid(const GroupRegisterRecord* rec, 
                                 uint32_t count) {
        // TODO
        return true;
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

    bool GroupRegister::get_latest_transaction_id(uint64_t& res) {
        if (storage.get_rec_count() == 0)
            return false;
        // TODO: ec...
        StorageType::ExitCode ec;
        StorageType::Record* rec = storage.get_record(
                                   storage.get_rec_count() - 2, ec);
        GroupRegisterRecord& gr = rec->payload;
        // TODO: stucturally bad message
        res = gr.group_record.hedera_transaction.sequenceNumber;
        return true;
    }

    GroupRegister::GroupRegister(Poco::Path root_folder,
                                 IGradidoFacade* gf,
                                 HederaTopicID topic_id) : 
        Parent(GROUP_REGISTER_NAME, root_folder, gf, topic_id),
        first_good_validation(true) {}


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
        rec0.success = alias.size() > 0 && 
            aliases.find(alias) == aliases.end();
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

    bool GroupRegister::get_transaction_id(uint64_t& res, 
                                           const Batch<GroupRegisterRecord>& rec) {
        // TODO: structurally bad message
        res = rec.buff[0].group_record.hedera_transaction.sequenceNumber;
        return true;
    }

    bool GroupRegister::calc_fields_and_update_indexes_for(
                        Batch<GroupRegisterRecord>& b) {
        
        // TODO: structurally bad message
        GroupRecord& rec = b.buff[0].group_record;
        // TODO: check bad characters

        std::string alias((char*)rec.alias, 
                          GROUP_ALIAS_LENGTH);

        bool succ = alias.size() > 0 && 
            aliases.find(alias) == aliases.end();
        rec.success = succ;

        if (succ) 
            aliases.insert({alias, rec});
        return true;
    }

    void GroupRegister::clear_indexes() {
        aliases.clear();
    }

    void GroupRegister::on_blockchain_ready() {
        StorageType::ExitCode ec;

        if (first_good_validation) {
            first_good_validation = false;
            gf->push_task(new ContinueFacadeInit(gf));
        }
    }

    bool GroupRegister::get_topic_id(std::string alias,
                                     HederaTopicID& res) {

        // TODO: optimize
        std::vector<GroupInfo> zz = get_groups();
        for (auto i : zz) {
            std::string aa(i.alias);
            if (aa.compare(alias) == 0) {
                res = i.topic_id;
                return true;
            }
        }
        return false;
    }
    
}


