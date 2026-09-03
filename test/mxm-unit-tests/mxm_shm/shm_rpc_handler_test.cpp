/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "shm_rpc_handler_test.h"
#include "dlock_context.h"
#include "mxm_shm/rpc_handler.h"
#include "ubsm_lock.h"
#include "zen_discovery.h"

using namespace UT;
using namespace ock::rpc::service;
using namespace ock::zendiscovery;
TEST_F(ShmRpcHandlerTestSuite, TestShmemNull)
{
    auto ret = MxmServerMsgHandle::QueryNodeInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::PingRequestInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::JoinRequestInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::VoteRequestInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::SendTransElectedInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::MasterElectedInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::BroadCastRequestInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::DLockClientReinit(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::PingRequestInfo(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::HandleMemLock(nullptr, nullptr);
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestQueryNodeInfo)
{
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::QueryNodeInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmRpcHandlerTestSuite, TestQueryNodeInfoErrRequest)
{
    auto request = std::make_shared<PingRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::QueryNodeInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestPingRequestInfo)
{
    MOCKER_CPP(&ZenDiscovery::HandlePingRequest, void (*)(const std::string &fromNodeId))
        .stubs()
        .will(returnValue(nullptr));
    auto request = std::make_shared<PingRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::PingRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmRpcHandlerTestSuite, TestPingRequestInfoErrRequest)
{
    auto request = std::make_shared<RpcQueryInfoResponse>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::PingRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestPingRequestInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));

    auto request = std::make_shared<PingRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::PingRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestJoinRequestInfo)
{
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::HandleJoinRequest,
               ock::rpc::NodeType(*)(const std::string &fromNodeId))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<PingRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcJoinInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::JoinRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmRpcHandlerTestSuite, TestJoinRequestInfoErrRequest)
{
    auto request = std::make_shared<RpcJoinInfoResponse>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcJoinInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::JoinRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestJoinRequestInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));

    auto request = std::make_shared<PingRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcJoinInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::JoinRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestVoteRequestInfo)
{
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::HandleVoteRequest,
               bool (*)(const std::string &fromNodeId, const std::string &candidate, int term))
        .stubs()
        .will(returnValue(true));

    auto request = std::make_shared<VoteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcVoteInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::VoteRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->isGranted_, true);
}

TEST_F(ShmRpcHandlerTestSuite, TestVoteRequestInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));

    auto request = std::make_shared<VoteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcVoteInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::VoteRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestSendTransElectedInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));

    auto request = std::make_shared<TransElectedRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::SendTransElectedInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestSendTransElectedInfo)
{
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::HandleSendTransElected,
               void (*)(const std::string &fromNodeId, const std::vector<std::string> &nodeList, int term))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<TransElectedRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::SendTransElectedInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->name_, "Transfer Master Elected Notification Processed");
}

TEST_F(ShmRpcHandlerTestSuite, TestMasterElectedInfo)
{
    auto request = std::make_shared<VoteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::MasterElectedInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->name_, "Master Elected Notification Processed");
}

TEST_F(ShmRpcHandlerTestSuite, TestMasterElectedInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));
    auto request = std::make_shared<VoteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::MasterElectedInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestBroadCastRequestInfoNull)
{
    ZenDiscovery *zen = nullptr;
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::GetInstance, ock::zendiscovery::ZenDiscovery * (*)())
        .stubs()
        .will(returnValue(zen));
    auto request = std::make_shared<VoteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::BroadCastRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, ock::common::MXM_ERR_NULLPTR);
}

TEST_F(ShmRpcHandlerTestSuite, TestBroadCastRequestInfo)
{
    MOCKER_CPP(&ock::zendiscovery::ZenDiscovery::HandleBroadCastRequest,
               void (*)(const std::string &fromNodeId, const std::map<std::string, ock::rpc::ClusterNode> &nodeList,
                        bool isSeverInited))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<BroadcastRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::BroadCastRequestInfo(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->name_, "Broadcast Request Processed");
}

TEST_F(ShmRpcHandlerTestSuite, TestDLockClientReinit)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Reinit, int32_t(*)()).stubs().will(returnValue(0));
    auto request = std::make_shared<DLockClientReinitRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<DLockClientReinitResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::DLockClientReinit(request.get(), response.get());
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmRpcHandlerTestSuite, TestDLockClientReinitFail)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Reinit, int32_t(*)()).stubs().will(returnValue(-1));
    auto request = std::make_shared<DLockClientReinitRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<DLockClientReinitResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::DLockClientReinit(request.get(), response.get());
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}

TEST_F(ShmRpcHandlerTestSuite, TestHandleMemLock)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Lock,
               int32_t(*)(const std::string &name, bool isExclusive, ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<LockRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<DLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::HandleMemLock(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->dLockCode_, -1);
}

TEST_F(ShmRpcHandlerTestSuite, TestHandleMemUnLock)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Unlock,
               int32_t(*)(const std::string &name, const ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<UnLockRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<DLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto ret = MxmServerMsgHandle::HandleMemUnLock(request.get(), response.get());
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->dLockCode_, -1);
}
