#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <unistd.h>
#include <exception>
#include <chrono>
#include <thread>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "gradido_interfaces.h"
#include "blockchain.h"

/*
- philosophy
  - focus on 
    - good, stable invariances
    - clear abstractions
      - no use of testing volatile stuff
    - exceptional situations
    - things which can be isolated from others
    - stable scenarios, somehow related to real world
    - functionality of a single class or small set of them
    - just interesting "what if..." stuff
  - don't test
    - too simple things
      - such as get/set in a data container
    - volatile things
      - less-abstract code which is going to change
    - those things which obviously will be checked by black box test
  - not good to overlap, but it is not a tragedy if that happens
  - iterative approach, giving more and more details with each next test
    case
  - tests don't answer question about how necessary is one or other piece
    of code; instead, they show that some set of requirements (implicit 
    or explicit) are satisfied
  - case name should reflect which requirement is being tested
- excluded from gradido
  - utils, config
    - useless; code is still volatile
  - facade
    - volatile; not abstract; tested by black box tests

 IMPORTANT: mock object proper deletion is checked automatically if
 there is at least one EXPECT_CALL defined for given object
 */

using namespace gradido;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

class MockSampleTask : public ITask {
public:
    MOCK_METHOD(void, run, (), (override));
};

ACTION(ThrowException) 
{
    throw std::exception();
}

ACTION(SleepSecond) 
{
    sleep(1);
}

#define TASK_SETTLE_TIME_MS 50

void task_settle();
void task_settle(int more_seconds);

struct SimpleRec {
    SimpleRec() : index(0) {}
    virtual ~SimpleRec() {}
    int index;
    virtual void empty() {}
};

class MockSimpleRec : public SimpleRec {
 public:
    MOCK_METHOD(void, empty, (), (override));
};

void erase_tmp_folder(std::string folder);

#endif
