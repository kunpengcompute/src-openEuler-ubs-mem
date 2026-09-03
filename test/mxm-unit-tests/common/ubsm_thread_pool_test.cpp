#include "ubsm_thread_pool.h"
#include <gtest/gtest.h>
#include <atomic>

using namespace ock::ubsm;

TEST(UbsmThreadPoolTest, StartStop)
{
    UBSMThreadPool pool(2, "test_pool");
    EXPECT_EQ(pool.Start(), 0);
    pool.Stop();
}

TEST(UbsmThreadPoolTest, PushAndExecute)
{
    UBSMThreadPool pool(2, "test_pool2");
    EXPECT_EQ(pool.Start(), 0);
    std::atomic<int> counter{0};
    pool.Push([&counter]() { counter++; });
    pool.Push([&counter]() { counter++; });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(counter.load(), 2);
    pool.Stop();
}

TEST(UbsmThreadPoolTest, DestructorStops)
{
    {
        UBSMThreadPool pool(1, "auto_stop");
        pool.Start();
        pool.Push([]() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
    }
}
