#include "test_common.h"

TEST(GradidoUtils, sanitize_for_log) {
    char buff[1024];
    int bp = 0;
    buff[bp++] = 'n';
    buff[bp++] = 'o';
    buff[bp++] = 'p';
    buff[bp++] = '\n';
    buff[bp++] = 170;
    buff[bp++] = 11;
    buff[bp++] = 17;
    buff[bp++] = 32;
    buff[bp++] = 127;
    buff[bp++] = 'n';
    buff[bp++] = 'o';
    buff[bp++] = 'p';
    buff[bp++] = 'n';
    buff[bp++] = 'o';
    buff[bp++] = 'p';
    buff[bp++] = 0;
    std::string z(buff);
    z = sanitize_for_log(z);
    ASSERT_EQ(z.compare("nop /aa/0b/11 /7fnopnop"), 0);
}
