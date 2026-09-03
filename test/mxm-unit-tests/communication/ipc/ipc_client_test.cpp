/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "ipc_client_test.h"
#include <sys/socket.h>
#include "mxm_com_engine.h"

namespace UT {
using namespace ock::com;

void MockTest()
{
    MOCKER(MxmComStartIpcClient).stubs().will(returnValue(0));
    MOCKER(MxmComStopIpcClient).stubs().will(returnValue(0));
}

TEST_F(IpcClinetTestSuite, TestIpcClientStartSuccess)
{
    MOCKER_CPP(&MxmCommunication::MxmComIpcConnect,
               HRESULT(*)(const std::string &engineName, const std::string &udsPath, const std::string &nodeId,
                          MxmChannelType chType))
        .stubs()
        .will(returnValue(HOK));

    auto ret = MxmComStartIpcClient();
    EXPECT_EQ(ret, HOK);
}

TEST_F(IpcClinetTestSuite, TestIpcClientStartFail)
{
    MOCKER_CPP(&MxmCommunication::MxmComIpcConnect,
               HRESULT(*)(const std::string &engineName, const std::string &udsPath, const std::string &nodeId,
                          MxmChannelType chType))
        .stubs()
        .will(returnValue(HFAIL));
    auto ret = MxmComStartIpcClient();
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(IpcClinetTestSuite, TestIpcClientSendSuccess)
{
    MOCKER_CPP(&MxmCommunication::CreateMxmComEngine,
               HRESULT(*)(const MxmComEngineInfo &engine, const MxmComLinkStateNotify &notify,
                          const EngineHandlerWorker &handlerWorker))
        .stubs()
        .will(returnValue(HOK));
    MOCKER_CPP(&MxmCommunication::MxmComIpcConnect,
               HRESULT(*)(const std::string &engineName, const std::string &udsPath, const std::string &nodeId,
                          MxmChannelType chType))
        .stubs()
        .will(returnValue(HOK));
    auto ret = MxmComStartIpcClient();
    EXPECT_EQ(ret, HOK);
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<CommonResponse>();
    MOCKER_CPP(&MxmCommunication::MxmComMsgSend,
               HRESULT(*)(const std::string &engineName, MxmComMessageCtx &message, MxmComDataDesc &retData))
        .stubs()
        .will(returnValue(HOK));
    ret = MxmComIpcClientSend(1, request.get(), response.get());
    EXPECT_EQ(ret, HOK);
}

TEST_F(IpcClinetTestSuite, TestIpcClientSendFail01)
{
    MockTest();
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<CommonResponse>();
    auto ret = MxmComIpcClientSend(1, request.get(), response.get());
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(IpcClinetTestSuite, TestIpcClientSendFail02)
{
    MockTest();
    MOCKER_CPP(&MxmCommunication::MxmComIpcConnect,
               HRESULT(*)(const std::string &engineName, const std::string &udsPath, const std::string &nodeId,
                          MxmChannelType chType))
        .stubs()
        .will(returnValue(HOK));
    auto ret = MxmComStartIpcClient();
    EXPECT_EQ(ret, HOK);
    auto request = std::make_shared<CommonRequest>();
    auto response = std::make_shared<CommonResponse>();
    ret = MxmComIpcClientSend(1, request.get(), response.get());
    EXPECT_NE(ret, 0);
}
} // namespace UT