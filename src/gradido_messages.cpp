#include "gradido_messages.h"
#include "utils.h"

namespace gradido {

std::string debug_str(const grpr::Transaction& t) {
    return t.DebugString();
}
}
