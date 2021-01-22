#include "utils.h"
#include "dumper.h"
#include "main_def.h"

using namespace gradido;

int main(int argc, char** argv) {
    GRADIDO_CMD_UTIL;
    return dump<SbRecord>(params, "subcluster blockchain");
}    
