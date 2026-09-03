/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef UBSM_TEST_DLOCK_TEST_H
#define UBSM_TEST_DLOCK_TEST_H

#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>
#define private public
#include "zen_discovery.h"
#undef private

namespace UT {
class DlockTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        int pingTimeoutMs = 1000;
        int joinTimeoutMs = 1000;
        int electionTimeoutMs = 1000;
        int minimumMasterNodes = 1;
        ock::zendiscovery::ZenDiscovery::Initialize(pingTimeoutMs, joinTimeoutMs, electionTimeoutMs,
                                                    minimumMasterNodes);
    };

    void TearDown() override
    {
        GlobalMockObject::reset();
    };
};

} // namespace UT
#endif // UBSM_TEST_DLOCK_TEST_H
