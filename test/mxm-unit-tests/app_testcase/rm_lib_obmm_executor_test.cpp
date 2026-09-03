/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "RmLibObmmExecutor.h"
#include "dlfcn.h"
#include "system_adapter.h"

namespace UT {
using namespace ock::ubsm;
class RmLibObmmExecutorTest : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(RmLibObmmExecutorTest, TestInitializeSuccess)
{
    void *mockAddress = reinterpret_cast<void *>(0x123456789);
    MOCKER(SystemAdapter::DlOpen).stubs().with(any(), any()).will(returnValue(mockAddress));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockAddress));

    ASSERT_EQ(ock::mxmd::RmLibObmmExecutor::GetInstance().Initialize(), HOK);
}

TEST_F(RmLibObmmExecutorTest, TestInitializeOpenLibFailed)
{
    void *mockAddress = nullptr;
    MOCKER(SystemAdapter::DlOpen).stubs().with(any(), any()).will(returnValue(mockAddress));

    ASSERT_EQ(ock::mxmd::RmLibObmmExecutor::GetInstance().Initialize(), ock::common::MXM_ERR_PARAM_INVALID);
}

TEST_F(RmLibObmmExecutorTest, TestInitializeLocdOpenFuncFailed)
{
    void *mockAddress = reinterpret_cast<void *>(0x123456789);
    MOCKER(SystemAdapter::DlOpen).stubs().with(any(), any()).will(returnValue(mockAddress));
    void *mockLoadFunc = nullptr;
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockLoadFunc));
    MOCKER(SystemAdapter::DlClose).stubs().with(any()).will(returnValue(0));

    ASSERT_EQ(ock::mxmd::RmLibObmmExecutor::GetInstance().Initialize(), ock::common::MXM_ERR_PARAM_INVALID);
}

TEST_F(RmLibObmmExecutorTest, TestExit)
{
    void *mockAddress = reinterpret_cast<void *>(0x123456789);
    MOCKER(SystemAdapter::DlOpen).stubs().with(any(), any()).will(returnValue(mockAddress));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockAddress));
    MOCKER(SystemAdapter::DlClose).stubs().with(any()).will(returnValue(0));

    ock::mxmd::RmLibObmmExecutor::GetInstance().Initialize();

    ASSERT_EQ(ock::mxmd::RmLibObmmExecutor::GetInstance().Exit(), HOK);
}
} // namespace UT