/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "ubs_common_types.h"

using testing::Test;

namespace UT {
using namespace ock::ubsm;

class UbsCommonTypeTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(UbsCommonTypeTestSuite, TestAppContext)
{
    AppContext context(0, 0, 0);
    context.Fill(1, 1, 1);
    ASSERT_EQ(context.GetString(), "1:1:1");
}
} // namespace UT
