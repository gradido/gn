#ifndef MAIN_DEF_H
#define MAIN_DEF_H

#include <vector>
#include "facades_impl.h"
#include "gradido_signals.h"

#define GET_PARAMS                              \
    std::vector<std::string> params;            \
    for (int i = 0; i < argc; i++)              \
        params.push_back(std::string(argv[i])); 

#define SHOW_VERSION                                                \
    for (auto i : params) {                                         \
        if (i.compare("--v") == 0 || i.compare("-version") == 0) {  \
            init_logging(false, true, false);                       \
            return 0;                                               \
        }                                                           \
    }


#define GRADIDO_NODE_MAIN(FacadeClass, NodeType)    \
                                                    \
    using namespace gradido;                        \
                                                    \
    int main(int argc, char** argv) {               \
        GET_PARAMS;                                 \
        SHOW_VERSION;                               \
        init_logging(true, true, true);             \
        FacadeClass gf;                             \
        GradidoSignals::init(&gf);                  \
                                                    \
        try {                                       \
            LOG("starting "                         \
                #NodeType  " node");                \
            gf.init(params);                        \
            gf.join();                              \
        } catch (Poco::Exception& e) {              \
            LOG(e.what());                          \
            return 1;                               \
        } catch (std::runtime_error& e) {           \
            LOG(e.what());                          \
            return 2;                               \
        }                                           \
        return 0;                                   \
    }                                               

#define GRADIDO_CMD_UTIL                        \
    GET_PARAMS;                                 \
    SHOW_VERSION;                               \
    init_logging(false, false, false);

#endif
