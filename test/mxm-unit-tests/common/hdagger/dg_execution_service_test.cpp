/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "common/hdagger/thread_pool/dg_execution_service.h"

using testing::Test;

namespace UT {
using namespace ock::dagger;

class DgExecutionServiceTestSuite : public Test {
public:
    DgExecutionServiceTestSuite();
    void SetUp() override;
    void TearDown() override;

protected:
    static ExecutorServicePtr executorService;
};

class Task : public Runnable {
public:
    void Run() override
    {
        std::cout << "task is executed" << std::endl;
    }
};

ExecutorServicePtr DgExecutionServiceTestSuite::executorService;

DgExecutionServiceTestSuite::DgExecutionServiceTestSuite() = default;

void DgExecutionServiceTestSuite::SetUp()
{
    executorService = ExecutorService::Create(1, 128);
    ASSERT_TRUE(executorService.Get() != nullptr);

    executorService->SetThreadName("tt");
    ASSERT_TRUE(executorService->Start());
}

void DgExecutionServiceTestSuite::TearDown()
{
    executorService->Stop();
}

TEST_F(DgExecutionServiceTestSuite, TestExecutionService)
{
    auto t = new (std::nothrow) Task();
    ASSERT_TRUE(executorService->Execute(t));

    sleep(1);
}

TEST_F(DgExecutionServiceTestSuite, TestLambdaExpression)
{
    struct timespec ts {
    };
    sem_t waitSem{};

    auto ret = clock_gettime(CLOCK_REALTIME, &ts);
    ASSERT_EQ(0, ret) << "get system time failed: " << errno << ": " << strerror(errno);

    ret = sem_init(&waitSem, 0, 0);
    ASSERT_EQ(0, ret) << "initialize sem failed: " << errno << ": " << strerror(errno);

    auto task = [&waitSem]() {
        sem_post(&waitSem);
    };
    auto success = executorService->Execute(task);
    ASSERT_TRUE(success);

    ts.tv_sec += 5;
    ret = sem_timedwait(&waitSem, &ts);
    ASSERT_EQ(0, ret) << "wait sem failed: " << errno << ": " << strerror(errno);

    sem_destroy(&waitSem);
}

TEST_F(DgExecutionServiceTestSuite, Execute_NullptrTask)
{
    Runnable *t = nullptr;
    auto success = executorService->Execute(t);
    EXPECT_FALSE(success);
}

TEST_F(DgExecutionServiceTestSuite, Create_WithInvalidThreadNum)
{
    auto s1 = ExecutorService::Create(-1);
    auto s2 = ExecutorService::Create(0);
    auto s3 = ExecutorService::Create(300);
    EXPECT_EQ(nullptr, s1.Get());
    EXPECT_EQ(nullptr, s2.Get());
    EXPECT_EQ(nullptr, s3.Get());
}

TEST_F(DgExecutionServiceTestSuite, Start_Twice)
{
    ASSERT_TRUE(executorService->Start());
    ASSERT_TRUE(executorService->Start());
}

TEST_F(DgExecutionServiceTestSuite, Stop_Twice)
{
    executorService->Stop();
    executorService->Stop();
    SUCCEED();
}

TEST_F(DgExecutionServiceTestSuite, Runnable_RunWithNullTask)
{
    Runnable r;
    r.Run();
    SUCCEED();
}

TEST_F(DgExecutionServiceTestSuite, DoRunnable_RuntimeError)
{
    bool flag = true;
    auto r = MakeRef<Runnable>([]() { throw std::runtime_error("test"); });
    executorService->Execute(r);
    sleep(1);
}

TEST_F(DgExecutionServiceTestSuite, DoRunnable_UnknownException)
{
    auto r = MakeRef<Runnable>([]() { throw 42; });
    executorService->Execute(r);
    sleep(1);
}

TEST(DgExecutionServiceStandalone, Create_WithZeroQueueCapacity)
{
    auto svc = ExecutorService::Create(1, 0);
    ASSERT_TRUE(svc.Get() != nullptr);
}

TEST(DgExecutionServiceStandalone, DestructorWithoutStop)
{
    auto svc = ExecutorService::Create(1, 10);
}

TEST(DgExecutionServiceStandalone, Stop_NewRunnableNull)
{
    auto svc = ExecutorService::Create(1, 10);
    svc->Start();
    svc->Stop();
}

TEST(DgExecutionServiceStandalone, RunInThread_WithEmptyNameAndCpuBind)
{
    auto svc = ExecutorService::Create(1, 10);
    svc->SetCpuSetStartIndex(0);
    svc->SetThreadName("");
    ASSERT_TRUE(svc->Start());
    svc->Stop();
}

} // namespace UT
