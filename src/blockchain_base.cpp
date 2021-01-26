#include "blockchain_base.h"

namespace gradido {

std::string ensure_blockchain_folder(const Poco::Path& sr,
                                     std::string name) {
    Poco::Path p0(sr);
    std::string blockchain_folder = name + ".bc";
    p0.append(blockchain_folder);

    Poco::File srf(p0.absolute());
    if (!srf.exists())
        srf.createDirectories();
    if (!srf.exists() || !srf.isDirectory())
        PRECISE_THROW(name + ": cannot create folder");
    return p0.absolute().toString();
}

}
