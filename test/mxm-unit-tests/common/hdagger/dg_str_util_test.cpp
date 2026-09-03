/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <cmath>
#include <mockcpp/mockcpp.hpp>

#include "common/hdagger/strings/dg_str_util.h"

using testing::Test;

namespace UT {
using namespace ock::dagger;

class DgStrUtilTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(DgStrUtilTestSuite, TestStratWith)
{
    ASSERT_TRUE(StrUtil::StartWith("TestStartWith", "Test"));

    ASSERT_TRUE(StrUtil::StartWith("TestStartWith", "TestStartWith"));

    ASSERT_FALSE(StrUtil::StartWith("TestStartWith", "With"));

    ASSERT_FALSE(StrUtil::StartWith("Test", "TestStartWith"));
}

TEST_F(DgStrUtilTestSuite, TestEndWith)
{
    ASSERT_TRUE(StrUtil::EndWith("TestEndWith", "With"));

    ASSERT_TRUE(StrUtil::EndWith("TestEndWith", "TestEndWith"));

    ASSERT_FALSE(StrUtil::EndWith("TestEndWith", "Test"));

    ASSERT_FALSE(StrUtil::EndWith("Test", "TestEndWith"));
}

TEST_F(DgStrUtilTestSuite, TestStrToLong)
{
    long value = 0;

    ASSERT_TRUE(StrUtil::StrToLong("10", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToLong("-10", value));
    ASSERT_EQ(value, -10);

    ASSERT_TRUE(StrUtil::StrToLong("010", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToLong("-010", value));
    ASSERT_EQ(value, -10);

    ASSERT_TRUE(StrUtil::StrToLong("9223372036854775807", value));
    ASSERT_EQ(value, 9223372036854775807);

    ASSERT_TRUE(StrUtil::StrToLong("-9223372036854775807", value));
    ASSERT_EQ(value, -9223372036854775807);

    ASSERT_FALSE(StrUtil::StrToLong("TestStrToLong", value));

    ASSERT_FALSE(StrUtil::StrToLong("-0.10", value));

    ASSERT_FALSE(StrUtil::StrToLong("100000000000000000000000000000", value));

    ASSERT_FALSE(StrUtil::StrToLong("10abc", value));
}

TEST_F(DgStrUtilTestSuite, TestStrToUint)
{
    uint32_t value = 0;

    ASSERT_TRUE(StrUtil::StrToUint("10", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToUint("010", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToUint("4294967295", value));
    ASSERT_EQ(value, 4294967295);

    ASSERT_TRUE(StrUtil::StrToUint("-1", value));
    ASSERT_EQ(value, 4294967295);

    ASSERT_FALSE(StrUtil::StrToUint("TestStrToUint", value));

    ASSERT_FALSE(StrUtil::StrToUint("0.10", value));

    ASSERT_FALSE(StrUtil::StrToUint("100000000000000000000000000000", value));

    ASSERT_FALSE(StrUtil::StrToUint("10abc", value));
}

TEST_F(DgStrUtilTestSuite, TestStrToULong)
{
    uint64_t value = 0;

    ASSERT_TRUE(StrUtil::StrToULong("10", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToULong("010", value));
    ASSERT_EQ(value, 10);

    ASSERT_TRUE(StrUtil::StrToULong("18446744073709551615", value));
    ASSERT_EQ(value, 18446744073709551615ULL);

    ASSERT_TRUE(StrUtil::StrToULong("-1", value));
    ASSERT_EQ(value, 18446744073709551615ULL);

    ASSERT_FALSE(StrUtil::StrToULong("TestStrToUint", value));

    ASSERT_FALSE(StrUtil::StrToULong("0.10", value));

    ASSERT_FALSE(StrUtil::StrToULong("100000000000000000000000000000", value));

    ASSERT_FALSE(StrUtil::StrToULong("10abc", value));
}

TEST_F(DgStrUtilTestSuite, TestStrToFloat)
{
    float value = 0;

    ASSERT_TRUE(StrUtil::StrToFloat("10", value));
    ASSERT_TRUE(fabs(value - 10.0f) < 0.0000001);

    ASSERT_TRUE(StrUtil::StrToFloat("-10", value));
    ASSERT_TRUE(fabs(value - (-10.0f)) < 0.0000001);

    ASSERT_TRUE(StrUtil::StrToFloat("-0.10", value));
    ASSERT_TRUE(fabs(value - (-0.10f)) < 0.0000001);

    ASSERT_TRUE(StrUtil::StrToFloat("-00.10", value));
    ASSERT_TRUE(fabs(value - (-0.10f)) < 0.0000001);

    ASSERT_FALSE(StrUtil::StrToFloat("TestStrToUint", value));

    ASSERT_FALSE(StrUtil::StrToFloat("0.00", value));

    ASSERT_FALSE(StrUtil::StrToFloat("0.0000001", value));
}

TEST_F(DgStrUtilTestSuite, TestStrSplit)
{
    std::vector<std::string> out;

    StrUtil::Split("Test|Str|Split", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)3);

    out.clear();
    StrUtil::Split("TestStrSplit|", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)1);

    out.clear();
    StrUtil::Split("TestStrSplit", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)1);
}

TEST_F(DgStrUtilTestSuite, TestStrSplitSet)
{
    std::set<std::string> out;

    StrUtil::Split("Test|Str|Split|Set", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)4);

    out.clear();
    StrUtil::Split("Test|Test|Str|Split|Set", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)4);

    out.clear();
    StrUtil::Split("TestStrSplitSet|", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)1);

    out.clear();
    StrUtil::Split("TestStrSplitSet", "|", out);
    ASSERT_EQ(out.size(), (std::size_t)1);
}

TEST_F(DgStrUtilTestSuite, TestStrTrim)
{
    std::string str = "";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "");

    str = " ";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "");

    str = "Test StrTrim";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "Test StrTrim");

    str = " Test StrTrim";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "Test StrTrim");

    str = "Test StrTrim ";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "Test StrTrim");

    str = "  Test StrTrim  ";
    StrUtil::StrTrim(str);
    ASSERT_EQ(str, "Test StrTrim");
}

TEST_F(DgStrUtilTestSuite, TestStrReplace)
{
    std::string str = "TestStrReplace";
    StrUtil::Replace(str, "\\w+Replace", "e");
    ASSERT_EQ(str, "");

    str = "";
    StrUtil::Replace(str, "\\w+Replace", "e");
    ASSERT_EQ(str, "");

    str = "%TestStrReplace";
    StrUtil::Replace(str, "\\w+Replace", "e");
    ASSERT_EQ(str, "%");

    str = "TestStrReplace%TestStrReplace";
    StrUtil::Replace(str, "\\w+Replace", "e");
    ASSERT_EQ(str, "e%");

    str = "%Replace";
    StrUtil::Replace(str, "\\w+Replace", "e");
    ASSERT_EQ(str, "%Replac");
}
} // namespace UT
