#include "test_common.h"
#include "worker_pool.h"

TEST(WorkerPool, single_task_gets_executed)
{
    WorkerPool wp;
    wp.init(1);
    NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
    EXPECT_CALL(*t, run()).Times(1);
    wp.push(t);
    task_settle();
}

TEST(WorkerPool, exception_doesnt_stop_worker)
{
    WorkerPool wp;
    wp.init(1);
    {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1)
            .WillRepeatedly(ThrowException());
        wp.push(t);
        task_settle();
    }
    {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1);
        wp.push(t);
        task_settle();
    }
}

TEST(WorkerPool, tasks_are_done_in_parallel)
{
    int worker_count = 10;
    WorkerPool wp;
    wp.init(worker_count);
    for (int i = 0; i < worker_count; i++) {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1)
            .WillRepeatedly(SleepSecond());
        wp.push(t);
    }
    task_settle(1);
    size_t tasks_in_progress = wp.get_task_queue_size() + 
        wp.get_busy_worker_count();
    ASSERT_EQ(tasks_in_progress, 0) << "some tasks are not yet done";
}


TEST(WorkerPool, tasks_are_done_in_serial)
{
    WorkerPool wp;
    wp.init(1);
    {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1)
            .WillRepeatedly(SleepSecond());
        wp.push(t);
    }
    {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1)
            .WillRepeatedly(SleepSecond());
        wp.push(t);
    }
    task_settle();
    ASSERT_EQ(wp.get_task_queue_size(), 1);
    ASSERT_EQ(wp.get_busy_worker_count(), 1);
    sleep(1);
    ASSERT_EQ(wp.get_task_queue_size(), 0);
    ASSERT_EQ(wp.get_busy_worker_count(), 1);
    sleep(1);
    ASSERT_EQ(wp.get_task_queue_size(), 0);
    ASSERT_EQ(wp.get_busy_worker_count(), 0);
}

class ShutdownTask : public ITask {
private: 
    WorkerPool& wp;
public:
    ShutdownTask(WorkerPool& wp) : wp(wp) {}
    virtual void run() {
        wp.init_shutdown();
    }
};

TEST(WorkerPool, shutdown_done_from_task)
{
    int worker_count = 10;
    WorkerPool wp;
    wp.init(worker_count);
    for (int i = 0; i < worker_count - 1; i++) {
        NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
        EXPECT_CALL(*t, run())
            .Times(1)
            .WillRepeatedly(SleepSecond());
        wp.push(t);
    }
    task_settle();
    wp.push(new ShutdownTask(wp));
    wp.join();
    size_t tasks_in_progress = wp.get_task_queue_size() + 
        wp.get_busy_worker_count();
    ASSERT_EQ(tasks_in_progress, 0) << "some tasks are not yet done";
    ASSERT_EQ(wp.get_worker_count(), 0) << "some workers are still up";
}

TEST(WorkerPool, push_after_shutdown)
{
    WorkerPool wp;
    wp.init(1);
    wp.init_shutdown();
    NiceMock<MockSampleTask>* t = new NiceMock<MockSampleTask>();
    EXPECT_CALL(*t, run()).Times(0);
    wp.push(t);
    task_settle();
}