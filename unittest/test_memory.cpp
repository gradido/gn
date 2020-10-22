#include "test_common.h"
#include "memory_managers.h"

using namespace gradido;

TEST(PermanentBlockAllocator, create_and_destroy_non_mock) {
    SimpleRec* res;
    PermanentBlockAllocator<SimpleRec> pba(10);
    pba.allocate(&res);
}


TEST(PermanentBlockAllocator, create_and_destroy) {
    MockSimpleRec* res;
    PermanentBlockAllocator<MockSimpleRec> pba(10);
    pba.allocate(&res);
    EXPECT_CALL(*res, empty()).
        Times(0);
}

TEST(PermanentBlockAllocator, create_and_destroy_two) {
    MockSimpleRec* res;
    PermanentBlockAllocator<MockSimpleRec> pba(10);
    pba.allocate(&res);
    EXPECT_CALL(*res, empty()).
        Times(0);

    pba.allocate(&res);
    EXPECT_CALL(*res, empty()).
        Times(0);
}


TEST(BigObjectMemoryPool, create_and_destroy) {
    BigObjectMemoryPool<MockSimpleRec, 5> pool(10);
    MockSimpleRec* res = pool.get();

    for (int i = 0; i < 5; i++)
        EXPECT_CALL(res[i], empty()).
            Times(0);

    pool.release(res);
}
