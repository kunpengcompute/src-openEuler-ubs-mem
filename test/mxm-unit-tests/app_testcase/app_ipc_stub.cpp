/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#include "app_ipc_stub.h"
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "RmLibObmmExecutor.h"
#include "mls_manager.h"
#include "ubse_mem_adapter_stub.h"
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
namespace UT {

static MxmComUdsInfo g_udsInfo = {12345, 0, 0};

int ClientObmmOpenStub(mem_id memId)
{
    printf("ClientObmmOpenStub\n");
    return memId;
}

int ClientIpcLeaseMallocMemStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_MALLOC_MEMORY\n");
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::MemFdCreateWithCandidate,
               int (*)(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                       const uint32_t *slot_ids, uint32_t slot_cnt, ubs_mem_fd_desc_t *fd_desc))
        .stubs()
        .will(invoke(MemFdCreateWithCandidateStub));

    ock::lease::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_MALLOC_MEMORY, request, response,
                                                            ock::lease::service::MxmServerMsgHandle::AppMallocMemory);
    return 0;
}

int ClientIpcLeaseMallocMemWithLocationStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcLeaseMallocMemWithLocationStub: receive IPC_MALLOC_MEMORY_LOC\n");
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::MemFdCreateWithLender,
               int32_t(*)(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                          const ubs_mem_lender_t *lender, uint32_t lender_cnt, ubs_mem_fd_desc_t *fd_desc))
        .stubs()
        .will(invoke(MemFdCreateWithLenderStub));
    MOCKER_CPP(&ock::lease::service::MLSManager::PreAddUsedMem,
               int32_t(*)(const std::string &name, size_t size, const AppContext &usr, bool isNuma, uint16_t perfLevel))
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&ock::lease::service::MLSManager::FinishAddUsedMem,
               int32_t(*)(const std::string &name, int64_t numaId, size_t unitSize, uint32_t slotId,
                          const std::vector<uint64_t> &memIds))
        .stubs()
        .will(returnValue(0));

    ock::lease::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_MALLOC_MEMORY_LOC, request, response,
        ock::lease::service::MxmServerMsgHandle::AppMallocMemoryWithLoc);
    return 0;
}

int ClientIpcLeaseFreeMemStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_FREE_RACKMEM\n");
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::FdPermissionChange,
               int (*)(const std::string &name, const ock::ubsm::AppContext &appContext, mode_t mode))
        .stubs()
        .will(invoke(FdPermissionChangeStub));
    ock::lease::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_FREE_RACKMEM, request, response,
                                                            ock::lease::service::MxmServerMsgHandle::AppFreeMemory);
    return 0;
}

int ClientIpcAppCheckLeaseMemoryResourceStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcAppCheckLeaseMemoryResourceStub: receive IPC_CHECK_MEMORY_LEASE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_CHECK_MEMORY_LEASE, request, response,
        ock::lease::service::MxmServerMsgHandle::AppCheckMemoryLease);
    return 0;
}

int ClientIpcShmLookRegionListStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmLookRegionListStub: receive IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, request,
                                                            response,
                                                            ock::share::service::MxmServerMsgHandle::ShmLookRegionList);
    return 0;
}

int ClientIpcShmCreateStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmCreateStub: receive IPC_RACKMEMSHM_CREATE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_CREATE, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmCreate);
    return 0;
}

int ClientIpcShmDeleteStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmDeleteStub: receive IPC_RACKMEMSHM_DELETE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_DELETE, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmDelete);
    return 0;
}

int ClientIpcShmMmapStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmMmapStub: receive IPC_RACKMEMSHM_MMAP\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_MMAP, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmMap);
    return 0;
}

int ClientIpcShmWriteStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmWriteStub: receive IPC_RACKMEMSHM_WRITELOCK\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_WRITELOCK, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmWriteLock);
    return 0;
}

int ClientIpcShmReadStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmReadStub: receive IPC_RACKMEMSHM_READLOCK\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_READLOCK, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmReadLock);
    return 0;
}

int ClientIpcShmUnLockStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmUNLockStub: receive IPC_RACKMEMSHM_UNLOCK\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_UNLOCK, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmUnLock);
    return 0;
}

int ClientIpcShmUnmmapStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmUnmmapStub: receive IPC_RACKMEMSHM_UNMMAP\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_UNMMAP, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmUnmap);
    return 0;
}

void ClientIpcStubSetUdsInfo(uint32_t pid, uint32_t uid, uint32_t gid)
{
    g_udsInfo.pid = pid;
    g_udsInfo.uid = uid;
    g_udsInfo.gid = gid;
}

void MxmComStopIpcClientStub()
{
    printf("MxmComStopIpcClientStub\n");
    return;
}

int ClientIpcLookupClusterStatisticStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcLookupClusterStatisticStub: receive IPC_QUERY_CLUSTERINFO\n");
    ock::lease::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_QUERY_CLUSTERINFO, request, response,
        ock::lease::service::MxmServerMsgHandle::AppQueryClusterInfo);
    return 0;
}

int ClientIpcQueryLeaseRecordStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_QUERY_CACHED_MEMORY\n");
    ock::lease::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_QUERY_CACHED_MEMORY, request, response,
        ock::lease::service::MxmServerMsgHandle::AppQueryCachedMemory);
    return 0;
}

int ClientIpcForceFreeCachedMemoryStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_FORCE_FREE_CACHED_MEMORY\n");
    ock::lease::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_FORCE_FREE_CACHED_MEMORY, request, response,
        ock::lease::service::MxmServerMsgHandle::AppForceFreeCachedMemory);
    return 0;
}

int ClientIpcShmQueryNodeStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_QUERY_NODE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_QUERY_NODE, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmQueryNode);
    auto rsp = dynamic_cast<QueryNodeResponse *>(response);
    if (rsp == nullptr) {
        return -1;
    }
    rsp->errCode_ = 0;
    return 0;
}

int ClientIpcLookupRegionListStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcLookupRegionListStub: receive IPC_REGION_LOOKUP_REGION_LIST\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_LOOKUP_REGION_LIST, request, response,
        ock::share::service::MxmServerMsgHandle::RegionLookupRegionList);
    return 0;
}

int ClientIpcLookupRegionStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcLookupRegionStub: receive IPC_REGION_LOOKUP_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_LOOKUP_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionLookupRegion);
    return 0;
}

int ClientIpcDestroyRegionStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcDestroyRegionStub: receive IPC_REGION_DESTROY_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_DESTROY_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionDestroyRegion);
    return 0;
}

int ClientIpcShmAttachStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmAttachStub: receive IPC_RACKMEMSHM_ATTACH\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_ATTACH, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmAttach);
    return 0;
}

int ClientRpcQueryNodeStub(MsgBase *request, MsgBase *response)
{
    printf("ClientRpcQueryNodeStub: receive RPC_AGENT_QUERY_NODE_INFO\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, RPC_AGENT_QUERY_NODE_INFO, request, response,
                                                            ock::share::service::MxmServerMsgHandle::RpcQueryNodeInfo);
    auto rsp = dynamic_cast<RpcQueryInfoResponse *>(response);
    rsp->errCode_ = 0;
    return 0;
}

int ClientIpcShmDetachStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmDetachStub: receive IPC_RACKMEMSHM_DETACH\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_DETACH, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmDetach);
    return 0;
}

int ClientIpcQueryDlockStatusStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcQueryDlockStatusStub: receive IPC_RACKMEMSHM_QUERY_DLOCK_STATUS\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_RACKMEMSHM_QUERY_DLOCK_STATUS, request, response,
        ock::share::service::MxmServerMsgHandle::ShmQueryDlockStatus);
    return 0;
}

int ClientIpcCreateRegionStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcCreateRegionStub: receive IPC_REGION_CREATE_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_CREATE_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionCreateRegion);
    return 0;
}

int ClientIpcShmListLookupStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmListLookupStub: receive IPC_RACKMEMSHM_LOOKUP_LIST\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP_LIST, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmListLookup);
    return 0;
}

int ClientIpcShmLookupStub(MsgBase *request, MsgBase *response)
{
    printf("ClientIpcShmLookupStub: receive IPC_RACKMEMSHM_LOOKUP\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmLookup);
    return 0;
}

int MxmComIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response)
{
    switch (opCode) {
        case IPC_MALLOC_MEMORY:
            return ClientIpcLeaseMallocMemStub(request, response);
        case IPC_FREE_RACKMEM:
            return ClientIpcLeaseFreeMemStub(request, response);
        case IPC_QUERY_CLUSTERINFO:
            return ClientIpcLookupClusterStatisticStub(request, response);
        case IPC_QUERY_CACHED_MEMORY:
            return ClientIpcQueryLeaseRecordStub(request, response);
        case IPC_FORCE_FREE_CACHED_MEMORY:
            return ClientIpcForceFreeCachedMemoryStub(request, response);
        case IPC_RACKMEMSHM_QUERY_DLOCK_STATUS:
            return ClientIpcQueryDlockStatusStub(request, response);
        case IPC_MALLOC_MEMORY_LOC:
            return ClientIpcLeaseMallocMemWithLocationStub(request, response);
        case IPC_CHECK_MEMORY_LEASE:
            return ClientIpcAppCheckLeaseMemoryResourceStub(request, response);
        default:
            printf("MxmComIpcClientSendStub received unexpected opCode:%u\n", opCode);
            return -1;
    }
    return 0;
}

int MxmShmIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response)
{
    switch (opCode) {
        case IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS:
            return ClientIpcShmLookRegionListStub(request, response);
        case IPC_RACKMEMSHM_CREATE:
            return ClientIpcShmCreateStub(request, response);
        case IPC_RACKMEMSHM_DELETE:
            return ClientIpcShmDeleteStub(request, response);
        case IPC_RACKMEMSHM_MMAP:
            return ClientIpcShmMmapStub(request, response);
        case IPC_RACKMEMSHM_UNMMAP:
            return ClientIpcShmUnmmapStub(request, response);
        case IPC_RACKMEMSHM_WRITELOCK:
            return ClientIpcShmWriteStub(request, response);
        case IPC_RACKMEMSHM_READLOCK:
            return ClientIpcShmReadStub(request, response);
        case IPC_RACKMEMSHM_UNLOCK:
            return ClientIpcShmUnLockStub(request, response);
        case IPC_RACKMEMSHM_QUERY_NODE:
            return ClientIpcShmQueryNodeStub(request, response);
        case IPC_REGION_LOOKUP_REGION_LIST:
            return ClientIpcLookupRegionListStub(request, response);
        case IPC_REGION_LOOKUP_REGION:
            return ClientIpcLookupRegionStub(request, response);
        case IPC_REGION_DESTROY_REGION:
            return ClientIpcDestroyRegionStub(request, response);
        case IPC_RACKMEMSHM_ATTACH:
            return ClientIpcShmAttachStub(request, response);
        case IPC_RACKMEMSHM_DETACH:
            return ClientIpcShmDetachStub(request, response);
        case IPC_REGION_CREATE_REGION:
            return ClientIpcCreateRegionStub(request, response);
        case IPC_RACKMEMSHM_LOOKUP_LIST:
            return ClientIpcShmListLookupStub(request, response);
        case IPC_RACKMEMSHM_LOOKUP:
            return ClientIpcShmLookupStub(request, response);
        case RPC_AGENT_QUERY_NODE_INFO:
            return ClientRpcQueryNodeStub(request, response);
        default:
            printf("MxmShmIpcClientSendStub received unexpected opCode:%u\n", opCode);
            return -1;
    }
    return 0;
}

} // namespace UT