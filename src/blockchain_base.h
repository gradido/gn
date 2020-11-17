#ifndef BLOCKCHAIN_BASE_H
#define BLOCKCHAIN_BASE_H

#include <Poco/Path.h>
#include <pthread.h>
#include <queue>
#include <Poco/File.h>
#include "gradido_events.h"
#include "utils.h"
#include "gradido_interfaces.h"
#include "blockchain_gradido_def.h"
#include "blockchain.h"

namespace gradido {

#define BLOCKCHAIN_RESET_TIMEOUT_SECONDS 5

// all blockchains have something in common; this is kept here
// T: record type
// Child: BlockchainBase is expected to be inherited; have to specify
//   Child class here; there are several pure virtual methods which
//   this class has to implement
// Parent: to keep class hierarchy less complex, BlockchainBase 
//   Parent can be specified
// TransactionComparator: used to order incoming transactions; expected
//   to contain only operator(lhs, rhs)
template<typename T, typename Child, typename Parent, 
    typename TransactionComparator>
class BlockchainBase : public Parent,
    ICommunicationLayer::BlockChecksumReceiver,
    ICommunicationLayer::BlockRecordReceiver,
    ICommunicationLayer::TransactionListener {
 public:


    virtual void on_transaction(ConsensusTopicResponse& transaction) {
        gf->push_task(new PushTransactionsTask<T>(this, transaction));
    }
    virtual void on_block_record(grpr::BlockRecord br) {
        MLock lock(main_lock);

        if (state != FETCHING_BLOCKS)
            throw std::runtime_error("group register on_block_record(): wrong state");
        if (!br.success()) {

            bool last_block = block_to_fetch == fetched_checksums.size();
            if (last_block) {
                if (fetched_data_count != fetched_record_count) {
                    reset_validation("bad record count in last block");
                } else {
                    state = VALIDATING;
                    clear_indexes();
                    indexed_blocks = 0;
                    gf->push_task(new ContinueValidationTask<T>(this));
                }
            } else {
                if (fetched_data_count != fetched_record_count)
                    reset_validation("bad record count"); 
                else {
                    block_to_fetch++;
                    fetch_next_block();
                }
            }
        } else {
            // omitting checksums; they are calculated anyway
            if (fetched_data_count % GRADIDO_BLOCK_SIZE < 
                GRADIDO_BLOCK_SIZE - 1 && 
                fetched_data_count + 1 < fetched_record_count) {
                T rec;
                memcpy(&rec, br.record().c_str(), sizeof(T));
                typename StorageType::ExitCode ec;
                storage.append(&rec, 1, ec);
                // TODO: check exit code
            }
            fetched_data_count++;
        }
    }

    virtual void on_block_checksum(grpr::BlockChecksum bc) {
        MLock lock(main_lock);

        if (state != FETCHING_CHECKSUMS)
            throw std::runtime_error(name + " on_block_checksum(): wrong state");

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
                reset_validation("unexpected checksum");
            } else {
                if (bc.block_size() < GRADIDO_BLOCK_SIZE)
                    fetched_checksum_expecting_last = true;
                fetched_record_count += bc.block_size();
                fetched_checksums.push_back(bc.checksum());
            }
        }
    }


 protected:
    // state can return to INITIAL if something goes wrong; this can
    // happen from any other state
    enum State {
        INITIAL=0,
        FETCHING_CHECKSUMS,
        FETCHING_BLOCKS,
        VALIDATING,
        READY
    };

    State state;
    IGradidoFacade* gf;
    std::string name;
    HederaTopicID topic_id;
    typedef ValidatedMultipartBlockchain<T, Child, GRADIDO_BLOCK_SIZE> StorageType; 

    std::string data_storage_root;
    StorageType storage;

    std::string ensure_storage_root(const Poco::Path& sr) {
        Poco::Path p0(sr);
        p0.append(name);
        Poco::File srf(p0.absolute());
        if (!srf.exists())
            srf.createDirectories();
        if (!srf.exists() || !srf.isDirectory())
            throw std::runtime_error("group register: cannot create folder");
        return p0.absolute().toString();
    }

    pthread_mutex_t main_lock;
    std::string fetch_endpoint;

    // those are related to block checksums
    std::vector<std::string> fetched_checksums;
    uint64_t fetched_record_count;
    bool fetched_checksum_expecting_last;

    // those are related to block data
    uint32_t block_to_fetch;
    uint64_t fetched_data_count;

    void fetch_next_block() {
        grpr::BlockDescriptor bd;
        grpr::GroupDescriptor* gd = bd.mutable_group();
        gd->set_group(name);
        bd.set_block_id(block_to_fetch);
        gf->get_communications()->require_block_data(
                                  fetch_endpoint, bd, this);
    }
    void reset_validation(std::string cause) {
        LOG(name + ": reset_validation() called, cause: " + cause);
        state = INITIAL;
        storage.truncate(0);
        clear_indexes();
        indexed_blocks = 0;
        gf->push_task(new ContinueValidationTask<T>(this), 
                      BLOCKCHAIN_RESET_TIMEOUT_SECONDS);
    }
    void validate_next_batch() {
        if (storage.get_checksum_valid_block_count() ==
            storage.get_block_count()) {
            state = READY;
            if (add_index_data()) {
                LOG(name + " blockchain ready");
                on_blockchain_ready();
            } else
                reset_validation("cannot add_index_data(), last block");
        } else {
            typename StorageType::ExitCode ec;
            bool res = storage.validate_next_checksum(ec);
            if (!res)
                reset_validation("cannot validate_next_checksum()");
            else {
                if (add_index_data())
                    gf->push_task(new ContinueValidationTask<T>(this));
                else
                    reset_validation("cannot add_index_data()");
            }
        }
    }

    uint64_t indexed_blocks;


    std::priority_queue<Batch<T>, std::vector<Batch<T>>, 
        TransactionComparator> inbound;
    int batch_size;
    bool topic_reset_allowed;

 protected:
    virtual bool get_latest_transaction_id(uint64_t& res) = 0;
    virtual bool get_transaction_id(uint64_t& res, 
                                    const Batch<T>& rec) = 0;

    // returns true, if record is ready to be inserted; if false, then
    // Child should create all necessary events for processing to not
    // stop
    virtual bool calc_fields_and_update_indexes_for(
                 Batch<T>& b) = 0;
    virtual void clear_indexes() = 0;
    virtual bool add_index_data() = 0;

    virtual void on_blockchain_ready() {}

 public:
    BlockchainBase(std::string name,
                Poco::Path root_folder,
                IGradidoFacade* gf,
                HederaTopicID topic_id) :
    state(INITIAL), gf(gf), name(name),
        topic_id(topic_id),
        data_storage_root(ensure_storage_root(root_folder)),
        storage(name, data_storage_root, 100, 100, *((Child*)this)),
        batch_size(gf->get_conf()->
                   get_blockchain_append_batch_size()),
        topic_reset_allowed(gf->get_conf()->is_topic_reset_allowed()) {

        SAFE_PT(pthread_mutex_init(&main_lock, 0));
    }


    virtual void init() {
        LOG(name + " init()");
        std::string endpoint = gf->get_conf()->get_hedera_mirror_endpoint();
        gf->get_communications()->receive_hedera_transactions(endpoint,
                                                              topic_id,
                                                              this);
    }

    virtual void continue_validation() {
        MLock lock(main_lock);

        switch (state) {
        case INITIAL: {
            if (gf->get_random_sibling_endpoint(fetch_endpoint)) {
                state = FETCHING_CHECKSUMS;
                LOG(name + " blockchain fetches checksums from " + 
                    fetch_endpoint);
                fetched_record_count = 0;
                fetched_checksums.clear();
                fetched_checksum_expecting_last = false;
                grpr::GroupDescriptor gd;
                gd.set_group(name);
                gf->get_communications()->require_block_checksums(
                                          fetch_endpoint, gd, this);
            } else {
                LOG(name + " blockchain cannot get sibling endpoint");
                state = VALIDATING;
                clear_indexes();
                indexed_blocks = 0;
                gf->push_task(new ContinueValidationTask<T>(this));
            }
            break;
        }
        case VALIDATING:
            validate_next_batch();
            break;
        case FETCHING_CHECKSUMS:
        case FETCHING_BLOCKS:
        case READY:
            throw std::runtime_error(name + " init(): wrong state");
        }
    }    

    virtual void continue_with_transactions() {
        MLock lock(main_lock);
        if (state != READY)
            return;

        for (int i = 0; i < batch_size; i++) {
            if (inbound.size() == 0)
                break;
            Batch<T> batch = inbound.top();
            inbound.pop();

            // TODO: stucturally bad message

            if (batch.reset_blockchain) {
                if (!topic_reset_allowed)
                    throw std::runtime_error("cannot reset topic");
                storage.truncate(0);
                indexed_blocks = 0;
                clear_indexes();
                if (batch.buff)
                    delete [] batch.buff;
            } else {
                if (batch.size < 1 || !batch.buff)
                    throw std::runtime_error("empty batch");

                uint64_t seq_num;
                bool has_latest = get_latest_transaction_id(seq_num);
                uint64_t curr_seq_num;
                get_transaction_id(curr_seq_num, batch);

                if ((has_latest && curr_seq_num == seq_num + 1) ||
                    (!has_latest && (topic_reset_allowed || 
                                     curr_seq_num == 1))) {
                    if (calc_fields_and_update_indexes_for(batch)) {
                        typename StorageType::ExitCode ec;
                        storage.append(batch.buff, batch.size, ec);
                        delete [] batch.buff;
                    } else {
                        inbound.push(batch);
                        return;
                    }
                } else {
                    if (has_latest && curr_seq_num <= seq_num) {
                        // omitting
                        delete [] batch.buff;
                        continue;
                    } else {
                        // TODO: more elaborate; need just some 
                        // records, not all of them
                        inbound.push(batch);
                        reset_validation("hole in seq_num");
                        return;
                    }
                }
            }
        }

        if (inbound.size() > 0)
            gf->push_task(new ContinueTransactionsTask<T>(this));
    }

    virtual uint64_t get_transaction_count() {
        MLock lock(main_lock);
        if (state != READY)
            return 0;
        uint64_t rc = storage.get_rec_count();
        if (rc == 0)
            return 0;
        // taking into account checksums
        return rc - storage.get_block_count();
    }

    virtual void add_transaction(Batch<T> rec) {
        {
            MLock lock(main_lock);
            inbound.push(rec);
        }
        continue_with_transactions();
    }

    virtual uint32_t get_block_count() {
        MLock lock(main_lock);
        return storage.get_block_count();
    }

    virtual void get_checksums(std::vector<BlockInfo>& res) {
        MLock lock(main_lock);
        res.clear();

        for (uint32_t i = 0; i < storage.get_block_count(); i++) {
            BlockInfo bi;
            typename StorageType::ExitCode ec;
            typename StorageType::Record* rec = storage.get_block(i, ec);
            if (i == storage.get_block_count() - 1) {
                for (uint32_t j = 0; j < GRADIDO_BLOCK_SIZE; j++) {
                    typename StorageType::Record* rec2 = rec + j;
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
    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::BlockRecord& res) {
        MLock lock(main_lock);

        uint32_t block_num = (uint32_t)(seq_num / GRADIDO_BLOCK_SIZE);
        
        if (block_num >= storage.get_block_count())
            return false;

        uint32_t seq_index = (uint32_t)(seq_num % GRADIDO_BLOCK_SIZE);

        typename StorageType::ExitCode ec;
        typename StorageType::Record* rec = 
            storage.get_block(block_num, ec) + seq_index;

        if (rec->type == StorageType::RecordType::EMPTY)
            return false;
        res.set_success(true);
        if (rec->type == StorageType::RecordType::PAYLOAD)
            res.set_record(std::string((char*)(&rec->payload), sizeof(T)));
        else if (rec->type == StorageType::RecordType::CHECKSUM)
            res.set_record(std::string((char*)rec->checksum, sizeof(T)));
        return true;
    }

    virtual bool get_block_record(uint64_t seq_num, 
                                  grpr::OutboundTransaction& res) {
        MLock lock(main_lock);

        uint32_t block_num = (uint32_t)(seq_num / GRADIDO_BLOCK_SIZE);
        
        if (block_num >= storage.get_block_count())
            return false;

        uint32_t seq_index = (uint32_t)(seq_num % GRADIDO_BLOCK_SIZE);

        typename StorageType::ExitCode ec;
        typename StorageType::Record* rec = 
            storage.get_block(block_num, ec) + seq_index;

        if (rec->type == StorageType::RecordType::EMPTY)
            return false;
        res.set_success(true);
        if (rec->type == StorageType::RecordType::PAYLOAD)
            res.set_data(std::string((char*)(&rec->payload), sizeof(T)));
        else if (rec->type == StorageType::RecordType::CHECKSUM)
            res.set_data(std::string((char*)rec->checksum, sizeof(T)));
        return true;
    }

};

}


#endif
