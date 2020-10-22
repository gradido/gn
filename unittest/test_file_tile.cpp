#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include "test_common.h"
#include "file_tile.h"

using namespace gradido;

#define FILE_TILE_FOLDER "/tmp/test-ft"
#define SIMPLE_REC_CNT 5

class Fnr {
private:
    std::string folder;
    Poco::File poco_folder;

public:
    Fnr(std::string folder) : folder(folder), poco_folder(folder) {
    }

    uint32_t get_file_count() const {
        Poco::DirectoryIterator it(folder);
        uint32_t res = 0;
        for (Poco::DirectoryIterator it(folder);
                 it != Poco::DirectoryIterator{}; ++it)
            res++;
        return res;
    }

    std::string get_file_name(uint32_t index) const {
        return folder + "/ft." + std::to_string(index);
    }
};

class EmptySup {
public:
    void reset() {}
};

TEST(FileTile, create_tile) {
    erase_tmp_folder(FILE_TILE_FOLDER);

    typedef FileTile<SimpleRec, Fnr, SIMPLE_REC_CNT, EmptySup> FtType;

    Poco::File ff(FILE_TILE_FOLDER);
    ff.createDirectories();
    
    for (int i = 0; i < 2; i++) {
        std::string fname = std::string(FILE_TILE_FOLDER) + "/ft." + 
            std::to_string(i);
        FILE* f = fopen(fname.c_str(), "wb");
        SimpleRec recs[SIMPLE_REC_CNT];
        for (int j = 0; j < SIMPLE_REC_CNT; j++) {
            recs[j].index = i * 1000 + j;
        }
        fwrite(recs, sizeof(SimpleRec), SIMPLE_REC_CNT, f);
        fclose(f);
    }

    Fnr fnr(FILE_TILE_FOLDER);
    FtType ft(fnr);

    ASSERT_EQ(ft.get_tile_count() == 2, true);

    FtType::Tile t = ft.get_tile(0);
    ASSERT_EQ(t.get_state(), FtType::OK);
    ASSERT_EQ(t.sync_from_file(false), true);
    ASSERT_EQ(t.get_state(), FtType::OK);
    SimpleRec* data = t.get_tile_data();
    
    ASSERT_EQ(data != 0, true);
    ASSERT_EQ(data[1].index == 1, true);
    ASSERT_EQ(data[2].index == 2, true);
}
