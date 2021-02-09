#include "test_common.h"
#include <stdlib.h>

bool test_init_logging() {
    init_logging(true, true, true);
    const char* val = "1";
    if (setenv(EXTENSIVE_LOGGING_ENABLED, val, 1) != 0)
        LOG("couldn't set " << EXTENSIVE_LOGGING_ENABLED);
}

bool loggin_init_done = test_init_logging();
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

bool should_use_valgrind() {
    char* cc = std::getenv(GRADIDO_TEST_USE_VALGRIND);
    std::string uv = std::string(cc ? cc : "");
    return uv.length() > 0;
}

bool use_valgrind = should_use_valgrind();

std::string init_build_dir() {
    std::string res = Poco::Path::current();
    res = build_dir.substr(0, build_dir.length() - 1);
    return res;
}

std::string build_dir = init_build_dir();


