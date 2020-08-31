#include "blockchain_gradido_def.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <algorithm>
#include "blockchain.h"
#include <Poco/Exception.h>

using namespace gradido;

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// TODO: maybe should merge with dump_blockchain
int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Utility for counting records in a Gradido blockchain" << 
            std::endl << "Usage: " << std::endl << "dump_blockchain <blockchain_folder>" << std::endl;
        return -1;
    }

    typedef Blockchain<GradidoRecord>::Record grec;

    try {

        std::string bf(argv[1]);
        Poco::Path block_root_name_path(bf);
        Poco::File block_root = Poco::File(block_root_name_path.absolute());

        std::vector<std::string> blocks;
        for (Poco::DirectoryIterator it(block_root);
             it != Poco::DirectoryIterator{}; ++it) {
            std::string ps = it.path().toString();
            if (ends_with(ps, ".block")) {
                Poco::File f(ps);
                if (f.getSize() == 0 || 
                    f.getSize() != GRADIDO_BLOCK_SIZE * sizeof(grec))
                    throw std::runtime_error(
                          "not a block file (bad size): " + ps);

                blocks.push_back(ps);
            }
        }
        std::sort(blocks.begin(), blocks.end());
        uint64_t rec_count = 0;

        for (std::string i : blocks) {
            std::ifstream fs(i, std::ios::binary);
            for (int i = 0; i < GRADIDO_BLOCK_SIZE; i++) {
                grec r;
                fs.read((char*)&r, sizeof(grec));
                if (r.hash_version > 0) {
                    rec_count++;
                } else 
                    break;
            }
            fs.close();
        }
        std::cout << rec_count << std::endl;
    } catch (Poco::Exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
    return 0;
}    
