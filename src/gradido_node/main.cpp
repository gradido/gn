#include "gradido_facade.h"
#include <iostream>
#include <string>
#include <vector>
#include "gradido_signals.h"

using namespace gradido;

int main(int argc, char** argv) {
    GradidoFacade gf;
    GradidoSignals::init(&gf);
    std::vector<std::string> params;

    for (int i = 0; i < argc; i++)
        params.push_back(std::string(argv[i]));

    try {
        LOG("starting gradido node");
        gf.init(params);
        gf.join();
    } catch (Poco::Exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }
}    
