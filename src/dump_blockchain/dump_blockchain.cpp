#include "utils.h"
#include "dumper.h"

using namespace gradido;

int main(int argc, char** argv) {
    using Record = typename BlockchainTypes<GradidoRecord>::Record;
    return dump<Record>(argc, argv, "Gradido blockchains");
}    
