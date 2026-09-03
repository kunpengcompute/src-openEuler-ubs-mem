/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "rpc_test.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include "defines.h"
#include "hcom_service.h"
#include "mxm_com_def.h"
#include "mxm_com_engine.h"
#include "mxm_rpc_server.h"
#include "ubs_certify_handler.h"

namespace UT {
using namespace ock::com::rpc;
using namespace ock::com;
using namespace ock::common;
using namespace ock::ubsm;

void RpcTestHandler(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        return;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        return;
    }
    response->errCode_ = 0;
    response->name_ = NetRpcConfig::GetInstance().GetLocalNode().name;
}

void EmptyStub()
{
    printf("EmptyStub.\n");
}

int RpcServerTestMock()
{
    MOCKER_CPP(&UbsCertifyHandler::StartScheduledCertVerify, int (*)()).stubs().will(returnValue(HOK));
    MOCKER_CPP(&UbsCertifyHandler::StopScheduledCertVerify, void (*)()).stubs().will(invoke(EmptyStub));
    return MxmComStartRpcServer();
}

TEST_F(RpcTestSuite, TestRpcStartSuccess1)
{
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);
}

TEST_F(RpcTestSuite, TestRpcStartSuccess)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);
}

TEST_F(RpcTestSuite, TestRpcServiceRegSuccess)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);

    MxmComEndpoint endPoint;
    endPoint.address = "test";
    endPoint.moduleId = 1;
    endPoint.serviceId = 1;
    MOCKER_CPP(&MxmCommunication::RegMxmComMsgHandler,
               HRESULT(*)(const std::string &engineName, const MxmComMsgHandler &handle))
        .stubs()
        .will(returnValue(HOK));
    ret = MxmRegRpcService(endPoint, RpcTestHandler);
    EXPECT_EQ(ret, HOK);
}

TEST_F(RpcTestSuite, TestRpcServiceRegFail)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);

    MxmComEndpoint endPoint;
    endPoint.address = "test";
    ret = MxmRegRpcService(endPoint, RpcTestHandler);
    EXPECT_NE(ret, 0);
}

TEST_F(RpcTestSuite, TestRpcConnectSuccess)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);
    RpcNode node = {"127.0.0.1:7201", "127.0.0.1", 7201};
    MOCKER_CPP(&MxmCommunication::MxmComRpcConnect,
               HRESULT(*)(const std::string &engineName, const RpcNode &remoteNodeId, const std::string &nodeId,
                          MxmChannelType chType))
        .stubs()
        .will(returnValue(HOK));
    ret = MxmComRpcServerConnect(node);
    EXPECT_EQ(ret, HOK);
}

TEST_F(RpcTestSuite, TestRpcConnectFail01)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    RpcNode node = {"127.0.0.1:7201", "", 7201};
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);
    ret = MxmComRpcServerConnect(node);
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(RpcTestSuite, TestRpcConnectFail02)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    RpcNode node = {"127.0.0.1:7201", "127.0.0.1", 7201};
    auto ret = MxmComRpcServerConnect(node);
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(RpcTestSuite, TestRpcMessageSendSuccess)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);

    MxmComEndpoint endPoint;
    endPoint.address = "test";
    endPoint.moduleId = 1;
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<RpcQueryInfoResponse>();
    std::string nodeId = "127.0.0.1:7201";
    MOCKER_CPP(&MxmCommunication::MxmComMsgSend,
               HRESULT(*)(const std::string &engineName, MxmComMessageCtx &message, MxmComDataDesc &retData))
        .stubs()
        .will(returnValue(HOK));
    ret = MxmComRpcServerSend(endPoint.serviceId, request.get(), response.get(), nodeId);
    EXPECT_EQ(ret, HOK);
}

TEST_F(RpcTestSuite, TestRpcMessageSendFail01)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    auto ret = RpcServerTestMock();
    EXPECT_EQ(ret, HOK);
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<RpcQueryInfoResponse>();
    MxmComEndpoint endPoint;
    endPoint.address = "test";
    endPoint.moduleId = 1;
    std::string nodeId = "127.0.0.1:7201";
    MOCKER_CPP(&MxmCommunication::MxmComMsgSend,
               HRESULT(*)(const std::string &engineName, MxmComMessageCtx &message, MxmComDataDesc &retData))
        .stubs()
        .will(returnValue(HFAIL));

    ret = MxmComRpcServerSend(endPoint.serviceId, request.get(), response.get(), nodeId);
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(RpcTestSuite, TestRpcMessageSendFail02)
{
    MOCKER_CPP(&MxmRpcServer::Start, HRESULT(*)()).stubs().will(returnValue(HOK));
    MxmComEndpoint endPoint;
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<RpcQueryInfoResponse>();
    endPoint.address = "test";
    endPoint.moduleId = 1;
    std::string nodeId = "127.0.0.1:7201";
    auto ret = MxmComRpcServerSend(endPoint.serviceId, request.get(), response.get(), nodeId);
    EXPECT_EQ(ret, HFAIL);
}
} // namespace UT