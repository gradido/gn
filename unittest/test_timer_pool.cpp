#include "test_common.h"

#include "timer_pool.h"

class TestTimerListenerImpl : public TimerListener {
public:
    virtual void timerEvent(Timer* timer) {
        LOG("TIME!!!");
    }

};

TEST(TimerPool, smoke) {
    Timer* t = TimerPool::getInstance()->createTimer();
    TestTimerListenerImpl listener;
    t->setListener(&listener);
    LOG("start!!!");

    t->start(2);
    sleep(3);
    delete t;
}

