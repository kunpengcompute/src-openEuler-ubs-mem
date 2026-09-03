/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef UBSM_TEST_DLOCK_EXECUTOR_TEST_H
#define UBSM_TEST_DLOCK_EXECUTOR_TEST_H

#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>

namespace UT {
class DlockExecutorTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        GlobalMockObject::reset();
    };

    void TearDown() override
    {
        GlobalMockObject::reset();
    };
};

} // namespace UT
#endif // UBSM_TEST_DLOCK_EXECUTOR_TEST_H
