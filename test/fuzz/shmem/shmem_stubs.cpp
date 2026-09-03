/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "shmem_stubs.h"
#include "RmLibObmmExecutor.h"

namespace FUZZ {
static size_t SIZE = 1024 * 1024 * 1024;
static MxmComUdsInfo g_udsInfo = {12345, 0, 0};

int32_t LookupRegionListFuzzStub(SHMRegions &regions)
{
    regions.num = 1;

    regions.region[0].num = 2;
    regions.region[0].type = ALL2ALL_SHARE;

    char safeHostName[MAX_HOST_NAME_DESC_LENGTH] = {};
    char safeNodeId[MAX_HOST_NAME_DESC_LENGTH] = {};

    std::string hostName = "1";
    std::string nodeId = "1";

    strncpy_s(safeHostName, MAX_HOST_NAME_DESC_LENGTH, hostName.c_str(),
              strnlen(hostName.c_str(), MAX_HOST_NAME_DESC_LENGTH));
    strncpy_s(safeNodeId, MAX_HOST_NAME_DESC_LENGTH, nodeId.c_str(),
              strnlen(nodeId.c_str(), MAX_HOST_NAME_DESC_LENGTH));
    strncpy_s(regions.region[0].hostName[0], MAX_HOST_NAME_DESC_LENGTH, safeHostName,
              strnlen(safeHostName, MAX_HOST_NAME_DESC_LENGTH));
    strncpy_s(regions.region[0].hostName[1], MAX_HOST_NAME_DESC_LENGTH, safeHostName,
              strnlen(safeHostName, MAX_HOST_NAME_DESC_LENGTH));
    strncpy_s(regions.region[0].nodeId[0], MAX_HOST_NAME_DESC_LENGTH, safeNodeId,
              strnlen(safeNodeId, MAX_HOST_NAME_DESC_LENGTH));
    strncpy_s(regions.region[0].nodeId[1], MAX_HOST_NAME_DESC_LENGTH, safeNodeId,
              strnlen(safeNodeId, MAX_HOST_NAME_DESC_LENGTH));

    return 0;
}

int ClientIpcLeaseMallocMemStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_MALLOC_MEMORY\n");
    ock::lease::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_MALLOC_MEMORY, request, response,
                                                            ock::lease::service::MxmServerMsgHandle::AppMallocMemory);
    return 0;
}

int ClientIpcLeaseFreeMemStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_FREE_RACKMEM\n");
    ock::lease::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_FREE_RACKMEM, request, response,
                                                            ock::lease::service::MxmServerMsgHandle::AppFreeMemory);
    return 0;
}

int ClientIpcShmLookRegionListStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, request,
                                                            response,
                                                            ock::share::service::MxmServerMsgHandle::ShmLookRegionList);
    return 0;
}

int ClientIpcShmCreateStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_CREATE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_CREATE, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmCreate);
    return 0;
}

int ClientIpcShmDeleteStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_DELETE\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_DELETE, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmDelete);
    return 0;
}

int ClientIpcShmMmapStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_MMAP\n");
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
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_UNMMAP\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_UNMMAP, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmUnmap);
    return 0;
}

int ClientIpcGetShmMemFaultStatusStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS, request, response,
        ock::share::service::MxmServerMsgHandle::ShmQueryMemFaultStatus);
    return 0;
}

int ClientIpcLookupRegionListStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_REGION_LOOKUP_REGION_LIST\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_LOOKUP_REGION_LIST, request, response,
        ock::share::service::MxmServerMsgHandle::RegionLookupRegionList);
    return 0;
}

int ClientIpcLookupRegionStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_REGION_LOOKUP_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_LOOKUP_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionLookupRegion);
    return 0;
}

int ClientIpcCreateRegionStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_REGION_CREATE_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_CREATE_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionCreateRegion);
    return 0;
}

int ClientIpcDestroyRegionStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_REGION_DESTROY_REGION\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(
        g_udsInfo, IPC_REGION_DESTROY_REGION, request, response,
        ock::share::service::MxmServerMsgHandle::RegionDestroyRegion);
    return 0;
}

int ClientIpcShmListLookupStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_LOOKUP_LIST\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP_LIST, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmListLookup);
    return 0;
}

int ClientIpcShmLookupStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_LOOKUP\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_LOOKUP, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmLookup);
    return 0;
}

int ClientIpcShmAttachStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_ATTACH\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_ATTACH, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmAttach);
    return 0;
}

int ClientIpcShmDetachStub(MsgBase *request, MsgBase *response)
{
    printf("MxmComIpcClientSendStub: receive IPC_RACKMEMSHM_DETACH\n");
    ock::share::service::IpcServer::GetInstance().MsgHandle(g_udsInfo, IPC_RACKMEMSHM_DETACH, request, response,
                                                            ock::share::service::MxmServerMsgHandle::ShmDetach);
    return 0;
}

void MxmComStopIpcClientStub()
{
    printf("MxmComStopIpcClientStub\n");
    return;
}

int MxmComIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response)
{
    switch (opCode) {
        case IPC_MALLOC_MEMORY:
            return ClientIpcLeaseMallocMemStub(request, response);
        case IPC_FREE_RACKMEM:
            return ClientIpcLeaseFreeMemStub(request, response);
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
        case IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS:
            return ClientIpcGetShmMemFaultStatusStub(request, response);
        case IPC_RACKMEMSHM_WRITELOCK:
            return ClientIpcShmWriteStub(request, response);
        case IPC_RACKMEMSHM_READLOCK:
            return ClientIpcShmReadStub(request, response);
        case IPC_RACKMEMSHM_UNLOCK:
            return ClientIpcShmUnLockStub(request, response);
        case IPC_REGION_LOOKUP_REGION_LIST:
            return ClientIpcLookupRegionListStub(request, response);
        case IPC_REGION_LOOKUP_REGION:
            return ClientIpcLookupRegionStub(request, response);
        case IPC_REGION_CREATE_REGION:
            return ClientIpcCreateRegionStub(request, response);
        case IPC_REGION_DESTROY_REGION:
            return ClientIpcDestroyRegionStub(request, response);
        case IPC_RACKMEMSHM_LOOKUP_LIST:
            return ClientIpcShmListLookupStub(request, response);
        case IPC_RACKMEMSHM_LOOKUP:
            return ClientIpcShmLookupStub(request, response);
        case IPC_RACKMEMSHM_ATTACH:
            return ClientIpcShmAttachStub(request, response);
        case IPC_RACKMEMSHM_DETACH:
            return ClientIpcShmDetachStub(request, response);
        default:
            printf("received unexpected opCode:%u\n", opCode);
            return -1;
    }
    return 0;
}

int RegionLookupRegionStub(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (rsp == nullptr) {
        printf("RegionLookupRegionStub: rsp is nullptr\n");
        return MXM_ERR_NULLPTR;
    }
    auto response = dynamic_cast<ShmLookupRegionResponse *>(rsp);
    SHMRegionDesc region;
    region.num = 1;
    response->errCode_ = 0;
    response->region_ = region;
    return 0;
}

int ShmImportStub(const ImportShmParam &param, ImportShmResult &result)
{
    result.shmSize = SIZE;
    result.unitSize = SIZE;
    result.memIds.push_back(1);
    return 0;
}

int ShmImportStub2(const ImportShmParam &param, ImportShmResult &result)
{
    result.shmSize = SIZE;
    result.unitSize = SIZE;
    result.memIds.push_back(1);
    result.flag = 1;
    return 0;
}

int LeaseMallocStub(const LeaseMallocParam &param, ock::mxm::LeaseMallocResult &result)
{
    if (param.isNuma) {
        result.numaId = 1;
        result.memIds.push_back(0);
    } else {
        result.memIds.push_back(1);
        result.numaId = -1;
        result.unitSize = SIZE;
    }
    return 0;
}

int ShmCreateStub(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (rsp == nullptr) {
        printf("ShmCreateStub: rsp is nullptr\n");
        return MXM_ERR_NULLPTR;
    }
    auto response = dynamic_cast<CommonResponse *>(rsp);
    response->errCode_ = 0;
    return 0;
}

int FilterAllHostsInAttrStub(SHMRegions *list, const ubsmem_region_attributes_t *reg_attr, int &index)
{
    index = 0;
    return UBSM_OK;
}
} // namespace FUZZ