#include "test_common.h"

void task_settle() {
    std::this_thread::sleep_for(std::chrono::milliseconds(TASK_SETTLE_TIME_MS));
}

void task_settle(int more_seconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(TASK_SETTLE_TIME_MS + more_seconds * 1000));
}
