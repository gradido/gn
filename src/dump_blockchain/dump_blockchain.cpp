#include "utils.h"
#include "dumper.h"

using namespace gradido;

int main(int argc, char** argv) {
    return dump<GradidoBlockchainType>(argc, argv, "Gradido blockchains");
}    
