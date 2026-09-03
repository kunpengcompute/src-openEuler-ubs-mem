/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "service_api_test.h"
#include "app_ipc_stub.h"
#include "mem_lease_service.h"
#include "mxm_ipc_server_interface.h"
#include "mxm_rpc_server_interface.h"
#include "region_manager.h"

namespace UT {
using namespace ock::lease::service;
using namespace ock::share::service;
using namespace ock::com::rpc;
using namespace ock::com::ipc;
void MxmComStopRpcServerStub()
{
    printf("MxmComStopRpcServerStub\n");
    return;
}

void MockTest()
{
    MOCKER(MxmComStartIpcClient).stubs().will(returnValue(0));
    MOCKER(MxmComStopIpcClient).stubs().will(invoke(MxmComStopIpcClientStub));
    MOCKER(MxmComStartRpcServer).stubs().will(returnValue(0));
    MOCKER(MxmComStopRpcServer).stubs().will(invoke(MxmComStopRpcServerStub));
    MOCKER(MxmComStartIpcServer).stubs().will(returnValue(0));
    MOCKER(MxmComStopIpcServer).stubs().will(returnValue(0));
}

TEST_F(ServiceApiTestSuite, CreateTest)
{
    auto service = Create();
    EXPECT_NE(service, nullptr);
    Destroy(service);
}

TEST_F(ServiceApiTestSuite, ServiceProcessArgsTest)
{
    auto service = Create();
    int argc = 1;
    char *argv[] = {"test"};
    auto ret = ServiceProcessArgs(service, argc, argv);
    EXPECT_EQ(ret, 0);
    Destroy(service);
}

TEST_F(ServiceApiTestSuite, ServiceInitializeTest)
{
    MockTest();
    auto service = Create();
    auto ret = ServiceInitialize(service);
    EXPECT_EQ(ret, 0);
    ret = ServiceUninitialize(service);
    EXPECT_EQ(ret, 0);
    Destroy(service);
}

TEST_F(ServiceApiTestSuite, ServiceApiTest)
{
    MockTest();
    MOCKER_CPP(&RegionManager::Recovery, int32_t(*)()).stubs().will(returnValue(0));
    auto service = Create();
    auto ret = ServiceInitialize(service);
    EXPECT_EQ(ret, 0);
    ret = ServiceStart(service);
    EXPECT_EQ(ret, 0);
    ret = ServiceHealthy(service);
    EXPECT_EQ(ret, 0);
    ret = ServiceShutdown(service);
    EXPECT_EQ(ret, 0);
    ret = ServiceUninitialize(service);
    EXPECT_EQ(ret, 0);
    Destroy(service);
}
} // namespace UT
