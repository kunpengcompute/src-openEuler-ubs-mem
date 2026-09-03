/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "dlock_test.h"
#include <dlfcn.h>
#include "dlock_utils/ubsm_lock.h"
#include "system_adapter.h"

using namespace ock::dlock_utils;
using namespace ock::ubsm;
namespace UT {
constexpr auto SERVER_ON = "on";
constexpr auto IP_MOCK = "1.2.3.4";
constexpr auto DEV_MOCK = "dev";
constexpr auto EID_MOCK = "0000:0000:0000:1000:0010:0000:df00:0404";
constexpr auto ELECTION_TIMEOUT = 1000;
constexpr auto DISCOVERY_MIN_NODES = 0;
int MockDLockSuccess()
{
    return dlock::DLOCK_SUCCESS;
}
int MockDLockFail()
{
    return -1;
}
void MockDlsym(const char *name)
{
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    void *mockFailFunc = reinterpret_cast<void *>(MockDLockFail);
    MOCKER(SystemAdapter::DlSym)
        .stubs()
        .with(any(), checkWith([name](const char *str) mutable -> bool { return strcmp(str, name) == 0; }))
        .will(returnValue(mockFailFunc));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
}
void TestDefaultConfig()
{
    auto &ctx = DLockContext::Instance();
    ctx.GetConfig().isDlockServer = true;
    ctx.GetConfig().serverIp = IP_MOCK;
    ctx.GetConfig().clientIp = IP_MOCK;
    ctx.GetConfig().dlockDevName = DEV_MOCK;
    ctx.GetConfig().dlockDevEid = EID_MOCK;
    ctx.GetConfig().dlockClientNum = 1;
}

TEST_F(DlockTestSuite, TestDLockInitSuccess)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_EQ(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockInitSuccess_TLS)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    MOCKER(SystemAdapter::DlClose).stubs().will(returnValue(0));
    TestDefaultConfig();
    DLockContext::Instance().GetConfig().enableTls = true;
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_EQ(ret, dlock::DLOCK_SUCCESS);
    DLockContext::Instance().GetConfig().enableTls = false;
    UbsmLock::Instance().DeInit();
}

TEST_F(DlockTestSuite, TestDLockInitFail1)
{
    // given
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(nullptr)));
    MOCKER(SystemAdapter::DlClose).stubs().will(returnValue(0));
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_NE(ret, UBSM_OK);
}

TEST_F(DlockTestSuite, TestDLockInitFail2)
{
    // given
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("dserver_lib_init");
    TestDefaultConfig();
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_NE(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockInitFail3)
{
    // given
    int mockHandle = 0;
    DLockContext::Instance().GetConfig().isDlockClient = true;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("dclient_lib_init");
    TestDefaultConfig();
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_NE(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockInitFail4)
{
    // given
    int mockHandle = 0;
    DLockContext::Instance().GetConfig().isDlockClient = true;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("client_init");
    TestDefaultConfig();
    // when
    auto ret = UbsmLock::Instance().Init();
    // then
    ASSERT_NE(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockDeInitSuccess)
{
    // given
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();
    MOCKER(SystemAdapter::DlClose).stubs().will(returnValue(0));
    cfg.isDlockClient = true;
    cfg.dlockClientNum = 0;
    ctx.SetServerDeinitFlag(true);
    DLockExecutor::GetInstance().DLockServerStopFunc = [](int server_id) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    DLockExecutor::GetInstance().DLockSeverLibDeInitFunc = []() -> void {
    };
    DLockExecutor::GetInstance().DLockClientLibDeInitFunc = []() -> void {
    };
    // when
    auto ret = UbsmLock::Instance().DeInit();
    // then
    ASSERT_EQ(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockDeInitFail)
{
    // given
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();
    MOCKER(SystemAdapter::DlClose).stubs().will(returnValue(0));
    cfg.dlockClientNum = 0;
    ctx.SetServerDeinitFlag(true);
    cfg.isDlockClient = true;
    // 日志分支
    DLockExecutor::GetInstance().DLockServerStopFunc = [](int server_id) -> int {
        return -1;
    };
    DLockExecutor::GetInstance().DLockSeverLibDeInitFunc = []() -> void {
    };
    DLockExecutor::GetInstance().DLockClientLibDeInitFunc = []() -> void {
    };
    // when
    auto ret = UbsmLock::Instance().DeInit();
    // then
    ASSERT_EQ(ret, dlock::DLOCK_SUCCESS);
}

TEST_F(DlockTestSuite, TestDLockLockSuccess)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    auto ret = UbsmLock::Instance().Init();
    ASSERT_EQ(UBSM_OK, ret);
    std::string name = "lock";
    DLockExecutor::GetInstance().DLockClientGetLockFunc = [](int clientId, const struct dlock::lock_desc *desc,
                                                             int *lockId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    DLockExecutor::GetInstance().DLockTryLockFunc = [](int clientId, const struct dlock::lock_request *req,
                                                       void *result) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    ock::rpc::NetRpcConfig::GetInstance().SetLocalNode(std::make_pair("1.2.3.4", 1234));
    ock::zendiscovery::ZenDiscovery::GetInstance()->nodes_.emplace(
        "1.2.3.4:1234", ock::rpc::ClusterNode{.id = "1.2.3.4:1234",
                                              .ip = "1.2.3.4",
                                              .port = 1234,
                                              .isActive = true,
                                              .type = ock::rpc::NodeType::VOTING_ONLY_NODE});

    ock::dlock_utils::LockUdsInfo info;
    info.pid = 0;
    info.uid = 0;
    info.gid = 0;
    // when
    ret = UbsmLock::Instance().Lock(name, true, info);
    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockTestSuite, TestDLockLockFail)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    auto ret = UbsmLock::Instance().Init();
    ASSERT_EQ(UBSM_OK, ret);
    std::string name = "lock1";
    DLockExecutor::GetInstance().DLockClientGetLockFunc = [](int clientId, const struct dlock::lock_desc *desc,
                                                             int *lockId) -> int {
        return dlock::DLOCK_FAIL;
    };
    ock::dlock_utils::LockUdsInfo info;
    info.pid = 0;
    info.uid = 0;
    info.gid = 0;
    // when
    ret = UbsmLock::Instance().Lock(name, true, info);
    // then
    ASSERT_EQ(ret, MXM_ERR_LOCK_NOT_FOUND);
}

TEST_F(DlockTestSuite, TestDLockUnLockSuccess)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    auto ret = UbsmLock::Instance().Init();
    ASSERT_EQ(UBSM_OK, ret);
    std::string name = "lock2";
    DLockExecutor::GetInstance().DLockClientGetLockFunc = [](int clientId, const struct dlock::lock_desc *desc,
                                                             int *lockId) -> int {
        return dlock::DLOCK_SUCCESS;
    };

    DLockExecutor::GetInstance().DLockTryLockFunc = [](int clientId, const struct dlock::lock_request *req,
                                                       void *result) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    ock::dlock_utils::LockUdsInfo info;
    info.pid = 0;
    info.uid = 0;
    info.gid = 0;
    ret = UbsmLock::Instance().Lock(name, true, info);
    ASSERT_EQ(ret, UBSM_OK);
    DLockExecutor::GetInstance().DLockUnlockFunc = [](int clientId, int lockId, void *result) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    // when
    ret = UbsmLock::Instance().Unlock(name, info);
    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockTestSuite, TestDLockUnLockFail)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    auto ret = UbsmLock::Instance().Init();
    ASSERT_EQ(UBSM_OK, ret);
    std::string name = "lock3";
    DLockExecutor::GetInstance().DLockClientGetLockFunc = [](int clientId, const struct dlock::lock_desc *desc,
                                                             int *lockId) -> int {
        return dlock::DLOCK_SUCCESS;
    };

    DLockExecutor::GetInstance().DLockTryLockFunc = [](int clientId, const struct dlock::lock_request *req,
                                                       void *result) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    ock::dlock_utils::LockUdsInfo info;
    info.pid = 0;
    info.uid = 0;
    info.gid = 0;
    ret = UbsmLock::Instance().Lock(name, true, info);
    ASSERT_EQ(ret, UBSM_OK);
    DLockExecutor::GetInstance().DLockUnlockFunc = [](int clientId, int lockId, void *result) -> int {
        return dlock::DLOCK_BAD_RESPONSE;
    };
    // when
    ret = UbsmLock::Instance().Unlock(name, info);
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_INNER);
}

void DefaultMock()
{
    auto &exec = DLockExecutor::GetInstance();
    exec.DLockServerStopFunc = [](int serverId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockServerLibInitFunc = [](unsigned int maxServerNum) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockServerStartFunc = [](const dlock::server_cfg &cfg, int &serverId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockClientInitFunc = [](int *clientId, const char *ipStr) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockServerStopFunc = [](int clientId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockUpdateAllLocksFunc = [](int clientId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockClientReinitDoneFunc = [](int clientId) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockClientHeartbeatFunc = [](int clientId, unsigned int timeout) -> int {
        return dlock::DLOCK_SUCCESS;
    };
}

TEST_F(DlockTestSuite, TestReinitSuccessAllSuccess)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();

    int32_t ret = ctx.InitDlockClient();
    EXPECT_EQ(UBSM_OK, ret);

    // 所有返回值均成功
    ret = UbsmLock::Instance().Reinit();
    EXPECT_EQ(UBSM_OK, ret);
}

TEST_F(DlockTestSuite, TestReinitSuccessServerStopFail)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    EXPECT_EQ(UBSM_OK, ret);

    // DLockServerStopFunc 返回非成功值
    exec.DLockServerStopFunc = [](int clientId) -> int {
        return -1;
    };
    ret = UbsmLock::Instance().Reinit();
    EXPECT_EQ(UBSM_OK, ret);
}

TEST_F(DlockTestSuite, TestReinitWithServerReinitFailedEmptyServerIp)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    DefaultMock();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 因 serverIp 为空失败
    ctx.GetConfig().serverIp = "";
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_PARAM_INVALID, ret);
}

TEST_F(DlockTestSuite, TestReinitWithServerReinitFailedServerLibInitFail)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    ctx.GetConfig().serverIp = "1.2.3.4";
    // 因 DLockServerLibInitFunc 返回非成功值失败
    ctx.SetServerDeinitFlag(false);
    exec.DLockServerLibInitFunc = [](unsigned int maxServerNum) -> int {
        return dlock::DLOCK_FAIL;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret);
}

TEST_F(DlockTestSuite, TestReinitWithServerReinitFailedServerStartNoResource)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    ctx.GetConfig().serverIp = "1.2.3.4";
    exec.DLockServerLibInitFunc = [](unsigned int maxServerNum) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    // 因 ServerStart 返回 DLOCK_SERVER_NO_RESOURCE 失败
    exec.DLockServerStartFunc = [](const dlock::server_cfg &cfg, int &serverId) -> int {
        return dlock::DLOCK_SERVER_NO_RESOURCE;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "ServerStart should return DLOCK_SERVER_NO_RESOURCE";
}

TEST_F(DlockTestSuite, TestReinitWithClientReinitFailedWithUnknownErr)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 返回 -1 导致失败
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return -1;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "Client returns -1";
}

TEST_F(DlockTestSuite, TestReinitWithClientReinitFailedTooManyNotReady)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 因长时间 DLOCK_NOT_READY 失败
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return dlock::DLOCK_NOT_READY;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "Client returns too many times DLOCK_NOT_READY";
}

TEST_F(DlockTestSuite, TestReinitWithClientReinitFailedBadResponse)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 因 ClientReInitStagesUpdateLocks 返回 DLOCK_BAD_RESPONSE 失败
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockUpdateAllLocksFunc = [](int clientId) -> int {
        return dlock::DLOCK_BAD_RESPONSE;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "ClientReInitStagesUpdateLocks should return DLOCK_BAD_RESPONSE";
}

TEST_F(DlockTestSuite, TestReinitWithClientReinitFailedEnomem)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 因 ClientReInitStagesUpdateLocks 返回 DLOCK_ENOMEM 失败
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockUpdateAllLocksFunc = [](int clientId) -> int {
        return dlock::DLOCK_ENOMEM;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "ClientReInitStagesUpdateLocks should return DLOCK_ENOMEM";
}

TEST_F(DlockTestSuite, TestReinitWithClientReinitFailedUnknownError)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    auto &exec = DLockExecutor::GetInstance();

    int32_t ret = ctx.InitDlockClient();
    ASSERT_EQ(UBSM_OK, ret);

    // 因 ClientReInitStagesUpdateLocks 返回 -1 失败
    exec.DLockClientReinitFunc = [](int clientId, const char *ipStr) -> int {
        return dlock::DLOCK_SUCCESS;
    };
    exec.DLockUpdateAllLocksFunc = [](int clientId) -> int {
        return -1;
    };
    ret = UbsmLock::Instance().Reinit();
    ASSERT_EQ(MXM_ERR_DLOCK_INNER, ret) << "ClientReInitStagesUpdateLocks should return -1";
}

TEST_F(DlockTestSuite, TestNoNeedHeartbeat)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    ctx.SetHeartBeatStatus(false);

    auto ret = UbsmLock::Instance().Heartbeat();
    ASSERT_EQ(dlock::DLOCK_SUCCESS, ret);
}

TEST_F(DlockTestSuite, TestHeartbeatAllSuccess)
{
    auto &ctx = DLockContext::Instance();
    TestDefaultConfig();
    ctx.GetConfig().isDlockServer = false;
    ctx.GetConfig().isDlockClient = true;
    DefaultMock();
    ctx.InitDlockClient();
    ctx.SetHeartBeatStatus(true);

    auto ret = UbsmLock::Instance().Heartbeat();
    ASSERT_EQ(dlock::DLOCK_SUCCESS, ret);
}
} // namespace UT