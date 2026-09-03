/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "mxm_rpc_server_interface.h"
#include "mxm_message_handler.h"
#include "mxm_rpc_server.h"
#include "rpc_config.h"
#include "util/defines.h"

namespace ock::com::rpc {
MxmRpcServer *g_mxmRpcServer{nullptr};
std::atomic<int> g_rpcServerCount{0};

const std::string RACK_AGENT_RPC_SERVER_ENGINE_NAME = "MxmAgentRpcServerEngine";

HRESULT RegRpcService(MxmComBaseMessageHandlerPtr &handlerPtr)
{
    if (g_mxmRpcServer != nullptr) {
        return g_mxmRpcServer->RegMessageHandler(handlerPtr);
    }
    return HOK;
}

/**
 * @brief 启动Rpc客户端
 * @return 成功返回0, 失败返回错误码
 */
int MxmComStartRpcServer()
{
    if (g_rpcServerCount.load() > 0 && g_mxmRpcServer != nullptr) {
        return HOK;
    }

    auto ipAndPort = NetRpcConfig::GetInstance().GetLocalNode();
    DBG_LOGINFO("Start Rpc Server, local node name=" << ipAndPort.name << ", ip=" << ipAndPort.ip);
    auto rpcServer = new (std::nothrow) MxmRpcServer(RACK_AGENT_RPC_SERVER_ENGINE_NAME, ipAndPort);
    if (rpcServer == nullptr) {
        DBG_LOGERROR("rpcServer is nullptr.");
        return HFAIL;
    }

    auto ret = rpcServer->Start();
    if (ret != HOK) {
        DBG_LOGERROR("Start MxmRpcServer fail," << ret);
        rpcServer->Stop();
        delete rpcServer;
        rpcServer = nullptr;
        return HFAIL;
    }
    g_mxmRpcServer = rpcServer;
    g_rpcServerCount.fetch_add(1);
    return HOK;
}

/**
 * @brief 停止Rpc客户端
 */
void MxmComStopRpcServer()
{
    if (g_rpcServerCount.load() == 0) {
        DBG_LOGINFO("Server has been stopped");
        return;
    }
    g_rpcServerCount.fetch_sub(1);
    if (g_mxmRpcServer != nullptr) {
        g_mxmRpcServer->Stop();
        delete g_mxmRpcServer;
        g_mxmRpcServer = nullptr;
    }
}

uint32_t MxmRegRpcService(const MxmComEndpoint &endpoint, const MxmComServiceHandler &handler)
{
    uint16_t moduleCode = endpoint.moduleId;
    auto opCode = static_cast<uint16_t>(endpoint.serviceId);
    MxmComBaseMessageHandlerPtr handlerPtr = new (std::nothrow) MxmNetMessageHandler(opCode, moduleCode, handler);
    if (handlerPtr == nullptr) {
        DBG_LOGERROR("New MxmComBaseMessageHandler fail");
        return HFAIL;
    }
    auto ret = HOK;

    ret = RegRpcService(handlerPtr);
    if (ret != HOK) {
        DBG_LOGERROR("Rpc Service fail, ret " << ret);
        return ret;
    }

    DBG_LOGINFO("serviceId is " << opCode << " ret is " << ret);
    return ret;
}

int MxmComRpcServerSend(uint16_t opCode, MsgBase *request, MsgBase *response, const std::string &nodeId)
{
    SendParam param(nodeId, static_cast<uint16_t>(MxmModuleCode::MEM), opCode, MxmChannelType::NORMAL);
    if (g_mxmRpcServer == nullptr) {
        DBG_LOGERROR("g_mxmRpcServer is nullptr");
        return HFAIL;
    }
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("request or response is nullptr");
        return HFAIL;
    }
    auto ret = g_mxmRpcServer->Send(param, request, response);
    return ret;
}

int MxmComRpcServerConnect(const RpcNode &nodeId)
{
    if (g_mxmRpcServer == nullptr) {
        DBG_LOGERROR("g_mxmRpcServer is nullptr");
        return HFAIL;
    }
    auto ret = g_mxmRpcServer->Connect(nodeId);
    return ret;
}
} // namespace ock::com::rpc