/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef UBSM_TEST_SHM_MANAGER_TEST_H
#define UBSM_TEST_SHM_MANAGER_TEST_H

#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>
#include "shm_manager.h"

namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class SHMManagerTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        ock::share::service::SHMManager::Recovery();
        GlobalMockObject::reset();
    };

    void TearDown() override
    {
        ock::share::service::SHMManager::Recovery();
        GlobalMockObject::reset();
    };
};

} // namespace UT
#endif // UBSM_TEST_SHM_MANAGER_TEST_H
