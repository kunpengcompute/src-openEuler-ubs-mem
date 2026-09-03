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
#include "mxm_ipc_server_interface.h"
#include "referable/dg_ref.h"
#include "region_manager.h"
#include "shm_manager.h"

using namespace ock::mxmd;
using namespace ock::com;
using namespace ock::com::ipc;
using namespace ock::dagger;

namespace ock::share::service {
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

void IpcServer::GetIpcCallbackTablePartition()
{
    callBackMap[IPC_RACKMEMSHM_ATTACH] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_ATTACH, req, rsp, execMap.at(IPC_RACKMEMSHM_ATTACH));
    }};
    callBackMap[IPC_RACKMEMSHM_DETACH] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_DETACH, req, rsp, execMap.at(IPC_RACKMEMSHM_DETACH));
    }};
    callBackMap[IPC_RACKMEMSHM_LOOKUP_LIST] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_LOOKUP_LIST, req, rsp, execMap.at(IPC_RACKMEMSHM_LOOKUP_LIST));
    }};
    callBackMap[IPC_RACKMEMSHM_LOOKUP] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_LOOKUP, req, rsp, execMap.at(IPC_RACKMEMSHM_LOOKUP));
    }};
    callBackMap[IPC_RACKMEMSHM_QUERY_NODE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_QUERY_NODE, req, rsp, execMap.at(IPC_RACKMEMSHM_QUERY_NODE));
    }};
    callBackMap[IPC_RACKMEMSHM_QUERY_DLOCK_STATUS] = {
        [](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
            GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_QUERY_DLOCK_STATUS, req, rsp,
                                    execMap.at(IPC_RACKMEMSHM_QUERY_DLOCK_STATUS));
        }};
    callBackMap[RPC_AGENT_QUERY_NODE_INFO] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, RPC_AGENT_QUERY_NODE_INFO, req, rsp, execMap.at(RPC_AGENT_QUERY_NODE_INFO));
    }};
}

const std::unordered_map<int, HcomServiceHandle> &IpcServer::GetIpcCallbackTable()
{
    IpcServer::GetInstance().InitExecMap();
    callBackMap.clear();
    callBackMap[MXM_MSG_SHM_ALLOCATE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, MXM_MSG_SHM_ALLOCATE, req, rsp, execMap.at(MXM_MSG_SHM_ALLOCATE));
    }};
    callBackMap[MXM_MSG_SHM_DEALLOCATE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, MXM_MSG_SHM_DEALLOCATE, req, rsp, execMap.at(MXM_MSG_SHM_DEALLOCATE));
    }};
    callBackMap[IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS] = {
        [](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
            GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, req, rsp,
                                    execMap.at(IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS));
        }};
    callBackMap[IPC_RACKMEMSHM_LOOKUP_REGIONINFO] = {
        [](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
            GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_LOOKUP_REGIONINFO, req, rsp,
                                    execMap.at(IPC_RACKMEMSHM_LOOKUP_REGIONINFO));
        }};
    callBackMap[IPC_RACKMEMSHM_CREATE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_CREATE, req, rsp, execMap.at(IPC_RACKMEMSHM_CREATE));
    }};
    callBackMap[IPC_RACKMEMSHM_CREATE_WITH_PROVIDER] = {
        [](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
            GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_CREATE_WITH_PROVIDER, req, rsp,
                                    execMap.at(IPC_RACKMEMSHM_CREATE_WITH_PROVIDER));
        }};
    callBackMap[IPC_RACKMEMSHM_DELETE] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_DELETE, req, rsp, execMap.at(IPC_RACKMEMSHM_DELETE));
    }};
    callBackMap[IPC_RACKMEMSHM_MMAP] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_MMAP, req, rsp, execMap.at(IPC_RACKMEMSHM_MMAP));
    }};
    callBackMap[IPC_RACKMEMSHM_UNMMAP] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_UNMMAP, req, rsp, execMap.at(IPC_RACKMEMSHM_UNMMAP));
    }};
    callBackMap[IPC_CHECK_SHARE_MEMORY] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_CHECK_SHARE_MEMORY, req, rsp, execMap.at(IPC_CHECK_SHARE_MEMORY));
    }};
    callBackMap[IPC_REGION_LOOKUP_REGION_LIST] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_REGION_LOOKUP_REGION_LIST, req, rsp,
                                execMap.at(IPC_REGION_LOOKUP_REGION_LIST));
    }};
    callBackMap[IPC_REGION_CREATE_REGION] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_REGION_CREATE_REGION, req, rsp, execMap.at(IPC_REGION_CREATE_REGION));
    }};
    callBackMap[IPC_REGION_LOOKUP_REGION] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_REGION_LOOKUP_REGION, req, rsp, execMap.at(IPC_REGION_LOOKUP_REGION));
    }};
    callBackMap[IPC_REGION_DESTROY_REGION] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_REGION_DESTROY_REGION, req, rsp, execMap.at(IPC_REGION_DESTROY_REGION));
    }};
    callBackMap[IPC_RACKMEMSHM_WRITELOCK] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_WRITELOCK, req, rsp, execMap.at(IPC_RACKMEMSHM_WRITELOCK));
    }};
    callBackMap[IPC_RACKMEMSHM_READLOCK] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_READLOCK, req, rsp, execMap.at(IPC_RACKMEMSHM_READLOCK));
    }};
    callBackMap[IPC_RACKMEMSHM_UNLOCK] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_UNLOCK, req, rsp, execMap.at(IPC_RACKMEMSHM_UNLOCK));
    }};
    callBackMap[IPC_RACKMEMSHM_QUERY_SLOT_ID] = {[](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(udsInfo, IPC_RACKMEMSHM_QUERY_SLOT_ID, req, rsp,
                                execMap.at(IPC_RACKMEMSHM_QUERY_SLOT_ID));
    }};
    GetIpcCallbackTablePartition();
    return callBackMap;
}

uint32_t IpcServer::InitializeIpcCallbackTable()
{
    auto map = IpcServer::GetInstance().GetIpcCallbackTable();
    for (auto &op : map) {
        MxmComEndpoint endpoint;
        endpoint.address = "mem_share";
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
    execMap.emplace(IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, MxmServerMsgHandle::ShmLookRegionList);
    execMap.emplace(IPC_RACKMEMSHM_CREATE, MxmServerMsgHandle::ShmCreate);
    execMap.emplace(IPC_RACKMEMSHM_CREATE_WITH_PROVIDER, MxmServerMsgHandle::ShmCreateWithProvider);
    execMap.emplace(IPC_RACKMEMSHM_DELETE, MxmServerMsgHandle::ShmDelete);
    execMap.emplace(IPC_RACKMEMSHM_MMAP, MxmServerMsgHandle::ShmMap);
    execMap.emplace(IPC_RACKMEMSHM_UNMMAP, MxmServerMsgHandle::ShmUnmap);
    execMap.emplace(IPC_CHECK_SHARE_MEMORY, MxmServerMsgHandle::AppCheckShareMemoryMap);
    execMap.emplace(IPC_REGION_LOOKUP_REGION_LIST, MxmServerMsgHandle::RegionLookupRegionList);
    execMap.emplace(IPC_REGION_CREATE_REGION, MxmServerMsgHandle::RegionCreateRegion);
    execMap.emplace(IPC_REGION_LOOKUP_REGION, MxmServerMsgHandle::RegionLookupRegion);
    execMap.emplace(IPC_REGION_DESTROY_REGION, MxmServerMsgHandle::RegionDestroyRegion);
    execMap.emplace(IPC_RACKMEMSHM_WRITELOCK, MxmServerMsgHandle::ShmWriteLock);
    execMap.emplace(IPC_RACKMEMSHM_READLOCK, MxmServerMsgHandle::ShmReadLock);
    execMap.emplace(IPC_RACKMEMSHM_UNLOCK, MxmServerMsgHandle::ShmUnLock);
    execMap.emplace(IPC_RACKMEMSHM_ATTACH, MxmServerMsgHandle::ShmAttach);
    execMap.emplace(IPC_RACKMEMSHM_DETACH, MxmServerMsgHandle::ShmDetach);
    execMap.emplace(IPC_RACKMEMSHM_LOOKUP_LIST, MxmServerMsgHandle::ShmListLookup);
    execMap.emplace(IPC_RACKMEMSHM_LOOKUP, MxmServerMsgHandle::ShmLookup);
    execMap.emplace(RPC_AGENT_QUERY_NODE_INFO, MxmServerMsgHandle::RpcQueryNodeInfo);
    execMap.emplace(IPC_RACKMEMSHM_QUERY_NODE, MxmServerMsgHandle::ShmQueryNode);
    execMap.emplace(IPC_RACKMEMSHM_QUERY_DLOCK_STATUS, MxmServerMsgHandle::ShmQueryDlockStatus);
    execMap.emplace(IPC_RACKMEMSHM_QUERY_SLOT_ID, MxmServerMsgHandle::LookupLocalSlotId);
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

    DBG_LOGINFO("Mxmshm hpc Server ipc is initialized");
    return HOK;
}

uint32_t IpcServer::Stop()
{
    return HOK;
}
} // namespace ock::share::service