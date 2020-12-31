#include <vector>
#include "facades_impl.h"
#include "gradido_signals.h"

#define GRADIDO_NODE_MAIN(FacadeClass, NodeType)    \
                                                    \
    using namespace gradido;                        \
                                                    \
    int main(int argc, char** argv) {               \
        FacadeClass gf;                             \
        GradidoSignals::init(&gf);                  \
        std::vector<std::string> params;            \
                                                    \
        for (int i = 0; i < argc; i++)              \
            params.push_back(std::string(argv[i])); \
                                                    \
        try {                                       \
            LOG("starting "                         \
                #NodeType  " node");                \
            gf.init(params);                        \
            gf.join();                              \
        } catch (Poco::Exception& e) {              \
            std::cerr << e.what() << std::endl;     \
            return 1;                               \
        } catch (std::runtime_error& e) {           \
            std::cerr << e.what() << std::endl;     \
            return 2;                               \
        }                                           \
        return 0;                                   \
    }                                               
