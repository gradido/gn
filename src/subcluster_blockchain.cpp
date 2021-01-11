#include "subcluster_blockchain.h"

namespace gradido {

    bool SubclusterBlockchain::add_index_data() {
        StorageType::ExitCode ec;
        for (uint32_t i = indexed_blocks; 
             i < storage.get_checksum_valid_block_count(); i++) {
            StorageType::Record* block = storage.get_block(i, ec);
            for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                StorageType::Record* rec = block + j;
                if (rec->type == StorageType::RecordType::CHECKSUM)
                    break;
                SbRecord* pp = &rec->payload;

                if (pp->result != (uint8_t)SbTransactionResult::SUCCESS)
                    continue;

                do_add_index_data(pp);                
            }
        }
        return true;
    }

    bool SubclusterBlockchain::get_latest_transaction_id(uint64_t& res) {
        if (storage.get_rec_count() == 0)
            return false;
        // TODO: ec...
        StorageType::ExitCode ec;
        StorageType::Record* rec = storage.get_record(
                                   storage.get_rec_count() - 2, ec);
        SbRecord& gr = rec->payload;
        // TODO: stucturally bad message
        res = gr.hedera_transaction.sequenceNumber;
        return true;
    }

    SubclusterBlockchain::SubclusterBlockchain(
                                 std::string pub_key,
                                 std::string ordering_node_endpoint,
                                 std::string ordering_node_pub_key,
                                 Poco::Path root_folder,
                                 IGradidoFacade* gf) : 
        Parent(pub_key, pub_key, 
               ordering_node_endpoint,
               ordering_node_pub_key, root_folder, gf),
                     first_good_validation(true),
        notifier(0) {}

    bool SubclusterBlockchain::get_transaction_id(uint64_t& res, 
                                           const Batch<SbRecord>& rec) {
        // TODO: structurally bad message
        res = rec.buff[0].hedera_transaction.sequenceNumber;
        return true;
    }

    void SubclusterBlockchain::do_add_index_data(SbRecord* pp) {
        switch ((SbRecordType)pp->record_type) {
        case SbRecordType::SB_HEADER:
            initial_key = std::string((char*)pp->header.pub_key, 
                                      PUB_KEY_LENGTH);
            break;
        case SbRecordType::SB_ADD_ADMIN:
            admins.insert({std::string((char*)pp->admin.pub_key, 
                                       PUB_KEY_LENGTH), 
                        pp->admin});
            break;
        case SbRecordType::SB_ADD_NODE: {
            nodes.insert({std::string((char*)pp->add_node.pub_key, 
                                      PUB_KEY_LENGTH), 
                        pp->add_node});
            auto z = nodes_by_type.find((SbNodeType)pp->add_node.node_type);
            z->second.insert({std::string((char*)pp->add_node.pub_key, 
                                          PUB_KEY_LENGTH), 
                        pp->add_node});
            break;
        }
            // TODO: finish with sb features
        case SbRecordType::ENABLE_ADMIN:
            break;
        case SbRecordType::DISABLE_ADMIN:
            break;
        case SbRecordType::ENABLE_NODE:
            break;
        case SbRecordType::DISABLE_NODE:
            break;
        case SbRecordType::ADD_BLOCKCHAIN:
            blockchains.insert({std::string(
                        (char*)pp->add_blockchain.pub_key, 
                        PUB_KEY_LENGTH), 
                        pp->add_blockchain});
            break;
        case SbRecordType::ADD_NODE_TO_BLOCKCHAIN:
            // TODO: finish
            break;
        case SbRecordType::REMOVE_NODE_FROM_BLOCKCHAIN:
            break;
        case SbRecordType::SIGNATURES:
            // TODO: validate signatures
            break;
        }
    }

    bool SubclusterBlockchain::calc_fields_and_update_indexes_for(
                        Batch<SbRecord>& b) {

        if (b.size == 0)
            return false;

        SbTransactionResult result = SbTransactionResult::SUCCESS;

        /*
    DUPLICATE_PUB_KEY,
    BAD_ADMIN_SIGNATURE,
    NON_MAJORITY,
    DUPLICATE_ALIAS,
    NODE_ALREADY_ADDED_TO_BLOCKCHAIN
        */

        SbRecord& rec = b.buff[0];

        while (1) {
            if (rec.record_type == (uint8_t)SbRecordType::SB_ADD_ADMIN &&
                admins.find(std::string((char*)rec.admin.pub_key, 
                                        PUB_KEY_LENGTH)) != admins.end()) {
                result = SbTransactionResult::DUPLICATE_PUB_KEY;
                break;
            }
            // TODO: more
            break;
        }
        rec.result = (uint8_t)result;

        if (result == SbTransactionResult::SUCCESS) {
            do_add_index_data(&rec);
            if (notifier)
                gf->push_task(notifier->create());
        }

        return true;
    }

    void SubclusterBlockchain::clear_indexes() {
        admins.clear();
        nodes.clear();
        for (auto i : nodes_by_type) 
            i.second.clear();
        blockchains.clear();
    }

    void SubclusterBlockchain::on_blockchain_ready() {
        StorageType::ExitCode ec;

        if (first_good_validation) {
            first_good_validation = false;
            gf->push_task(new ContinueFacadeInitAfterSbDone(gf));
        }
    }

    void SubclusterBlockchain::init() {
        LOG(name + " init()");

        nodes_by_type.insert({SbNodeType::ORDERING, 
                    std::map<std::string, SbNode>()});
        nodes_by_type.insert({SbNodeType::GRADIDO, 
                    std::map<std::string, SbNode>()});
        nodes_by_type.insert({SbNodeType::LOGIN, 
                    std::map<std::string, SbNode>()});
        nodes_by_type.insert({SbNodeType::BACKUP, 
                    std::map<std::string, SbNode>()});
        nodes_by_type.insert({SbNodeType::LOGGER, 
                    std::map<std::string, SbNode>()});
        nodes_by_type.insert({SbNodeType::PINGER, 
                    std::map<std::string, SbNode>()});

        std::string endpoint = get_ordering_node_endpoint();
        gf->get_communications()->receive_grpr_transactions(endpoint,
                                                            pub_key,
                                                            this);
    }

    std::vector<std::string> SubclusterBlockchain::get_logger_nodes() {
        auto z = nodes_by_type.find(SbNodeType::LOGGER)->second;
        std::vector<std::string> res;
        for (auto i : z)
            res.push_back(std::string((char*)i.second.pub_key, 
                                      PUB_KEY_LENGTH));
        return res;
    }

    bool SubclusterBlockchain::is_valid(const SbRecord* rec, 
                                        uint32_t count) {
        return true;
        // TODO
    }

    bool SubclusterBlockchain::get_fetch_endpoint(std::string& res) {
        res = get_ordering_node_endpoint();
        return true;
    }

    void SubclusterBlockchain::on_succ_append(
        ISubclusterBlockchain::INotifier* n) {
        notifier = n;
    }




}
