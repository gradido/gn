#include "gradido_interfaces.h"
#include <typeinfo>


namespace gradido {

proto::Timestamp get_current_time() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    proto::Timestamp res;
    res.set_seconds(t.tv_sec);
    res.set_nanos(t.tv_nsec);
    return res;
}

std::string ITask::get_task_info() {
    std::stringstream ss;
    ss << this << ":" << typeid(*this).name();
    return ss.str();
}
    
}
