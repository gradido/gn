extern bool simple_log;

#include "gradido_interfaces.h"
#include "utils.h"
#include "dumper.h"
#include "main_def.h"

using namespace gradido;


int main(int argc, char** argv) {
    GRADIDO_CMD_UTIL;
    return dump<GradidoRecord>(params, "Gradido blockchains");
}    
