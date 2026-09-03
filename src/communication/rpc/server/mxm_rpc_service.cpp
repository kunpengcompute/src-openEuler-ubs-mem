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
#include "mxm_rpc_server.h"
#include "ubs_certify_handler.h"
#include "ubs_common_config.h"

namespace ock::com::rpc {

static void RPCServerHandlerWork(void (*handler)(MxmComMessageCtx &messageCtx), MxmComMessageCtx &messageCtx)
{
    if (handler != nullptr) {
        handler(messageCtx);
    }
    messageCtx.FreeMessage();
}
HRESULT MxmRpcServer::Start()
{
    auto ret = ock::ubsm::UbsCertifyHandler::GetInstance().StartScheduledCertVerify();
    if (ret != 0) {
        return HFAIL;
    }
    MxmComEngineInfo engineInfo{};
    engineInfo.SetEngineType(MxmEngineType::SERVER);
    engineInfo.SetProtocol(MxmProtocol::TCP);
    engineInfo.SetWorkerMode(MxmWorkerMode::NET_EVENT_POLLING);
    engineInfo.SetName(name);
    engineInfo.SetWorkGroup("RPCServer");
    engineInfo.SetLogFunc(Log);
    engineInfo.SetIpInfo(ipAndPort);
    engineInfo.SetEnableTls(ock::ubsm::UbsCommonConfig::GetInstance().GetTlsSwitch());
    engineInfo.SetCipherSuite(ock::ubsm::UbsCommonConfig::GetInstance().GetCipherSuit());
    engineInfo.SetReconnectHook([](std::string remoteId, MxmChannelType type) -> bool { return false; });
    return MxmCommunication::CreateMxmComEngine(engineInfo, LinkNotify, RPCServerHandlerWork);
}

void MxmRpcServer::Stop()
{
    MxmCommunication::DeleteMxmComEngine(name);
    ock::ubsm::UbsCertifyHandler::GetInstance().StopScheduledCertVerify();
}

HRESULT MxmRpcServer::Connect(const RpcNode &remoteNodeId)
{
    HRESULT res = MxmCommunication::MxmComRpcConnect(name, remoteNodeId, nodeId, MxmChannelType::NORMAL);
    if (res != HOK) {
        DBG_LOGERROR("MxmRpcServer::Connect failed, ret: " << res);
        return res;
    }
    DBG_LOGDEBUG("MxmRpcServer::Connect success, node=" << remoteNodeId.ip);
    return HOK;
}
} // namespace ock::com::rpc
