/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>
#include "dlock_context.h"
#include "dlock_types.h"
#include "dlock_utils/ubsm_lock.h"
#include "dlock_utils/ubsm_lock_event.h"
#include "mxm_shm/rpc_server.h"
#include "system_adapter.h"
#include "zen_discovery/zen_discovery.h"

using namespace testing;
using namespace ock::dlock_utils;
using namespace ock::zendiscovery;
using namespace ock::ubsm;
using ZenElectionEventType = ElectionModule::ZenElectionEventType;

namespace UT {
class EventTestSuite : public Test {
protected:
    void SetUp() override
    {
        int pingTimeoutMs = 1000;
        int joinTimeoutMs = 1000;
        int electionTimeoutMs = 1000;
        int minimumMasterNodes = 1;
        ZenDiscovery::Initialize(pingTimeoutMs, joinTimeoutMs, electionTimeoutMs, minimumMasterNodes);
        UbsmLock::Instance().DeInit();
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

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
    ctx.GetConfig().serverIp = "192.168.1.1";
    ctx.GetConfig().clientIp = "192.168.1.1";
    ctx.GetConfig().dlockDevName = "dev";
    ctx.GetConfig().dlockDevEid = "0000:0000:0000:1000:0010:0000:df00:0404";
    ctx.GetConfig().dlockClientNum = 1;
}

TEST_F(EventTestSuite, TestHandleElectionEventDoPreElection)
{
    ZenElectionEventType eventType = ElectionModule::ZenElectionEventType::ELECTION_STARTED;
    std::string masterId = "192.168.1.1:1234";
    std::string clientId = "192.168.1.1:12345";

    MOCKER(UbsmLockEvent::DoPreElection).expects(once());
    MOCKER(UbsmLockEvent::OnDLockServerRecovery).expects(never());
    MOCKER(UbsmLockEvent::OnMasterElected).expects(never());
    MOCKER(UbsmLockEvent::OnDLockDemoted).expects(never());
    MOCKER(UbsmLockEvent::OnDLockClientInit).expects(never());
    UbsmLockEvent::HandleElectionEvent(eventType, masterId, clientId);
}

TEST_F(EventTestSuite, TestHandleElectionEventOnDLockServerRecovery)
{
    ZenElectionEventType eventType = ElectionModule::ZenElectionEventType::HAVE_JOINED;
    std::string masterId = "192.168.1.1:1234";
    std::string clientId = "192.168.1.1:12345";

    MOCKER(UbsmLockEvent::DoPreElection).expects(never());
    MOCKER(UbsmLockEvent::OnDLockServerRecovery).expects(once());
    MOCKER(UbsmLockEvent::OnMasterElected).expects(never());
    MOCKER(UbsmLockEvent::OnDLockDemoted).expects(never());
    MOCKER(UbsmLockEvent::OnDLockClientInit).expects(never());
    UbsmLockEvent::HandleElectionEvent(eventType, masterId, clientId);
}

TEST_F(EventTestSuite, TestHandleElectionEventOnMasterElected)
{
    ZenElectionEventType eventType = ElectionModule::ZenElectionEventType::MASTER_ELECTED;
    std::string masterId = "192.168.1.1:1234";
    std::string clientId = "192.168.1.1:12345";

    MOCKER(UbsmLockEvent::DoPreElection).expects(never());
    MOCKER(UbsmLockEvent::OnDLockServerRecovery).expects(never());
    MOCKER(UbsmLockEvent::OnMasterElected).expects(once());
    MOCKER(UbsmLockEvent::OnDLockDemoted).expects(never());
    MOCKER(UbsmLockEvent::OnDLockClientInit).expects(never());
    UbsmLockEvent::HandleElectionEvent(eventType, masterId, clientId);
}

TEST_F(EventTestSuite, TestHandleElectionEventOnDLockDemoted)
{
    ZenElectionEventType eventType = ElectionModule::ZenElectionEventType::NODE_DEMOTED;
    std::string masterId = "192.168.1.1:1234";
    std::string clientId = "192.168.1.1:12345";

    MOCKER(UbsmLockEvent::DoPreElection).expects(never());
    MOCKER(UbsmLockEvent::OnDLockServerRecovery).expects(never());
    MOCKER(UbsmLockEvent::OnMasterElected).expects(never());
    MOCKER(UbsmLockEvent::OnDLockDemoted).expects(once());
    MOCKER(UbsmLockEvent::OnDLockClientInit).expects(never());
    UbsmLockEvent::HandleElectionEvent(eventType, masterId, clientId);
}

TEST_F(EventTestSuite, TestHandleElectionEventOnDLockClientInit)
{
    ZenElectionEventType eventType = ElectionModule::ZenElectionEventType::BECOME_VOTE_NODE;
    std::string masterId = "192.168.1.1:1234";
    std::string clientId = "192.168.1.1:12345";

    MOCKER(UbsmLockEvent::DoPreElection).expects(never());
    MOCKER(UbsmLockEvent::OnDLockServerRecovery).expects(never());
    MOCKER(UbsmLockEvent::OnMasterElected).expects(never());
    MOCKER(UbsmLockEvent::OnDLockDemoted).expects(never());
    MOCKER(UbsmLockEvent::OnDLockClientInit).expects(once());
    UbsmLockEvent::HandleElectionEvent(eventType, masterId, clientId);
}

TEST_F(EventTestSuite, TestDoPreElection)
{
    auto &ctx = DLockContext::Instance();
    // 先将心跳打开
    ctx.SetHeartBeatStatus(true);
    UbsmLock::Instance().UbsmLockInitSet(true);
    ASSERT_TRUE(ctx.IsHeartBeatActive());
    ASSERT_TRUE(UbsmLock::Instance().IsUbsmLockInit());
    UbsmLockEvent::DoPreElection();
    ASSERT_FALSE(UbsmLock::Instance().IsUbsmLockInElection());
}

TEST_F(EventTestSuite, TestOnMasterElectedLocalIsNotMaster)
{
    std::string masterId = "192.168.1.2:1234";
    auto &rpcConfig = ock::rpc::NetRpcConfig::GetInstance();
    rpcConfig.SetLocalNode(std::make_pair("192.168.1.1", 1234)); // 1234 是模拟的端口号
    UbsmLockEvent::OnMasterElected(masterId);
    // 当本节点不是主节点的时候，UbsmLock将不会初始化
    ASSERT_FALSE(UbsmLock::Instance().IsUbsmLockInit());
}

TEST_F(EventTestSuite, TestOnMasterElectedWithNodeListAndLockNotInit)
{
    std::string masterId = "192.168.1.1:1234";
    auto &rpcConfig = ock::rpc::NetRpcConfig::GetInstance();
    rpcConfig.SetLocalNode(std::make_pair("192.168.1.1", 1234)); // 1234 是模拟的端口号
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    TestDefaultConfig();
    UbsmLockEvent::OnMasterElected(masterId);
    // 当本节点是主节点，会初始化 UbsmLock
    ASSERT_TRUE(UbsmLock::Instance().IsUbsmLockInit());
}
} // namespace UT