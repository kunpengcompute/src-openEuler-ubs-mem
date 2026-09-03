/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Author: bao
 */

#include <gtest/gtest.h>
#include <unistd.h>

#include "util/functions.h"

namespace UT {
class UtilFunctionTest : public testing::Test {
public:
    UtilFunctionTest();
    virtual void SetUp(void);
    virtual void TearDown(void);

protected:
};

UtilFunctionTest::UtilFunctionTest() {}

void UtilFunctionTest::SetUp() {}

void UtilFunctionTest::TearDown() {}

TEST_F(UtilFunctionTest, TestOckStol)
{
    long val = 0;
    bool ret = ock::common::OckStol("0", val);
    EXPECT_EQ(true, ret);
}

TEST_F(UtilFunctionTest, TestOckStof)
{
    float val = 0.0;
    bool ret = ock::common::OckStof("0.0", val);
    EXPECT_EQ(true, ret);
}

TEST_F(UtilFunctionTest, TestIsBool)
{
    std::string falseStr = "false";
    bool ret = true;
    ret = ock::common::IsBool(falseStr, ret);
    EXPECT_EQ(true, ret);
    std::string trueStr = "true";
    ret = ock::common::IsBool(trueStr, ret);
    EXPECT_EQ(true, ret);
}
} // namespace UT