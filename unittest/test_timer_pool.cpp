#include "test_common.h"

#include "timer_pool.h"

using namespace timer_pool;

class TestTimerListenerImpl : public TimerListener {
public:
    virtual void timerEvent(Timer* timer) {
        std::cerr << "TIME!!!" << std::endl;
    }

};

TEST(TimerPool, smoke) {
    Timer* t = TimerPool::getInstance()->createTimer();
    TestTimerListenerImpl listener;
    t->setListener(&listener);
    std::cerr << "start!!!" << std::endl;

    t->start(2);
    sleep(3);
    delete t;
}

