/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "mxm_com.h"
#include <sys/socket.h>
#ifdef DEBUG_MEM_UT
#define private public
#define protected public
#endif
#include "mxm_com_base.h"
#include "mxm_ipc_client.h"
#include "mxm_message_handler.h"

namespace UT {
using namespace ock::com;

TEST_F(MxmComTestSuite, TestRemoveHandler)
{
    auto opCode = 11;     // 11
    auto moduleCode = 22; // 22
    MxmComIpcServiceHandler handler{};
    MxmComBaseMessageHandlerPtr handlerPtr = new (std::nothrow) MxmNetMessageHandler(opCode, moduleCode, handler);
    auto engineName = "test_engine";
    auto ptr = MxmComBaseMessageHandlerManager::GetHandler(moduleCode, opCode, engineName);
    EXPECT_EQ(ptr, nullptr);

    MxmComBaseMessageHandlerManager::AddHandler(handlerPtr, engineName);
    ptr = MxmComBaseMessageHandlerManager::GetHandler(moduleCode, opCode, engineName);
    EXPECT_NE(ptr, nullptr);

    MxmComBaseMessageHandlerManager::RemoveHandler(moduleCode, opCode, engineName);
}

TEST_F(MxmComTestSuite, TestMxmComBaseNotify)
{
    MxmComEngineInfo info{};
    auto pid = 222; // 222
    auto engineName = "test_engine";
    info.SetName(engineName);
    MxmComBase::LinkNotify(info, "1", pid, MxmLinkState::LINK_DOWN);
    MxmComBase::LinkNotify(info, "1", pid, MxmLinkState::LINK_STATE_UNKNOWN);
    MxmComBase::LinkNotify(info, "1", pid, MxmLinkState::LINK_UP);
    auto vec = MxmComBase::GetLinkInfoFromMap(engineName, pid);
    EXPECT_NE(vec.size(), 0);

    LinkNotifyFunction executor = [](const std::vector<MxmLinkInfo> &) {
        std::cout << "LinkNotifyFunction..." << std::endl;
    };
    ipc::MxmIpcClient ipcClient("/uds", "UT", "1");
    ipcClient.AddLinkNotifyFunc(executor);
}

TEST_F(MxmComTestSuite, TestMxmComBaseInterface)
{
    HandlerExecutor executor = [](const std::function<void()> &task) {
        std::cout << "Executing task..." << std::endl;
        task(); // 执行传入的任务
        std::cout << "Task executed." << std::endl;
    };
    MxmComBase::SetHandlerExecutor(executor);
    MxmComBase::SetIpcHandlerExecutor(executor);
    LinkEventHandler eventHandler = [](const std::vector<MxmLinkInfo> &linkInfoList) {
        std::cout << "linkInfoList size " << linkInfoList.size() << std::endl;
    };
    MxmComBase::SetLinkEventHandler(eventHandler);
    MxmComMessageCtx msgCtx{};
    auto respPtr = CreateResponseByOpCode(IPC_REGION_LOOKUP_REGION_LIST);
    EXPECT_NE(respPtr, nullptr);

    Reply(msgCtx, respPtr);
    delete respPtr;
}

TEST_F(MxmComTestSuite, TestMxmComBaseInterface2)
{
    MxmComEngineInfo info{};
    info.SetMaxSendReceiveSize(4096);  // 4096
    info.SetSendReceiveSegCount(4096); // 4096
    auto res = info.IsServerSide();
    res = info.IsUds();
    info.GetReConnectHook();
    info.GetCipherSuite();
    info.SetNodeId("1");
    auto node = info.GetNodeId();
    EXPECT_NE(node, "");

    info.SetWorkGroup("test_group");
    auto group = info.GetWorkGroup();
    EXPECT_NE(group, "");

    std::pair<std::string, uint16_t> udsInfo;
    udsInfo.first = "testUds";
    info.SetUdsInfo(udsInfo);
    auto uds = info.GetUdsInfo();
    EXPECT_EQ(udsInfo.first, "testUds");
}

TEST_F(MxmComTestSuite, TestMxmComBaseInterface3)
{
    MxmComChannelConnectInfo channelConnectInfo{};
    channelConnectInfo.SetIp("127.0.0.1");
    channelConnectInfo.SetPort(6379); // 6379
    auto port = channelConnectInfo.GetPort();
    EXPECT_EQ(port, 6379);
    channelConnectInfo.SetIsUds(true);
    auto isUds = channelConnectInfo.IsUds();
    EXPECT_EQ(isUds, true);
    channelConnectInfo.SetRemoteNodeId("2");
    channelConnectInfo.SetCurNodeId("1");
}
TEST_F(MxmComTestSuite, TestMxmComBaseInterface4)
{
    MxmComChannelInfo info{};
    info.SetIsServer(true);
    UBSHcomNetUdsIdInfo uds{};
    info.SetUDSInfo(uds);
    UBSHcomChannelPtr channelPtr;
    info.SetChannel(channelPtr);
    MxmComChannelConnectInfo connectInfo{};
    info.SetConnectInfo(connectInfo);
    info.SetChannelType(MxmChannelType::NORMAL);
    info.SetEngineName("engineName");
    StringToChannelType("Normal");
    StringToChannelType("Emergency");
    StringToChannelType("Heartbeat");
    StringToChannelType("");

    auto isServer = info.IsServerSide();
    auto udsInfo = info.GetUDSInfo();
    auto channel = info.GetChannel();
    info.GetConnectInfo();
    auto type = info.GetChannelType();
    EXPECT_EQ(type, MxmChannelType::NORMAL);
    auto name = info.GetEngineName();
    EXPECT_EQ(name, "engineName");
}

TEST_F(MxmComTestSuite, TestMxmComBaseInterface5)
{
    MxmComMessageHead message{};
    message.GetOpCode();
    message.GetModuleCode();
    message.GetBodyLen();
    message.GetCrc();
    MxmComMessageCtx ctx{};
    MxmComMessagePtr messagePtr = new uint8_t[10]; // 10
    ctx.SetMessage(messagePtr, 10);
    ctx.SetRspCtx(ctx.GetRspCtx());
    ctx.SetSrcId(ctx.GetSrcId());
    ctx.SetChannelId(ctx.GetChannelId());
    ctx.SetReplyFuncHook(ctx.GetReplyFuncHook());
    ctx.SetUdsInfo(ctx.GetUdsInfo());
    ctx.SetDstId("3");
    auto dstId = ctx.GetDstId();
    EXPECT_EQ(dstId, "3");

    UBSHcomServiceContext context{};
    MxmUdsIdInfo udsIdInfo{};
    delete[] messagePtr;
    ctx.FreeMessage();
}
} // namespace UT