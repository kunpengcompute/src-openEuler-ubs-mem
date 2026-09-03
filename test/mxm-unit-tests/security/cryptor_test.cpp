/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <securec.h>
#include <string>
#include "ubs_cryptor_handler.h"

using namespace ock::ubsm;

namespace UT {

class CryptorTest : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

static void TestLogger(int level, const char *str)
{
    std::cout << "[DECRYPT]"
              << "[" << level << "]" << str << std::endl;
}
} // namespace UT