/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#ifndef MXM_IPC_SERVICE_TEST_H
#define MXM_IPC_SERVICE_TEST_H
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "daemon/daemon_test_common.h"
#include "mxm_ipc_server_interface.h"
namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
const size_t MAX_SIZE = 255;
class IpcServerTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        GlobalMockObject::reset();
    };

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
        ock::com::ipc::MxmComStopIpcServer();
        UT::Daemon::DaemonTestCommon::DeleteConf();
        ock::common::Configuration::DestroyInstance();
    };
};
} // namespace UT
#endif // MXM_IPC_SERVICE_TEST_H
