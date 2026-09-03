/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef UBSM_TEST_SHM_IPC_HANDLER_TEST_H
#define UBSM_TEST_SHM_IPC_HANDLER_TEST_H

#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>
namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class ShmIpcHandlerTestSuite : public testing::Test {
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
#endif // UBSM_TEST_SHM_IPC_HANDLER_TEST_H
