/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "shm_irpc_server_test.h"
#include "mxm_shm/ipc_server.h"
#include "mxm_shm/rpc_server.h"
#include "service.h"
#include "shm_service_api.h"

#include <ock_service.h>

using namespace UT;

TEST_F(ShmIRpcServerTestSuite, TestIPCRackMemConBaseInitialize)
{
    auto ret = ock::share::service::IpcServer::RackMemConBaseInitialize();
    ASSERT_EQ(ret, HOK);
    ret = ock::share::service::IpcServer::GetInstance().Stop();
    ASSERT_EQ(ret, HOK);
}

TEST_F(ShmIRpcServerTestSuite, TestRPCRackMemConBaseInitialize)
{
    auto ret = ock::rpc::service::RpcServer::RackMemConBaseInitialize();
    ASSERT_EQ(ret, HOK);
    ret = ock::rpc::service::RpcServer::GetInstance().Stop();
    ASSERT_EQ(ret, HOK);
}
