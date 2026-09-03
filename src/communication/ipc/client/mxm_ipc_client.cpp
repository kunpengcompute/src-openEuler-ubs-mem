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

#include "mxm_ipc_client.h"

namespace ock::com::ipc {

static void IPCClientHandlerWork(void (*handler)(MxmComMessageCtx &messageCtx), MxmComMessageCtx &messageCtx)
{
    if (handler != nullptr) {
        handler(messageCtx);
    }
    messageCtx.FreeMessage();
}

HRESULT MxmIpcClient::Start()
{
    MxmComEngineInfo engineInfo{};
    engineInfo.SetEngineType(MxmEngineType::CLIENT);
    engineInfo.SetProtocol(MxmProtocol::UDS);
    engineInfo.SetWorkerMode(MxmWorkerMode::NET_BUSY_POLLING);
    engineInfo.SetName(name);
    engineInfo.SetNodeId(nodeId);
    engineInfo.SetWorkGroup("IPCClient");
    engineInfo.SetLogFunc(Log);
    return MxmCommunication::CreateMxmComEngine(engineInfo, LinkNotify, IPCClientHandlerWork);
}

void MxmIpcClient::Stop()
{
    MxmCommunication::DeleteMxmComEngine(name);
    ClearStateMap();
}

int MxmIpcClient::SetPostReconnectHandler(MxmComPostReconnectHandler handler)
{
    return MxmCommunication::SetPostReconnectHandler(name, std::move(handler));
}

HRESULT MxmIpcClient::Connect()
{
    HRESULT res = MxmCommunication::MxmComIpcConnect(name, udsPath, nodeId, MxmChannelType::SINGLE_SIDE);
    if (res != HOK) {
        DBG_LOGERROR("Failed to ipc connect, ret=" << res << ", engineName=" << name << ", nodeId=" << nodeId);
        return res;
    }
    return HOK;
}
} // namespace ock::com::ipc
