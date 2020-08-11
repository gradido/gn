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


namespace gradido {

template<typename T>
class Blockchain {
public:

    // TODO: batch validation?...
    class BlockchainRecordValidator {
    public:
        virtual void validate(uint64_t rec_num, const T& rec) = 0;
    };
    
private:

    // TODO: add caching

    struct Record {
        T payload;        
        // TODO: consider uint8_t
        unsigned char hash[SHA_512_SIZE];
        unsigned char hash_type; // starts with 1
    };

    // TODO: constexpr
    std::string name;
    std::string storage_root;
    uint64_t rec_per_block;
    size_t record_size;
    size_t payload_size;
    uint64_t block_size;
    Poco::Path block_root_name_path;
    Poco::File block_root;
    uint64_t block_count;
    uint64_t total_rec_count;
    BlockchainRecordValidator* validator;

    class ActiveBlock final {
    private:
        uint64_t block_num;
        std::fstream file;
    public:
        ActiveBlock(uint64_t block_num, std::string fname) :
            block_num(block_num),
            file(std::fstream(fname, std::ios::binary | std::ios::in | std::ios::out)) {}
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

    ActiveBlock* tail;
    void free_tail() {
        if (tail) {
            delete tail;
            tail = 0;
        }
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

    std::string create_block_file_name(uint64_t num) {
        Poco::Path tmp = block_root_name_path;
        tmp.append(name + "." + std::to_string(num) + ".block");
        return tmp.absolute().toString();
    }

    void validate_all_blocks() {
        // finding max index
        Poco::RegularExpression re("^" + name + ".([\\d]+).block$");
        for (Poco::DirectoryIterator it(block_root);
             it != Poco::DirectoryIterator{}; ++it) {

            Poco::File curr(it.path());
            if (curr.isFile()) {
                std::string fname = it.path().getFileName();
                std::vector<std::string> ss;
                if (re.split(fname, ss)) {
                    uint64_t block_num = static_cast<uint64_t>(std::stoul(ss[1]));
                    // TODO: after cast it may overlap
                    if (std::to_string(block_num).compare(ss[1]) != 0)
                        throw std::runtime_error("bad block number for " + fname);
                    if (block_count <= block_num)
                        block_count = block_num + 1;
                }
            }
        }

        // first look on files: do they exist and if their sizes are good
        std::vector<uint64_t> missing_blocks;
        std::vector<uint64_t> corrupted_blocks;
        for (uint64_t i = 0; i < block_count; i++) {
            std::string fname = create_block_file_name(i);
            Poco::File bfile(fname);
            if (!bfile.exists() || ! bfile.isFile())
                missing_blocks.push_back(i);
            else if (bfile.getSize() != block_size)
                corrupted_blocks.push_back(i);
        }

        // going through blockchain, validating hashes
        if (!missing_blocks.size() && !corrupted_blocks.size()) {
            unsigned char buff[block_size];
            BlockValidator bv(rec_per_block);
            for (uint64_t i = 0; i < block_count; i++) {
                // TODO: inspect implications of cast
                read_block_into_buffer(i, (char*)buff);
                total_rec_count += bv.validate(buff, i, validator);
            }
        }

        if (missing_blocks.size() || corrupted_blocks.size()) {
            // TODO: dump those into cerr
            throw std::runtime_error("missing / corrupted blocks");
        } 
    }

    void init_next_block() {
        std::string fname = create_block_file_name(block_count);

        std::ofstream file(fname, std::ios::binary | std::ios::out);
        Record zero_rec = {0};
        for (int i = 0; i < rec_per_block; i++)
            file.write((char*)&zero_rec, record_size);

        file.close();
        block_count++;
    }

    void read_block_into_buffer(uint64_t block_num, char* buff) {
        std::string fname = create_block_file_name(block_num);
        std::ifstream file(fname, std::ios::binary | std::ios::in);

        // TODO: RAII close files on exit
        
        if (!file.read(buff, block_size))
            throw std::runtime_error("couldn't load " + fname);
        file.close();
    }

    class BlockValidator {
    private:
        Record last_rec;
        bool is_first;
        bool end_padding_reached;
        size_t record_size;
        size_t payload_size;
        uint64_t rec_per_block;
    public:
        BlockValidator(uint64_t rec_per_block) :
            last_rec({0}),
            is_first(true), end_padding_reached(false),
            record_size(sizeof(Record)),
            payload_size(sizeof(T)),
            rec_per_block(rec_per_block) {
        }
        uint64_t validate(const unsigned char* buff, uint64_t block_num,
                          BlockchainRecordValidator* validator) {
            uint64_t res = 0;
            for (int i = 0; i < rec_per_block; i++) {
                Record curr_rec = *((Record*)(buff) + i);
                if (!curr_rec.hash_type)
                    end_padding_reached = true;
                else {
                    if (end_padding_reached)
                        throw std::runtime_error("end padding reached before block:record " + std::to_string(block_num) + ":" + std::to_string(i));

                    sha_context_t sc;
                    sha512_init(&sc);

                    if (is_first) {
                        sha512_update(&sc, (unsigned char*)&last_rec,
                                      record_size);
                        is_first = false;
                    } else
                        sha512_update(&sc, (unsigned char*)&last_rec,
                                      record_size);
                    sha512_update(&sc, (unsigned char*)&curr_rec.payload,
                                  payload_size);

                    unsigned char ch[SHA_512_SIZE];
                    sha512_final(&sc, ch);

                    if (strncmp((char*)curr_rec.hash, (char*)ch, SHA_512_SIZE))
                        throw std::runtime_error("bad hash at block:record " + std::to_string(block_num) + ":" + std::to_string(i));

                    validator->validate(block_num * rec_per_block + i,
                                        curr_rec.payload);
                    last_rec = curr_rec;
                    res++;
                }
            }
            return res;
        }
    };
    
   
public:
    // if underlying data is corrupted, exception is thrown
    Blockchain(std::string name, std::string storage_root,
               uint64_t rec_per_block,
               BlockchainRecordValidator* validator) :
        name(name),
        storage_root(storage_root),
        rec_per_block(rec_per_block),
        record_size(sizeof(Record)),
        payload_size(sizeof(T)),
        block_size(record_size * rec_per_block),
        block_count(0),
        total_rec_count(0),
        tail(0),
        validator(validator)
    {
        if (!validator)
            throw std::runtime_error("provide validator to Blockchain");

        Poco::Path sr_path = Poco::Path(storage_root).absolute();
        block_root_name_path = sr_path.append(name);
        block_root = Poco::File(block_root_name_path);
    }
    void init() {
        if (!block_root.exists()) {
            block_root.createDirectories();
            init_next_block();
        } else {
            // TODO: validate payload
            validate_all_blocks();
            if (block_count == 0)
                init_next_block();            
        }
        // tail always has the block ready to insert
        uint64_t block_num = total_rec_count / rec_per_block;
        std::string fname = create_block_file_name(block_num);
        tail = new ActiveBlock(block_num, fname);
    }
    
    ~Blockchain() {
        free_tail();
    }

    void append(const T& payload) {
        uint64_t block_num = total_rec_count / rec_per_block;
        uint64_t rec_pos = total_rec_count - (block_num * rec_per_block);

        // TODO: remove this; strong assumption is that payload is
        // already validated
        //validator->validate(total_rec_count, payload);

        Record prev_rec = {0};
        if (total_rec_count > 0)
            prev_rec = get_full_rec(total_rec_count - 1);
        Record new_rec;
        new_rec.payload = payload;        
        new_rec.hash_type = 1;
        sha_context_t sc;
        sha512_init(&sc);
        sha512_update(&sc, (unsigned char*)&prev_rec, record_size);
        sha512_update(&sc, (unsigned char*)&payload, payload_size);
        sha512_final(&sc, new_rec.hash);
        tail->write_rec(new_rec, rec_pos);
        total_rec_count++;
        if (rec_pos + 1 == rec_per_block) {
            // last record inserted in the block, preparing next one
            free_tail();
            init_next_block();            
            std::string fname = create_block_file_name(block_num);
            tail = new ActiveBlock(block_num, fname);
        }
    }

    uint64_t get_rec_count() {
        return total_rec_count;
    }

    T get_rec(uint64_t rec_num) {
        return get_full_rec(rec_num).payload;
    }
    
};
    
 
}

#endif
