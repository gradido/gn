#include "gradido_facade.h"
#include <iostream>

using namespace gradido;

int main(int argc, char** argv) {
    GradidoFacade gf;
    gf.init();
    gf.join();
}    
