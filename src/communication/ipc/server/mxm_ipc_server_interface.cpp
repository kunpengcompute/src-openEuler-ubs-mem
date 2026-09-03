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

#include "mxm_ipc_server_interface.h"
#include "mxm_com_base.h"
#include "mxm_ipc_server.h"
#include "mxm_message_handler.h"
#include "mxm_msg.h"
#include "ubsm_com_constants.h"
#include "util/defines.h"

namespace ock::com::ipc {

MxmIpcServer *g_mxmIpcServer{nullptr};
std::atomic<int> g_ipcServerCount{0};

HRESULT RegIpcService(MxmComBaseMessageHandlerPtr &handlerPtr)
{
    if (g_mxmIpcServer != nullptr) {
        return g_mxmIpcServer->RegMessageHandler(handlerPtr);
    }
    return HOK;
}

/**
 * @brief 启动Ipc客户端
 * @return 成功返回0, 失败返回错误码
 */
int MxmComStartIpcServer()
{
    if (g_ipcServerCount.load() > 0 && g_mxmIpcServer != nullptr) {
        return HOK;
    }
    std::string udsPathPrefix = MXM_IPC_UDS_PATH_PREFIX_DEFAULT;
    int result = setenv(RACK_HCOM_FILE_PATH_PREFIX, udsPathPrefix.c_str(), 1);
    if (result != 0) {
        DBG_LOGERROR("Set env HCOM_FILE_PATH fail," << result);
        return HFAIL;
    }

    auto ipcServer = new (std::nothrow)
        MxmIpcServer(MXM_AGENT_IPC_SERVER_NAME, MXM_AGENT_IPC_SERVER_ENGINE_NAME, FAKE_CUR_NODE_ID, MIN_UDS_PERM);
    if (ipcServer == nullptr) {
        DBG_LOGERROR("ipcServer is nullptr.");
        return HFAIL;
    }

    auto ret = ipcServer->Start();
    if (ret != HOK) {
        DBG_LOGERROR("Start MxmIpcServer fail," << ret);
        ipcServer->Stop();
        delete ipcServer;
        ipcServer = nullptr;
        return HFAIL;
    }
    g_mxmIpcServer = ipcServer;
    g_ipcServerCount.fetch_add(1);
    return HOK;
}

/**
 * @brief 停止Ipc客户端
 */
void MxmComStopIpcServer()
{
    if (g_mxmIpcServer != nullptr) {
        g_mxmIpcServer->Stop();
        delete g_mxmIpcServer;
        g_mxmIpcServer = nullptr;
        g_ipcServerCount.fetch_sub(1);
    }
}

void MXMSetLinkEventHandler(const MXMLinkEventHandler &handler)
{
    g_mxmIpcServer->AddLinkNotifyFunc([handler](const std::vector<MxmLinkInfo> &linkInfoList) -> void {
        for (MxmLinkInfo info : linkInfoList) {
            if (info.GetState() == MxmLinkState::LINK_DOWN) {
                uint32_t pid = info.GetPID();
                handler(pid);
            }
        }
    });
}

uint32_t MxmRegIpcService(const MxmComEndpoint &endpoint, const MxmComIpcServiceHandler &handler)
{
    uint16_t moduleCode = endpoint.moduleId;
    auto opCode = static_cast<uint16_t>(endpoint.serviceId);
    MxmComBaseMessageHandlerPtr handlerPtr = new (std::nothrow) MxmNetMessageHandler(opCode, moduleCode, handler);
    if (handlerPtr == nullptr) {
        DBG_LOGERROR("New MxmComBaseMessageHandler fail");
        return HFAIL;
    }
    auto ret = HOK;

    ret = RegIpcService(handlerPtr);
    if (ret != HOK) {
        DBG_LOGERROR("Ipc Service fail, ret " << ret);
        return ret;
    }

    DBG_LOGINFO("Register moduleId: " << moduleCode << ",serviceId is " << opCode << " ret is " << ret);
    return ret;
}
} // namespace ock::com::ipc