/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef SERVICE_API_TEST_H
#define SERVICE_API_TEST_H
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "service_api.h"
namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class ServiceApiTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        GlobalMockObject::reset();
    };

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    };
};
} // namespace UT
#endif // SERVICE_API_TEST_H
