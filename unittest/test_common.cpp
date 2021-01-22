#include "test_common.h"

bool loggin_init_done = init_logging(true, true);
bool not_supported_init_done = set_gradido_strict_not_supported(false);

void task_settle() {
    std::this_thread::sleep_for(std::chrono::milliseconds(TASK_SETTLE_TIME_MS));
}

void task_settle(int more_seconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(TASK_SETTLE_TIME_MS + more_seconds * 1000));
}

void erase_tmp_folder(std::string folder) {
    ASSERT_EQ(std::string(folder).rfind("/tmp", 0) == 0, true);
    ASSERT_EQ(std::string(folder).size() > 4, true);
    Poco::File bf(folder);
    try {
        bf.remove(true);
    } catch (...)  {
    }
}
