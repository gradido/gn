#ifndef GRADIDO_BLOCKCHAIN_H
#define GRADIDO_BLOCKCHAIN_H

#include <unistd.h>
#include <string>
#include "ed25519/ed25519.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/RegularExpression.h>
#include <fstream>
#include <string.h>
#include "gradido_interfaces.h"
#include "file_tile.h"

namespace gradido {

// - thread agnostic
// - validates in batches
// - last record is always checksum
// - last record in each block is always checksum
// - checksum is calculated from string prev_checksum + curr_record, for
//   each record; only last checksum is stored in block
// - block size cannot be less than 2
template<typename T, int RecCount>
class Blockchain {
public:

    using RecordType = typename BlockchainTypes<T>::RecordType;
    using Record = typename BlockchainTypes<T>::Record;

    enum ExitCode {
        OK=0,
        NON_EMPTY_RECORD_AFTER_CHECKSUM,
        EMPTY_RECORD_BEFORE_CHECKSUM,
        NO_PAYLOAD,
        NO_CHECKSUM,
        EXPECTED_FULL_BLOCK,
        ONLY_LAST_BLOCK_MAY_NOT_BE_FULL,
        BAD_CHECKSUM,
        WRITE_ERROR,
        READ_ERROR,
        LAST_BLOCK_NOT_FULL,
        INDEX_TOO_LARGE,
        BAD_BLOCK_LENGTH
    };

private:
    const uint16_t optimal_cache_size;
    const uint16_t cache_auto_flush_min_interval_sec;

    struct Timestamp {
        Timestamp() : seconds(0) {}
        uint64_t seconds;
        bool operator<(const Timestamp& ts) {
            return ts.seconds > seconds;
        }
        Timestamp operator+(int sec_value) {
            Timestamp res;
            res.seconds = seconds + sec_value;
            return res;
        }
        std::string to_string() {
            return std::to_string(seconds);
        }
    };

    struct TileSup {
        uint8_t checksum[BLOCKCHAIN_CHECKSUM_SIZE];
        Timestamp last_use;
        
        void reset() {
            memset(checksum, 0, BLOCKCHAIN_CHECKSUM_SIZE);
        }
    };

    class Fnr {
    private:
        const Poco::Path storage_path;
    public:
        const std::string name;
        const std::string storage_root;
        Fnr(std::string name, std::string storage_root) : 
            name(name), storage_root(storage_root),
            storage_path(storage_root)  {

            Poco::File tmp(storage_root);
            if (!tmp.exists())
                throw std::runtime_error("storage root doesn't exist");
            if (!tmp.isDirectory())
                throw std::runtime_error("storage root is not a folder");
            if (!tmp.canRead())
                throw std::runtime_error("storage root cannot be read");
            if (!tmp.canWrite())
                throw std::runtime_error("storage root cannot be written");
        }
        uint32_t get_file_count() const {
            uint32_t res = 0;
            Poco::RegularExpression re("^" + name + "\\.(\\d+)\\.block$");
            for (Poco::DirectoryIterator it(storage_root);
                 it != Poco::DirectoryIterator{}; ++it) {

                Poco::File curr(it.path());
                if (curr.isFile()) {
                    std::string fname = it.path().getFileName();
                    std::vector<std::string> ss;

                    if (re.split(fname, ss)) {
                        uint64_t block_num = 
                            static_cast<uint64_t>(std::stoul(ss[1]));

                        // TODO: after cast it may overlap
                        if (std::to_string(block_num).compare(ss[1]) != 0)
                            throw std::runtime_error("bad block number for " + fname);
                        if (res <= block_num)
                            res = block_num + 1;
                    }
                }
                if (res == UINT32_MAX)
                    throw std::runtime_error("too many blocks");
            }
            return res;
        }

        std::string get_file_name(uint32_t index) const {
            Poco::Path tmp = storage_path;
            tmp.append(name + "." + std::to_string(index) + ".block");
            return tmp.absolute().toString();
        }

        std::string get_bad_file_name(Timestamp ts, 
                                      uint32_t index) const {
            Poco::Path tmp = storage_path;
            tmp.append(name + "." + std::to_string(index) + "." + 
                       ts.to_string() + ".bad");
            return tmp.absolute().toString();
        }
    };
    const Fnr fnr;
    typedef FileTile<Record, Fnr, RecCount, TileSup> FtType;
    using Tile = typename FtType::Tile;
    using TileState = typename FtType::TileState;
    FtType tiles;
    uint32_t checksum_valid_block_count;

    static Timestamp get_current_ts() {
        Timestamp res;
        res.seconds = time(0);
        return res;
    }

    Timestamp next_cache_flush_timestamp;
    void maybe_flush_cache() {
        Timestamp ct = get_current_ts();
        if (next_cache_flush_timestamp < ct)
            flush_cache();
    }

    uint64_t rec_count;

    struct SortableTileIndex {
        SortableTileIndex(uint32_t index, Timestamp& ts) : 
            index(index), ts(ts) {}
        uint32_t index;
        Timestamp ts;
        bool operator<(const SortableTileIndex& sti) {
            return ts < sti.ts;
        }
    };

    void get_prev_checksum(uint32_t block_index, uint8_t* out) {
        memset(out, 0, BLOCKCHAIN_CHECKSUM_SIZE);
        if (block_index != 0) {
            Tile t = tiles.get_tile(block_index - 1);
            if (t.get_state() == TileState::OK) {
                memcpy(out, t.get_sup()->checksum, 
                       BLOCKCHAIN_CHECKSUM_SIZE);
            } else 
                throw std::runtime_error("no checksum for block_index " + std::to_string(block_index));
        }
    }

    void move_to_bad(uint32_t block_index) {
        Timestamp ct = get_current_ts();
        std::string bfn = fnr.get_bad_file_name(ct, block_index);
        std::string fn = fnr.get_file_name(block_index);
        Poco::File pf(fn);
        pf.moveTo(bfn);
    }

    void calc_checksum(uint8_t* prev_checksum, Record* r, uint8_t* out) {
        sha_context_t sc;
        sha512_init(&sc);
        sha512_update(&sc, (unsigned char*)prev_checksum,
                      BLOCKCHAIN_CHECKSUM_SIZE);
        sha512_update(&sc, (unsigned char*)r, sizeof(Record));
        sha512_final(&sc, out);
    }

    bool validate_block(uint32_t block_index, Record* r, 
                        bool must_be_full, ExitCode& ec) {
        bool finished = false;
        bool has_empty = false;
        bool has_payload = false;
        for (int i = 0; i < RecCount; i++) {
            if (finished) {
                if (r[i].type != (uint8_t)RecordType::EMPTY) {
                    ec = NON_EMPTY_RECORD_AFTER_CHECKSUM;
                    return false;
                } else
                    has_empty = true;
            } else {
                if (r[i].type == (uint8_t)RecordType::PAYLOAD) 
                    has_payload = true;
                else if (r[i].type == (uint8_t)RecordType::EMPTY) {
                    ec = EMPTY_RECORD_BEFORE_CHECKSUM;
                    return false;
                }
                else if (r[i].type == (uint8_t)RecordType::CHECKSUM) {
                    if (!has_payload) {
                        ec = NO_PAYLOAD;
                        return false;
                    }
                    finished = true;
                }
            }
        }
        // TODO: check if empty record consists of 0
        if (!finished) {
            ec = NO_CHECKSUM;
            return false;
        }
        if (must_be_full && has_empty) {
            return EXPECTED_FULL_BLOCK;
            return false;
        }
        if (has_empty && block_index != tiles.get_tile_count() - 1) {
            return ONLY_LAST_BLOCK_MAY_NOT_BE_FULL;
            return false;
        }

        uint8_t checksum[BLOCKCHAIN_CHECKSUM_SIZE];
        get_prev_checksum(block_index, checksum);

        /* TODO: finish
        for (int i = 0; i < RecCount; i++) {
            if (r[i].type == (uint8_t)RecordType::CHECKSUM) {
                if (strncmp((char*)r[i].checksum,
                            (char*)checksum, 
                            BLOCKCHAIN_CHECKSUM_SIZE)) {
                    ec = BAD_CHECKSUM;
                    return false;
                }
                break;
            } else {
                calc_checksum(checksum, r + i, checksum);
            }
        }
        */
        ec = OK;
        return true;
    }

    uint32_t get_rec_count(Record* r) {
        uint32_t res = 0;
        for (int i = 0; i < RecCount; i++)
            if (r[i].type == (uint8_t)RecordType::EMPTY)
                break;
            else res++;
        return res;
    }


public:

    Blockchain(std::string name, std::string storage_root,
               uint16_t optimal_cache_size,
               uint16_t cache_auto_flush_min_interval_sec) : 
        optimal_cache_size(optimal_cache_size),
        cache_auto_flush_min_interval_sec(cache_auto_flush_min_interval_sec),
        fnr(name, storage_root), tiles(fnr), 
        checksum_valid_block_count(0),
        rec_count(0) {
        if constexpr (RecCount < 2)
            throw std::runtime_error("Blockchain: RecCount < 2");
    }

    // TODO: rec_count and validation

    bool append(const T& rec_payload, ExitCode& ec) {

        // TODO: transactional manner; should handle situations when
        // data is not written on hdd because of error, and roll back

        maybe_flush_cache();
        ec = OK;

        int block_pos = rec_count % RecCount;
        uint8_t prev_checksum[BLOCKCHAIN_CHECKSUM_SIZE];

        Record rec;
        rec.type = RecordType::PAYLOAD;
        rec.payload = rec_payload;

        Record cr;
        cr.type = RecordType::CHECKSUM;
        if (block_pos == 0) {
            if (tiles.get_tile_count() > 0) {
                Tile t0 = tiles.get_tile(tiles.get_tile_count() - 1);
                // flush_cache() would do it anyway; speeding things up
                t0.close_file();
            }
            get_prev_checksum(tiles.get_tile_count(), 
                              prev_checksum);
            calc_checksum(prev_checksum, &rec, cr.checksum);
            // checksum_valid_block_count advances with new block
            if (tiles.get_tile_count() == checksum_valid_block_count)
                checksum_valid_block_count++;
            Tile t = tiles.append_tile();
            t.alloc_tile_data();
            Record* r = t.get_tile_data();
            r[0] = rec;
            r[1] = cr;
            rec_count++;
            rec_count++;
            if (!t.sync_to_file(false)) {
                ec = WRITE_ERROR;
                return false;
            }
        } else {
            Tile t = tiles.get_tile(tiles.get_tile_count() - 1);
            memcpy(prev_checksum, 
                   t.get_tile_data()[block_pos - 1].checksum, 
                   BLOCKCHAIN_CHECKSUM_SIZE);
            calc_checksum(prev_checksum, &rec, cr.checksum);
            if (!t.write(&rec, block_pos - 1, false)) {
                ec = WRITE_ERROR;
                return false;
            } else {
                if (!t.write(&cr, block_pos, false)) {
                    ec = WRITE_ERROR;
                    return false;
                } else
                    rec_count++;
            }
        }
        return true;
    }

    // only works if last block is exactly full (or there are no blocks)
    bool append_block(Record* data, ExitCode& ec) {
        ec = OK;
        maybe_flush_cache();
        uint32_t tc = tiles.get_tile_count();

        if (tc == 0) {
            if (!validate_block(tc, data, false, ec))
                return false;
            Tile t = tiles.append_tile();
            t.set_tile_data(data);
            if (t.sync_to_file(true))
                return true;
            else {
                ec = WRITE_ERROR;
                return false;
            }
        } else {
            Tile t = tiles.get_tile(tc - 1);
            if (!t.sync_from_file()) {
                ec = READ_ERROR;
                return false;
            }
            Record* r = t.get_tile_data();
            if (r[RecCount - 1].type !=
                (uint8_t)RecordType::CHECKSUM) {
                ec = LAST_BLOCK_NOT_FULL;
                return false; 
            }
            if (!validate_block(tc, data, false, ec))
                return false;
            t = tiles.append_tile();
            t.set_tile_data(data);
            if (t.sync_to_file(true))
                return true;
            else {
                ec = WRITE_ERROR;
                return false;
            }
        }
        return false;
    }

    bool replace_block(uint32_t index, Record* data, ExitCode& ec) {
        maybe_flush_cache();
        ec = OK;

        if (index < tiles.get_tile_count()) {
            if (!validate_block(index, data, true, ec))
                return false;
            Tile t = tiles.get_tile(index);
            t.set_tile_data(data);
            if (!t.sync_to_file(true)) {
                ec = WRITE_ERROR;
                return false;
            }
        } else {
            ec = INDEX_TOO_LARGE;
            return false;
        }
        return true;
    }

    // attempts to fix where it is possible (moving corrupted blocks to
    // .bad files, etc.); throws exception when not possible to fix
    bool validate_next_checksum(ExitCode& ec) {
        ec = OK;
        bool res = false;
        maybe_flush_cache();

        if (checksum_valid_block_count < tiles.get_tile_count()) {
            Tile t = tiles.get_tile(checksum_valid_block_count);
            if (t.sync_from_file(true)) {
                Record* r = t.get_tile_data();
                if (validate_block(checksum_valid_block_count, r, 
                                   false, ec)) {
                    checksum_valid_block_count++;
                    rec_count += get_rec_count(r);
                    res = true;
                } else {
                    move_to_bad(checksum_valid_block_count);
                    res = false;
                }
            } else {
                if (t.get_state() == TileState::MISSING_FILE) {
                    // do nothing
                } else if (t.get_state() == TileState::BAD_LENGTH) {
                    ec = BAD_BLOCK_LENGTH;
                    move_to_bad(checksum_valid_block_count);
                    res = false;
                } else
                    throw std::runtime_error(
                               "bad block file, cannot recover, " + 
                               fnr.get_file_name(checksum_valid_block_count));
            }
        }
        return res;
    }
    uint32_t get_block_count() {
        return tiles.get_tile_count();
    }
    uint32_t get_checksum_valid_block_count() {
        return checksum_valid_block_count;
    }

    Record* get_record(uint64_t index, ExitCode& ec) {

        if (index >= rec_count) {
            ec = INDEX_TOO_LARGE;
            return 0;
        }

        uint32_t block_id = (uint32_t)(index / GRADIDO_BLOCK_SIZE);
        uint32_t rec_id = (uint32_t)(index % GRADIDO_BLOCK_SIZE);

        Record* res = get_block(block_id, ec);
        if (res) {
            ec = OK;
            res = res + rec_id;
        }
        return res;
    }

    Record* get_block(uint32_t index, ExitCode& ec) {
        maybe_flush_cache();

        ec = OK;

        Record* res = 0;
        Tile t = tiles.get_tile(index);

        // TODO: remove
        ;; t.sync_from_file(true);


        res = t.get_tile_data();

        if (!res) {
            if (t.sync_from_file(true)) {
                res = t.get_tile_data();
                // TODO: elaborate here; what if block cannot be 
                // open? need some type of errno
                if (!validate_block(index, res, false, ec))
                    res = 0;
            }
        }
        return res;
    }

    // returns true if block is locally valid (all previous blocks
    // have been validated, and checksum is good)
    bool is_block_valid(uint32_t index) {
        return index < checksum_valid_block_count;
    }

    void flush_cache() {
        std::vector<SortableTileIndex> sti;
        // i < .. - 1: reason is that tail is never uncached
        
        if (tiles.get_tile_count() > 0) {
            for (uint32_t i = 0; i < tiles.get_tile_count() - 1; i++) {
                Tile t = tiles.get_tile(i);
                if (t.get_tile_data()) {
                    TileSup* ts = t.get_sup();
                    sti.push_back(SortableTileIndex(i, ts->last_use));
                }
                t.close_file();
            }

            if (sti.size() > optimal_cache_size) {
                std::sort(sti.begin(), sti.end());
                for (int i = 0; i < sti.size() - optimal_cache_size; 
                     i++) {
                    Tile t = tiles.get_tile(sti[i].index);
                    t.release_fetched_data();
                }
            }

            next_cache_flush_timestamp = get_current_ts() + 
                cache_auto_flush_min_interval_sec;
        }
    }
    uint64_t get_rec_count() {
        return rec_count;
    }
    void truncate(uint32_t first) {
        tiles.truncate_tail(first);
    }
    
};    

// - allows additional validation to happen
// - includes multipart record related logics: counting, validating
template<typename T, typename Validator, int RecCount>
class ValidatedMultipartBlockchain {
private:
    Blockchain<T, RecCount> blockchain;
    Validator& validator;
public:
    using BType = Blockchain<T, RecCount>;
    using RecordType = typename BType::RecordType;
    using Record = typename BType::Record;
    using ExitCode = typename BType::ExitCode;
    
    ValidatedMultipartBlockchain(
               std::string name, std::string storage_root,
               uint16_t optimal_cache_size,
               uint16_t cache_auto_flush_min_interval_sec,
               Validator& validator) :
        blockchain(name, storage_root, optimal_cache_size, 
                   cache_auto_flush_min_interval_sec),
        validator(validator) {
    }

    bool append(const T* rec_payload, uint32_t rec_count, ExitCode& ec) {
        if (validator.is_valid(rec_payload, rec_count)) {
            for (uint32_t i = 0; i < rec_count; i ++)
                if (!blockchain.append(*(rec_payload + i), ec))
                    return false;
            return true;
        } else
            return false;
    }

    bool replace_block(uint32_t index, Record* data, ExitCode& ec) {
        return blockchain.replace_block(index, data, ec);
    }

    bool validate_next_checksum(ExitCode& ec) {
        return blockchain.validate_next_checksum(ec);
    }

    uint32_t get_block_count() {
        return blockchain.get_block_count();
    }
    uint32_t get_checksum_valid_block_count() {
        return blockchain.get_checksum_valid_block_count();
    }

    Record* get_block(uint32_t index, ExitCode& ec) {
        return blockchain.get_block(index, ec);
    }

    bool is_block_valid(uint32_t index) {
        return blockchain.is_block_valid(index);
    }
    void flush_cache() {
        return blockchain.flush_cache();
    }
    uint64_t get_rec_count() {
        return blockchain.get_rec_count();
    }
    void truncate(uint32_t first) {
        return blockchain.truncate(first);
    }
    Record* get_record(uint64_t index, ExitCode& ec) {
        return blockchain.get_record(index, ec);
    }
};
 
}

#endif
