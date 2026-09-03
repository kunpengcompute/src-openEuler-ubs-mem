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

#include "ipc_server.h"
#include "ipc_server_engine.h"
#include "log.h"

#include "ipc_handler.h"
#include "mls_manager.h"
#include "mxm_ipc_server_interface.h"
#include "referable/dg_ref.h"
#include "ubse_mem_adapter.h"

using namespace ock::mxmd;
using namespace ock::com;
using namespace ock::com::ipc;
using namespace ock::dagger;
using namespace ock::ubsm;

namespace ock::lease::service {
static std::map<uint32_t, MsgExecFuc> execMap;
static std::unordered_map<int, HcomServiceHandle> callBackMap;

void IpcServer::Handle(const MxmComUdsInfo &udsInfo, uint16_t opCode, const MsgBase *req, MsgBase *rsp,
                       const MsgExecFuc &exePtr)
{
    if (exePtr == nullptr) {
        DBG_LOGERROR("Execution function is nullptr, opCode=" << opCode);
        return;
    }
    DBG_LOGDEBUG("server handle ipc msg, opCode=" << opCode);
    MxmComUdsInfo ctx{udsInfo.pid, udsInfo.uid, udsInfo.gid};
    auto ret = exePtr(req, rsp, ctx);
    if (ret != 0) {
        DBG_LOGERROR("server handle msg failed, ret=" << ret << ", opCode=" << opCode);
    }
}

const std::unordered_map<int, HcomServiceHandle> &IpcServer::GetIpcCallbackTable()
{
    IpcServer::GetInstance().InitExecMap();
    callBackMap.clear();
    callBackMap[IPC_MALLOC_MEMORY] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_MALLOC_MEMORY, req, rsp, execMap.at(IPC_MALLOC_MEMORY));
    }};
    callBackMap[IPC_MALLOC_MEMORY_LOC] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_MALLOC_MEMORY_LOC, req, rsp, execMap.at(IPC_MALLOC_MEMORY_LOC));
    }};
    callBackMap[IPC_CHECK_MEMORY_LEASE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_CHECK_MEMORY_LEASE, req, rsp, execMap.at(IPC_CHECK_MEMORY_LEASE));
    }};
    callBackMap[IPC_FREE_RACKMEM] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_FREE_RACKMEM, req, rsp, execMap.at(IPC_FREE_RACKMEM));
    }};
    callBackMap[IPC_QUERY_CLUSTERINFO] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_QUERY_CLUSTERINFO, req, rsp, execMap.at(IPC_QUERY_CLUSTERINFO));
    }};
    callBackMap[IPC_FORCE_FREE_CACHED_MEMORY] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_FORCE_FREE_CACHED_MEMORY, req, rsp,
                                execMap.at(IPC_FORCE_FREE_CACHED_MEMORY));
    }};
    callBackMap[IPC_QUERY_CACHED_MEMORY] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_QUERY_CACHED_MEMORY, req, rsp, execMap.at(IPC_QUERY_CACHED_MEMORY));
    }};
    return callBackMap;
}

uint32_t IpcServer::InitializeIpcCallbackTable()
{
    auto map = IpcServer::GetInstance().GetIpcCallbackTable();
    for (auto &op : map) {
        MxmComEndpoint endpoint;
        endpoint.address = "mem_lease";
        endpoint.moduleId = 0x0;
        endpoint.serviceId = op.first;
        uint32_t hr = IpcServerEngine::GetInstance().RegisterOpcode(endpoint, op.second);
        if (BresultFail(hr)) {
            return hr;
        }
    }
    return HOK;
}

uint32_t IpcServer::InitExecMap()
{
    DBG_LOGINFO("start init exec map");
    execMap.emplace(IPC_MALLOC_MEMORY, MxmServerMsgHandle::AppMallocMemory);
    execMap.emplace(IPC_MALLOC_MEMORY_LOC, MxmServerMsgHandle::AppMallocMemoryWithLoc);
    execMap.emplace(IPC_CHECK_MEMORY_LEASE, MxmServerMsgHandle::AppCheckMemoryLease);
    execMap.emplace(IPC_FREE_RACKMEM, MxmServerMsgHandle::AppFreeMemory);
    execMap.emplace(IPC_QUERY_CLUSTERINFO, MxmServerMsgHandle::AppQueryClusterInfo);
    execMap.emplace(IPC_FORCE_FREE_CACHED_MEMORY, MxmServerMsgHandle::AppForceFreeCachedMemory);
    execMap.emplace(IPC_QUERY_CACHED_MEMORY, MxmServerMsgHandle::AppQueryCachedMemory);
    for (auto &iter : execMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("RPC init exe map error, no func");
            return MXM_ERR_REGISTER_HANDLER_NULL;
        }
    }
    DBG_LOGINFO("started init exec map");
    return HOK;
}

uint32_t IpcServer::RackMemConBaseInitialize()
{
    IpcServerEngine::GetInstance().Initialize();
    auto hr = IpcServer::GetInstance().InitializeIpcCallbackTable();
    if (BresultFail(hr)) {
        DBG_LOGERROR("RackMemConInitializeRpcCallbackTable failed!");
        return hr;
    }
    DBG_LOGINFO("Mxm lease hpc Server ipc is initialized");
    return HOK;
}

uint32_t IpcServer::Stop()
{
    return HOK;
}
} // namespace ock::lease::service