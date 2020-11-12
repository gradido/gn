#ifndef FILE_TILE_H
#define FILE_TILE_H

#include <vector>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "memory_managers.h"

namespace gradido {

    // imaginary tile array
    // - each tile has an index, tile data is stored on hdd (one file 
    //   per tile)
    // - files all are equal size TCount * sizeof(T)
    // - their names are acquired from FileNameResolver
    // - each tile has a state (whether file exists, is loaded in RAM, 
    //   is broken, etc.)
    // - number of tiles is determined by number of files
    // - tiles can be replaced
    // - tile list can be truncated, new tiles are added to end of the
    //   list
    // - files are expected to be readable and writeable
    //
    // main idea is to allow user class to optimize things in various
    // ways, by allowing low-level access to data, still enforcing few
    // important invariances
    // 
    // - FileNameResolver methods
    //   - get_file_name(uint32_t)
    //   - get_file_count()
    template<typename T, typename FileNameResolver, int TCount,
        typename Sup>
    class FileTile {
    public:

        enum TileState {
            OK=0,
            MISSING_FILE,
            NOT_A_FILE,
            BAD_LENGTH,
            NOT_READABLE,
            NOT_WRITEABLE,
            CANNOT_OPEN_FILE,
            CANNOT_CLOSE_FILE,
            ERROR_WHILE_READING,
            ERROR_WHILE_WRITING,
            CANNOT_SYNC_NO_RAW_DATA,
            FSEEK_FAILED,
            REC_INDEX_EXCEEDS_BOUNDS,
            TRUNCATED,
            BAD_INDEX
        };

    private:
        static const size_t mem_pool_block_size = 100;
        static const size_t tile_block_size = 1000;
        static const size_t realloc_prevention_factor = 2;
        struct TileDesc {
            uint32_t index;
            // describes outcome of last operation; not used to check
            // if an operation is possible (no checks from inside of
            // FileTile at all)
            TileState state;
            T* fetched_data;
            FILE* file_handle;
            Sup sup;

            void reset(uint32_t ind) {
                index = ind;
                state = OK;
                fetched_data = 0;
                file_handle = 0;
                sup.reset();
            }
            TileDesc() : fetched_data(0), file_handle(0), index(0) {}
        };

        static TileState check_file2(const std::string& fname) {
            const char* fn = fname.c_str();
            struct stat fs;
            stat(fn, &fs);

            if (access(fn, F_OK))
                return MISSING_FILE;
            else if (!S_ISREG(fs.st_mode))
                return NOT_A_FILE;
            else if (fs.st_size != sizeof(T) * TCount)
                return BAD_LENGTH;
            else if (access(fn, R_OK))
                return NOT_READABLE;
            else if (access(fn, W_OK))
                return NOT_WRITEABLE;
            return OK;
        }

        PermanentBlockAllocator<TileDesc> tile_allocator;
        std::vector<TileDesc*> unused_tiles;
        std::vector<TileDesc*> tiles;
        const FileNameResolver& fnr;
        BigObjectMemoryPool<T, TCount> mem_pool;

        TileDesc* create_tile_desc(uint32_t index) {
            if (unused_tiles.size() == 0) {
                TileDesc* td;
                tile_allocator.allocate(&td);
                unused_tiles.reserve(tile_allocator.block_size * 
                                     realloc_prevention_factor);
                for (size_t i = 0; i < tile_allocator.block_size; i++) {
                    unused_tiles.push_back(td + i);
                }
            }
            TileDesc* td = unused_tiles[unused_tiles.size() - 1];
            td->reset(index);

            unused_tiles.pop_back();
            return td;
        }

    public:

        // copyable, in cheap manner
        class Tile {
        private:
            TileDesc* desc;
            const FileNameResolver* fnr;
            BigObjectMemoryPool<T, TCount>* mp;
        public:
            Tile(TileDesc* desc, const FileNameResolver* fnr,
                 BigObjectMemoryPool<T, TCount>* mp) : 
            desc(desc), fnr(fnr), mp(mp) {}
            // open / close are accessible for accessing records 
            // separately, with read() / write(); assuming, that 
            // preferred state of file is closed
            bool open_file() {
                if (!desc->file_handle) {
                    std::string fname = fnr->get_file_name(desc->index);
                    TileState ts = check_file2(fname);

                    if (ts == MISSING_FILE)
                        desc->file_handle = fopen(fname.c_str(), "wb+");
                    else if (ts == OK)
                        desc->file_handle = fopen(fname.c_str(), "rb+");
                    if (!desc->file_handle) {
                        desc->state = CANNOT_OPEN_FILE;
                        return false;
                    } 
                }
                desc->state = OK;
                return true;
            }
            void close_file() {
                if (desc->file_handle) {
                    int res = fclose(desc->file_handle);
                    if (res) {
                        desc->state = CANNOT_CLOSE_FILE;
                        return;
                    }
                    desc->file_handle = 0;
                }
                desc->state = OK;
            }
            // writes into file and fetched (if fetched is present)
            bool write(T* rec, uint32_t rec_index, 
                       bool close_on_exit) {

                if (rec_index >= TCount) {
                    desc->state = REC_INDEX_EXCEEDS_BOUNDS;
                    return false;
                }

                if (open_file()) {
                    int res = fseek(desc->file_handle, 
                                    sizeof(T) * rec_index, 
                                    SEEK_SET);
                    if (res) {
                        desc->state = FSEEK_FAILED;
                        close_file();
                        return false;
                    }

                    size_t res2 = fwrite(rec, 
                                         sizeof(T), 
                                         1,
                                         desc->file_handle);

                    res = fseek(desc->file_handle, 
                                0, 
                                SEEK_SET);
                    if (res) {
                        desc->state = FSEEK_FAILED;
                        close_file();
                        return false;
                    }
                    if (res2 != 1) {
                        desc->state = ERROR_WHILE_WRITING;
                        close_file();
                        return false;
                    }
                    if (close_on_exit)
                        close_file();
                    return true;
                } else 
                    return false;
            }
            // takes from fetched; if not fetched, then fetches
            T* read(uint32_t rec_index) {
                if (rec_index >= TCount) {
                    desc->state = REC_INDEX_EXCEEDS_BOUNDS;
                    return false;
                }

                if (!desc->fetched_data)
                    if (!sync_from_file(true))
                        return 0;
                return desc->fetched_data + rec_index;
            }
            bool sync_to_file(bool close_on_exit) {
                if (desc->fetched_data) {
                    if (open_file()) {
                        size_t res = fwrite((void*)desc->fetched_data, 
                                            sizeof(T), 
                                            TCount,
                                            desc->file_handle);
                        if (res != TCount) {
                            desc->state = ERROR_WHILE_WRITING;
                            return false;
                        } else
                            desc->state = OK;
                        if (close_on_exit)
                            close_file();
                        return true;
                    }
                } else {
                    desc->state = CANNOT_SYNC_NO_RAW_DATA;
                    return false;
                }
            }
            bool sync_from_file(bool close_on_exit) {
                if (open_file()) {
                    if (!desc->fetched_data)
                        desc->fetched_data = mp->get();

                    size_t res = fread(desc->fetched_data,
                                       sizeof(T),
                                       TCount,
                                       desc->file_handle);
                    if (res != TCount) {
                        desc->state = ERROR_WHILE_READING;
                        return false;
                    }
                } else
                    return false;
                desc->state = OK;
                return true;
            }
            TileState get_state() {
                return desc->state;
            }
            T* get_tile_data() { 
                desc->state = OK;
                return desc->fetched_data;
            }

            void alloc_tile_data() {
                if (!desc->fetched_data)
                    desc->fetched_data = mp->get();
            }

            void set_tile_data(const T* data) { 
                if (!desc->fetched_data)
                    desc->fetched_data = mp->get();
                // TODO: consider allowing mp to take ownership of
                // data, then copy won't be needed
                memcpy(desc->fetched_data, data, sizeof(T) * TCount);
                desc->state = OK;
            }
            void check_file() {
                desc->state = check_file(
                              fnr->get_file_name(desc->index));
            }
            void release_fetched_data() {
                if (desc->fetched_data) {
                    mp->release(desc->fetched_data);
                    desc->fetched_data = 0;
                }
                desc->state = OK;
            }
            Sup* get_sup() {
                return &desc->sup;
            }
        };

        FileTile(const FileNameResolver& fnr) : fnr(fnr), 
            mem_pool(mem_pool_block_size), 
            tile_allocator(tile_block_size) {
            uint32_t fc = fnr.get_file_count();
            for (uint32_t i = 0; i < fc; i++) {
                std::string fn = fnr.get_file_name(i);
                TileDesc* ts = create_tile_desc(i);
                ts->state = check_file2(fn);
                tiles.push_back(ts);
            }
        }
        ~FileTile() {
            for (auto i : tiles) {
                if (i->fetched_data)
                    mem_pool.release(i->fetched_data);
                if (i->file_handle)
                    fclose(i->file_handle);
            }
        }

        uint32_t get_tile_count() {
            return tiles.size();
        }

        Tile get_tile(uint32_t index) {
            if (index >= tiles.size())
                throw std::runtime_error("TileFile: index too large");
            Tile res(tiles[index], &fnr, &mem_pool);
            return res;
        }

        void truncate_tail(uint32_t first) {
            if (first >= tiles.size())
                return;
            uint32_t cnt = tiles.size() - first;
            unused_tiles.reserve(cnt * realloc_prevention_factor);
            for (uint32_t i = first; i < tiles.size(); i++) {
                TileDesc* td = tiles[i];
                td->state = TRUNCATED;
                unused_tiles.push_back(td);
            }
            tiles.resize(first);
        }

        Tile append_tile() {
            uint32_t i = tiles.size();
            TileDesc* ts = create_tile_desc(i);
            ts->state = OK;
            tiles.push_back(ts);
            Tile res(ts, &fnr, &mem_pool);
            return res;
        }
    };
}

#endif
