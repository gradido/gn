#ifndef DUMPER_H
#define DUMPER_H

#include <fstream>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <algorithm>
#include "blockchain.h"
#include <Poco/Exception.h>
#include "blockchain_gradido_def.h"
#include "utils.h"

namespace gradido {

template<typename T>
int dump(int argc, char** argv, std::string type_desc) {

    using RecType = typename BlockchainTypes<T>::Record;

    if (argc < 2) {
        std::cerr << "Utility for dumping " << type_desc << 
            std::endl << "Usage: " << std::endl << "dump_blockchain <blockchain_folder> <options>" << 
            std::endl << "options:" << 
            std::endl << "  -c: returns number of records" << std::endl;
        return -1;
    }

    try {

        std::string bf(argv[1]);
        Poco::Path block_root_name_path(bf);
        Poco::File block_root = Poco::File(block_root_name_path.absolute());

        bool just_count = false;
        for (int i = 2; i < argc; i++) {
            std::string opt(argv[i]);
            if (opt.compare("-c") == 0)
                just_count = true;
        }

        std::vector<std::string> blocks;
        for (Poco::DirectoryIterator it(block_root);
             it != Poco::DirectoryIterator{}; ++it) {
            std::string ps = it.path().toString();
            if (ends_with(ps, ".block")) {
                Poco::File f(ps);
                if (f.getSize() == 0 || 
                    f.getSize() != GRADIDO_BLOCK_SIZE * sizeof(RecType))
                    throw std::runtime_error(
                          "not a block file (bad size): " + ps);

                blocks.push_back(ps);
            }
        }
        std::sort(blocks.begin(), blocks.end());

        uint64_t rec_count = 0;

        if (!just_count)
            std::cout << "[" << std::endl;

        for (std::string i : blocks) {
            std::ifstream fs(i, std::ios::binary);
            bool is_first = true;

            for (int i = 0; i < GRADIDO_BLOCK_SIZE; i++) {
                RecType r;
                fs.read((char*)&r, sizeof(RecType));
                if (r.type > 0) {
                    rec_count++;
                    if (!just_count) {
                        if (is_first)
                            is_first = false;
                        else 
                            std::cout << ",\n";
                        dump_transaction_in_json<T>(r, std::cout);
                    }
                } else 
                    break;
            }
            fs.close();
        }
        if (just_count)
            std::cout << rec_count << std::endl;
        else
            std::cout << "]" << std::endl;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

}

#endif
