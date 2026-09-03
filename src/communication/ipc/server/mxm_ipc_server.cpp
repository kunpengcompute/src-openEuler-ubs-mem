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

#include "mxm_ipc_server.h"
#include "configuration.h"
#include "ubsm_thread_pool.h"

namespace ock::com::ipc {
using namespace ock::common;

static void IpcServerWork(void (*handler)(MxmComMessageCtx &messageCtx), MxmComMessageCtx &messageCtx)
{
    MxmComMessageCtx ctx = std::move(messageCtx);
    if (handler != nullptr) {
        UBSMThreadPool::GetInstance().Push([handler, ctx]() mutable {
            try {
                handler(ctx);
            } catch (std::exception &e) {
                DBG_LOGERROR("exception " << e.what() << " was caught");
            } catch (...) {
                DBG_LOGERROR("unknown exception was caught.");
            }
            ctx.FreeMessage();
        });
    } else {
        ctx.FreeMessage();
    }
}

HRESULT MxmIpcServer::Start()
{
    MxmComEngineInfo engineInfo{};
    engineInfo.SetEngineType(MxmEngineType::SERVER);
    engineInfo.SetProtocol(MxmProtocol::UDS);
    engineInfo.SetWorkerMode(MxmWorkerMode::NET_EVENT_POLLING);
    engineInfo.SetName(name);
    engineInfo.SetNodeId(nodeId);
    engineInfo.SetUdsInfo(std::make_pair(udsPath, udsMode));
    engineInfo.SetWorkGroup("IPCServer");
    engineInfo.SetLogFunc(Log);
    engineInfo.SetReconnectHook([](std::string remoteId, MxmChannelType type) -> bool { return false; });

    auto conf = Configuration::GetInstance();
    if (conf == nullptr) {
        DBG_LOGERROR("Configuration::GetInstance() is nullptr.");
        return HFAIL;
    }
    auto maxNum = conf->GetInt(ock::common::ConfConstant::UBSMD_MAX_HCOM_CONNECT_NUM.first);
    engineInfo.SetMaxHcomConnectNum(maxNum);
    DBG_LOGINFO("options.confPath is " << conf->GetConfigPath() << "num is " << maxNum);
    return MxmCommunication::CreateMxmComEngine(engineInfo, LinkNotify, IpcServerWork);
}

void MxmIpcServer::Stop()
{
    MxmCommunication::DeleteMxmComEngine(name);
}
} // namespace ock::com::ipc
