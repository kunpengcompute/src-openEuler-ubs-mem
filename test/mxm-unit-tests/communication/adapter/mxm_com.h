/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef MXM_COM_H
#define MXM_COM_H

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
const size_t MAX_SIZE = 255;
class MxmComTestSuite : public testing::Test {
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

#endif // MXM_COM_H
