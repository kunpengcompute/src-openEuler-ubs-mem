/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "ipc_server_test.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "defines.h"
#include "mxm_com_engine.h"
#include "mxm_com_error.h"
#include "service/service.h"
#include "util/common_headers.h"

namespace UT {
using namespace ock::com;
using namespace ock::com::ipc;
using namespace UT::Daemon;

static void IpcServerTestMock()
{
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = DEBUG\n"
                                 "ubsm.server.log.rotation.file.size = 200\n"
                                 "ubsm.server.log.rotation.file.count = 100\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log/../log/\n"
                                 "ubsm.server.audit.log.rotation.file.count = 100\n"
                                 "ubsm.server.audit.log.rotation.file.size = 400\n"
                                 "ubsm.lock.enable = on\n"
                                 "ubsm.lock.tls.enable = off\n"
                                 "ubsm.lock.ub_token.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char confPath[2048];
    auto ret = sprintf_s(confPath, sizeof(confPath), "%s/config/ubsmd.conf\0", DaemonTestCommon::CWD().c_str());
    auto conf = Configuration::GetInstance(confPath);
    MOCKER_CPP(&MxmComEngine::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
}

TEST_F(IpcServerTestSuite, TestIpcServerStartSuccess)
{
    IpcServerTestMock();
    auto ret = MxmComStartIpcServer();
    EXPECT_EQ(ret, HOK);
}

TEST_F(IpcServerTestSuite, TestIpcServerStartFail)
{
    MOCKER_CPP(&MxmCommunication::CreateMxmComEngine,
               HRESULT(*)(const MxmComEngineInfo &engine, const MxmComLinkStateNotify &notify,
                          const EngineHandlerWorker &handlerWorker))
        .stubs()
        .will(returnValue(HFAIL));
    auto ret = MxmComStartIpcServer();
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(IpcServerTestSuite, TestIpcServerRegSuccess)
{
    IpcServerTestMock();
    auto ret = MxmComStartIpcServer();
    EXPECT_EQ(ret, HOK);
    MxmComEndpoint endPoint;
    endPoint.address = "test";
    endPoint.moduleId = 1;
    endPoint.serviceId = 1;
    ret = MxmRegIpcService(endPoint, nullptr);
    EXPECT_EQ(ret, HOK);
}

TEST_F(IpcServerTestSuite, TestIpcServerRegFail)
{
    IpcServerTestMock();
    auto ret = MxmComStartIpcServer();
    EXPECT_EQ(ret, HOK);
    MxmComEndpoint endPoint;
    endPoint.address = "test";
    ret = MxmRegIpcService(endPoint, nullptr);
    EXPECT_EQ(ret, MXM_COM_ERROR_MESSAGE_INVALID_OP_CODE);
}
} // namespace UT
