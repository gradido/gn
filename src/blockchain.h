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


namespace gradido {

// - thread agnostic
// - validates in batches
// - currently doesn't cache anything
// - if records first byte is \0, then it is considered blank
// - multi-part record validation is supported
template<typename T>
class Blockchain final {
public:

    enum RecordValidationResult {
        INVALID=0,
        VALID,
        NEED_NEXT
    };

    class BlockchainRecordValidator {
    public:
        virtual RecordValidationResult validate(const T& rec) = 0;
        virtual void added_successfuly(const T& rec) = 0;
    };

    struct Record {
        // if \0, will be considered blank
        uint8_t hash_version; // starts with 1
        T payload;        
        uint8_t hash[SHA_512_SIZE];
    };
private:
    std::vector<Record> pending_insertions;

    
private:

    class ActiveBlock final {
    private:
        uint64_t block_num;
        std::fstream file;
    public:
        ActiveBlock(uint64_t block_num, std::string fname) :
        block_num(block_num) {
            Poco::File bf(fname);
            if (!bf.isFile()) 
                throw std::runtime_error("non-existent block file " + fname);

            file.open(fname, std::ios::binary | 
                      std::ios::in | std::ios::out);
        }

    ActiveBlock(uint64_t block_num, std::string fname, uint64_t rec_per_block) :
        block_num(block_num) {
            Poco::File bf(fname);
            if (!bf.exists() || bf.getSize() == 0) {
                std::ofstream ff(fname, std::ios::binary | std::ios::out);
                Record empty = {0};
                for (int i = 0; i < rec_per_block; i++)
                    ff.write((char*)&empty, sizeof(Record));
                ff.close();
            }
            file.open(fname, std::ios::binary | 
                      std::ios::in | std::ios::out);
        }
        ~ActiveBlock() {
            file.close();
        }
        
        uint64_t get_block_num() {
            return block_num;
        }

        Record get_rec(uint64_t rec_pos) {
            uint64_t pos = rec_pos * sizeof(Record);

            file.seekg(pos, std::ios::beg);
            char buff[sizeof(Record)];
            if (!file.read(buff, sizeof(Record)))
                throw std::runtime_error("couldn't read block:rec_pos " +
                                         std::to_string(block_num) + ":" +
                                         std::to_string(rec_pos));
            Record res = *((Record*)buff);
            return res;
        }
        void write_rec(const Record& rec, uint64_t rec_pos) {
            uint64_t pos = rec_pos * sizeof(Record);
            file.seekp(pos, std::ios::beg);
            file.write((char*)&rec, sizeof(Record));
            file.flush();
        }
    };

    std::string name;
    Poco::Path storage_root;
    uint64_t rec_per_block;
    Poco::File block_root;
    uint64_t block_size;
    uint64_t total_rec_count;
    ActiveBlock* tail;
    BlockchainRecordValidator* validator;
    Record last_rec;

    void free_tail() {
        if (tail) {
            delete tail;
            tail = 0;
        }
    }
    
    void ensure_tail_points_to_upcoming_rec() {
        uint64_t bnum = (uint64_t)(total_rec_count / block_size);
        if (!tail || tail->get_block_num() != bnum) {
            free_tail();
            std::string bname = create_block_file_name(bnum);
            tail = new ActiveBlock(bnum, bname, rec_per_block);
        }
    }

    std::string create_block_file_name(uint64_t num) {
        Poco::Path tmp = storage_root;
        tmp.append(name + "." + std::to_string(num) + ".block");
        return tmp.absolute().toString();
    }

    Record get_full_rec(uint64_t rec_num) {
        // TODO: ofc, caching
        uint64_t block_num = rec_num / rec_per_block;
        uint64_t rec_pos = rec_num - block_num * rec_per_block;
        if (tail && tail->get_block_num() == block_num) {
            Record res = tail->get_rec(rec_pos);
            return res;
        } else {
            std::string fname = create_block_file_name(block_num);
            ActiveBlock ab(block_num, fname);
            return ab.get_rec(rec_pos);
        }
    }

    RecordValidationResult validate(const Record* curr_rec, 
                                    const Record* last_rec) {
        sha_context_t sc;
        sha512_init(&sc);
        sha512_update(&sc, (unsigned char*)last_rec,
                      sizeof(Record));
        sha512_update(&sc, (unsigned char*)&curr_rec->payload,
                      sizeof(T));

        unsigned char ch[SHA_512_SIZE];
        sha512_final(&sc, ch);

        if (strncmp((char*)curr_rec->hash, (char*)ch, SHA_512_SIZE))
            return INVALID;
        return validator->validate(curr_rec->payload);
    }

    void prepare_rec(Record* rec, Record* last_rec) {
        rec->hash_version = 1;
        sha_context_t sc;
        sha512_init(&sc);
        sha512_update(&sc, (unsigned char*)last_rec, sizeof(Record));
        sha512_update(&sc, (unsigned char*)&rec->payload, sizeof(T));
        sha512_final(&sc, rec->hash);
    }

public:

    Blockchain(std::string name, Poco::Path storage_root,
               uint64_t rec_per_block,
               BlockchainRecordValidator* validator) :
        name(name),
        storage_root(storage_root),
        rec_per_block(rec_per_block),
        block_size(sizeof(Record) * rec_per_block),
        total_rec_count(0),
        tail(0),
        validator(validator),
        last_rec{0} {
            if (!validator)
                throw std::runtime_error("provide validator to Blockchain");
            block_root = Poco::File(storage_root);
        }
    
    
    ~Blockchain() {
        free_tail();
    }

    int validate_next_batch(int amount) {
        int i = 0;
        for (i = 0; i < amount; i++) {
            int cnt = 0;
            Record tmp = last_rec;
            while (true) {
                ensure_tail_points_to_upcoming_rec();            
                Record rec = tail->get_rec(total_rec_count);
                RecordValidationResult zz = validate(&rec, &tmp);
                if (zz == VALID) {
                    total_rec_count += cnt + 1;
                    last_rec = rec;
                    validator->added_successfuly(last_rec.payload);
                    break;
                } else if (zz == NEED_NEXT) {
                    tmp = rec;
                    total_rec_count++;
                    cnt++;
                } else {
                    total_rec_count -= cnt;
                    ensure_tail_points_to_upcoming_rec();            
                    return i;
                }
            }
        }
        return i;
    }

    bool append(const T& payload) {
        append(payload, 0);
    }

    bool append(const T& payload, char* hash_to_verify) {

        RecordValidationResult zz = validator->validate(payload);
        if (zz == INVALID) {
            pending_insertions.clear();
            return false;
        }
        
        Record rec;
        rec.payload = payload;
        Record* lr = pending_insertions.size() > 0 ? &pending_insertions.back() : &last_rec;
        prepare_rec(&rec, lr);
        if (hash_to_verify && strncmp((char*)rec.hash, hash_to_verify, 
                                      SHA_512_SIZE)) {
            pending_insertions.clear();
            return false;
        }

        if (zz == NEED_NEXT) {
            pending_insertions.push_back(rec);
            return true;
        } else {
            for (auto i : pending_insertions) {
                ensure_tail_points_to_upcoming_rec();
                tail->write_rec(i, total_rec_count++);
                validator->added_successfuly(i.payload);
            }
            ensure_tail_points_to_upcoming_rec();
            tail->write_rec(rec, total_rec_count++);
            validator->added_successfuly(last_rec.payload);
            last_rec = rec;
            pending_insertions.clear();
            return true;
        }
    }

    uint64_t get_rec_count() {
        return total_rec_count;
    }

    T get_rec(uint64_t rec_num) {
        Record rec = get_full_rec(rec_num);
        return rec.payload;
    }    

    T get_rec(uint64_t rec_num, std::string& hash) {
        Record rec = get_full_rec(rec_num);
        hash = std::string((char*)rec.hash, SHA_512_SIZE);
        return rec.payload;
    }    
};
    
 
}

#endif
