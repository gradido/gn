#ifndef GRADIDO_MESSAGES_H
#define GRADIDO_MESSAGES_H

// TODO: only for linux, to prevent warnings
#include <sys/sysmacros.h>

#include "gradido/gradido.pb.h"
#include "gradido/gradido.grpc.pb.h"
#include "mirror/ConsensusService.pb.h"
#include "mirror/ConsensusService.grpc.pb.h"

namespace gradido {
    
using namespace com::hedera::mirror::api::proto;
namespace grpr = ::com::gradido::proto;
    
}

#endif
