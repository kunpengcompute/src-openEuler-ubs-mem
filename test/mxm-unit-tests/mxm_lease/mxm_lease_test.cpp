/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

namespace UT {
class MemLeaseTest : public testing::Test {
public:
    static void TearDownTestCase()
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void SetUp() override {}

    void TearDown() override {}
};
} // namespace UT
