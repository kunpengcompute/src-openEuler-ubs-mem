/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "ipc_command.h"
#include <sys/mman.h>
#include <memory>
#include "ubs_mem_def.h"
#include "BorrowAppMetaMgr.h"
#include "RackMem.h"
#include "ipc_proxy.h"
#include "message_op.h"
#include "rack_mem_lib_common.h"

namespace ock::mxmd {
using namespace ock::common;
uint32_t IpcCommand::AppMallocMemory(std::shared_ptr<AppMallocMemoryRequest> &request,
                                     std::shared_ptr<AppMallocMemoryResponse> &response)
{
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_MALLOC_MEMORY);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc BorrowMemory, ret" << hr);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("BorrowMemory response error : " << errCode);
        return errCode;
    }
    return UBSM_OK;
}

uint32_t IpcCommand::AppMallocMemoryWithLoc(std::shared_ptr<AppMallocMemoryWithLocRequest> &request,
                                            std::shared_ptr<AppMallocMemoryResponse> &response)
{
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_MALLOC_MEMORY_LOC);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy IPC_MALLOC_MEMORY_LOC, ret=" << hr);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("BorrowMemory response error=" << errCode);
        return errCode;
    }
    return UBSM_OK;
}

uint32_t IpcCommand::AppFreeMemory(const std::string &regionName, const std::string &name,
                                   const std::vector<mem_id> &memIds)
{
    if (memIds.empty()) {
        DBG_LOGERROR("memids is empty");
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("AppFreeMemory start. regionName=" << regionName << " name=" << name << " memIdNum=" << memIds.size());
    auto request = std::make_shared<AppFreeMemoryRequest>(memIds, regionName, name);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_FREE_RACKMEM);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IPC_FREE_RACKMEM " << name);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCallUnborrow response error : " << errCode);
        return errCode;
    }
    DBG_LOGINFO("AppFreeMemory successfully.");

    return UBSM_OK;
}

uint32_t IpcCommand::IpcRackMemLookupClusterStatistic(ubsmem_cluster_info_t *clusterInfo)
{
    DBG_LOGINFO("IpcRackMemLookupClusterStatistic start.");
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<AppQueryClusterInfoResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_QUERY_CLUSTERINFO);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc RackMemLookupClusterStatistic, res is " << hr);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("RackMemLookupClusterStatistic response error : " << errCode);
        return errCode;
    }
    auto ret = memcpy_s(clusterInfo, sizeof(ubsmem_cluster_info_t), &response->info_, sizeof(response->info_));
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed when restoring userinfo, ret:" << ret);
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("IpcRackMemLookupClusterStatistic end. clusterInfo.num " << clusterInfo->host_num);
    return UBSM_OK;
}

uint32_t IpcCommand::AppQueryLeaseRecord(std::vector<std::string> &recordInfo)
{
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<AppQueryCachedMemoryResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_QUERY_CACHED_MEMORY);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc AppQueryLeaseRecord, res is " << hr);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("AppQueryLeaseRecord response error : " << errCode);
        return errCode;
    }
    recordInfo = response->records_;
    return HOK;
}

uint32_t IpcCommand::AppForceFreeCachedMemory()
{
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_FORCE_FREE_CACHED_MEMORY);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc AppForceFreeCachedMemory, res is " << hr);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("RackMemLookupClusterStatistic response error : " << errCode);
        return errCode;
    }
    return HOK;
}

uint32_t IpcCommand::AppCheckLeaseMemoryResource()
{
    std::vector<std::string> names;
    auto ret = BorrowAppMetaMgr::GetInstance().GetAllUsedMemoryNames(names);
    if (ret != MXM_OK) {
        DBG_LOGERROR("GetAllUsedMemoryNames returned error : " << ret);
        return MXM_ERR_MALLOC_FAIL;
    }
    std::shared_ptr<CheckMemoryLeaseRequest> request;
    std::shared_ptr<CommonResponse> response;
    try {
        request = std::make_shared<CheckMemoryLeaseRequest>(names);
        response = std::make_shared<CommonResponse>();
    } catch (std::exception &e) {
        DBG_LOGERROR("CheckMemoryLeaseRequest exception : " << e.what());
        return MXM_ERR_MALLOC_FAIL;
    } catch (...) {
        DBG_LOGERROR("CheckMemoryLeaseRequest unknown exception.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_CHECK_MEMORY_LEASE);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IPC_CHECK_MEMORY_LEASE, res is " << hr);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("AppCheckLeaseMemoryResource response error : " << errCode);
        return errCode;
    }
    return MXM_OK;
}
} // namespace ock::mxmd