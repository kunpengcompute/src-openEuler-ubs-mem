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
#include "shm_ipc_command.h"
#include <sys/mman.h>
#include <memory>
#include "ipc_proxy.h"
#include "message_op.h"
#include "rack_mem_functions.h"
#include "rack_mem_lib_common.h"

#include <ShmMetaDataMgr.h>

namespace ock::mxmd {
using namespace ock::common;

uint32_t ShmIpcCommand::IpcCallShmCreate(const std::string &regionName, const std::string &name, uint64_t size,
                                         const std::string &nid, const SHMRegionDesc &shmRegionDesc, uint64_t flags,
                                         mode_t mode)
{
    DBG_LOGINFO("Create share memory start to construct the ipc message, name=" << name);
    auto request = std::make_shared<ShmCreateRequest>(regionName, name, size, nid, shmRegionDesc, flags, mode);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    DBG_LOGINFO("BaseNid before send ipc: " << nid);
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_CREATE);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcCallShmCreate, name=" << name);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCallShmCreate response error=" << errCode << " name=" << name);
        return errCode;
    }
    DBG_LOGDEBUG("Create share memory receive message from agent successfully name=" << name);
    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmCreateWithProvider(const std::string &hostName, uint32_t socketId, uint32_t numaId,
                                                     uint32_t portId, const std::string &name, uint64_t size,
                                                     uint64_t flags, mode_t mode)
{
    DBG_LOGINFO("Create shm with provider start to construct the ipc message, name=" << name);
    auto request = std::shared_ptr<ShmCreateWithProviderRequest>(
        new (std::nothrow) ShmCreateWithProviderRequest(hostName, socketId, numaId, portId, name, size, flags, mode));
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::shared_ptr<CommonResponse>(new (std::nothrow) CommonResponse());
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_CREATE_WITH_PROVIDER);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Get exception when IpcCallShmCreateWithProvider, name=" << name << ", ret=" << ret);
        return ret;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCallShmCreateWithProvider response error=" << errCode << ", name=" << name);
        return errCode;
    }
    DBG_LOGDEBUG("Create shm with provider receive message from agent successfully name=" << name);
    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmDelete(const std::string &name)
{
    auto request = std::make_shared<ShmDeleteRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_DELETE);
    if (BresultFail(hr)) {
        DBG_LOGERROR("get exception when IpcProxy::GetInstance().Ipc IpcCallShmDelete " << name);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCall ShmDelete response error : " << errCode << " name " << name);
        return errCode;
    }
    DBG_LOGINFO("app receive delete share memory message from agent successfully " << name);
    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmLookRegionList(const std::string &baseNid, ShmRegionType type, SHMRegions &list)
{
    DBG_LOGINFO("Looking up region list, region type=" << type);
    auto request = std::make_shared<ShmLookRegionListRequest>(baseNid, static_cast<int32_t>(type));
    if (request == nullptr) {
        DBG_LOGERROR("Request is empty.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmLookRegionListResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is empty.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when calling IpcCallShmLookRegionList, region type=" << type);
        return hr;
    }

    hr = response->errCode_;
    if (BresultFail(hr)) {
        DBG_LOGERROR("Failed to look up regions list, error=" << hr << ", region type==" << type);
        return hr;
    }
    const SHMRegions &shmRegions = response->regions_;
    if (shmRegions.num <= 0 || shmRegions.num > MAX_REGIONS_NUM) {
        DBG_LOGERROR("The shmRegion value error shmRegions->num is :" << shmRegions.num);
        return MXM_ERR_CHECK_RESOURCE;
    }
    DBG_LOGINFO("Result of Ipc looking up regions list, number=" << shmRegions.num);
    list.num = shmRegions.num;
    for (int i = 0; i < list.num; ++i) {
        list.region[i] = shmRegions.region[i];
        DBG_LOGINFO("Serial number=" << i << ", number=" << shmRegions.region[i].num);
        for (int j = 0; j < shmRegions.num; ++j) {
            DBG_LOGDEBUG("Node id=" << shmRegions.region[i].nodeId[j]
                                    << ", host name=" << shmRegions.region[i].hostName[j]);
        }
    }
    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmMap(const std::string &name, uint64_t mapSize, std::vector<uint64_t> &memIds,
                                      size_t &shmSize, size_t &unitSize, uint64_t &flag, int &oflag, mode_t mode,
                                      int prot)
{
    auto request = std::make_shared<ShmMapRequest>("default", name, mapSize, mode, prot);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<ShmMapResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_MMAP);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcCallShmMap " << name);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCallShmMap response error : " << errCode);
        return errCode;
    }
    if (response->memIds_.empty()) {
        DBG_LOGERROR("memid error " << MemToStr(memIds));
        return MXM_ERR_CHECK_RESOURCE;
    }
    if (response->shmSize_ <= 0) {
        DBG_LOGERROR("ShmSize error " << response->shmSize_);
        return MXM_ERR_CHECK_RESOURCE;
    }
    if (response->unitSize_ <= 0) {
        DBG_LOGERROR("UnitSize error " << response->unitSize_);
        return MXM_ERR_CHECK_RESOURCE;
    }
    memIds.assign(response->memIds_.begin(), response->memIds_.end());
    shmSize = response->shmSize_;
    unitSize = response->unitSize_;
    flag = response->flag_;
    oflag = response->oflag_;
    if (oflag != O_RDONLY && oflag != O_RDWR) {
        DBG_LOGERROR("Invalid oflag value=" << oflag);
        return MXM_ERR_CHECK_RESOURCE;
    }
    return MXM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmUnMap(const std::string &name)
{
    auto request = std::make_shared<ShmUnmapRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_UNMMAP);
    if (BresultFail(hr)) {
        DBG_LOGERROR("get exception when IpcProxy::GetInstance().Ipc IpcCallShmUnMap " << name);
        return hr;
    }

    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("IpcCallShmUnMap response error : " << errCode << " name: " << name);
        return errCode;
    }

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallLookupRegionList(const std::string &baseNid, ShmRegionType type, SHMRegions &list)
{
    auto request = std::make_shared<ShmLookRegionListRequest>(baseNid, static_cast<int32_t>(type));
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<ShmLookRegionListResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_REGION_LOOKUP_REGION_LIST);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when calling IpcCallLookupRegionList, ret=" << hr);
        return hr;
    }

    hr = response->errCode_;
    if (BresultFail(hr)) {
        DBG_LOGERROR("IpcCallLookupRegionList, peer response error=" << hr << ", base node id=" << baseNid);
        return hr;
    }
    const SHMRegions &shmRegions = response->regions_;
    if (shmRegions.num <= 0 || shmRegions.num > MAX_REGIONS_NUM) {
        DBG_LOGERROR("Value of shared memory is error, number of regions=" << shmRegions.num);
        return MXM_ERR_CHECK_RESOURCE;
    }
    list.num = shmRegions.num;
    DBG_LOGINFO("Ipc of Looking up regions is success, number of regions=" << shmRegions.num);
    for (int i = 0; i < list.num; ++i) {
        list.region[i] = shmRegions.region[i];
    }

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallCreateRegion(const std::string &name, const SHMRegionDesc &region)
{
    DBG_LOGINFO("IpcCallCreateRegion name=" << name);
    auto request = std::make_shared<ShmCreateRegionRequest>(name, region);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<ShmCreateRegionResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_REGION_CREATE_REGION);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcCallCreateRegion, ret=" << hr);
        return hr;
    }

    hr = response->errCode_;
    if (BresultFail(hr)) {
        DBG_LOGERROR("IpcCallCreateRegion response, peer error=" << hr << ", name=" << name);
        switch (hr) {
            case MXM_ERR_REGION_EXIST:
                return MXM_ERR_REGION_EXIST;
            default:
                return hr;
        }
    }

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallLookupRegion(const std::string &name, SHMRegionDesc &region)
{
    auto request = std::make_shared<ShmLookupRegionRequest>(name);
    if (request == nullptr) {
        DBG_LOGERROR("Request is empty.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<ShmLookupRegionResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is empty.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_REGION_LOOKUP_REGION);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when calling IpcCallLookupRegion, name=" << name);
        return hr;
    }

    hr = response->errCode_;
    if (BresultFail(hr)) {
        DBG_LOGERROR("IpcCallLookupRegion response, error=" << hr);
        switch (hr) {
            case MXM_ERR_REGION_NOT_EXIST:
                return MXM_ERR_REGION_NOT_EXIST;
            default:
                return hr;
        }
    }
    region = response->region_;
    DBG_LOGINFO("Get region info successfully, region name=" << name << ", number=" << region.num);

    if (region.num > MEM_TOPOLOGY_MAX_HOSTS) {
        DBG_LOGERROR("Region num=" << region.num << " exceeds maximum=" << MEM_TOPOLOGY_MAX_HOSTS);
        return MXM_ERR_PARAM_INVALID;
    }

    for (int i = 0; i < region.num; ++i) {
        DBG_LOGDEBUG("Node id=" << region.nodeId[i] << ", host name=" << region.hostName[i]
                                << ", affinity=" << region.affinity[i]);
    }

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallDestroyRegion(const std::string &name)
{
    DBG_LOGINFO("IpcCallDestroyRegion, name=" << name);
    auto request = std::make_shared<ShmDestroyRegionRequest>(name);
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto response = std::make_shared<ShmDestroyRegionResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_REGION_DESTROY_REGION);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when calling IpcCallDestroyRegion, ret=" << hr);
        return hr;
    }

    hr = response->errCode_;
    if (BresultFail(hr)) {
        DBG_LOGERROR("response is error, peer error code=" << hr);
        return hr;
    }

    return UBSM_OK;
}

int32_t ShmIpcCommand::IpcShmemWriteLock(const std::string &name)
{
    auto request = std::make_shared<ShmWriteLockRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmWriteLockResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc Shmem WriteLock name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_WRITELOCK);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcShmemWriteLock " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcShmemWriteLock response error : " << errCode);
        return errCode;
    }
    DBG_LOGINFO("Ipc Shmem WriteLock success, name=" << name);

    return UBSM_OK;
}

int32_t ShmIpcCommand::IpcShmemReadLock(const std::string &name)
{
    auto request = std::make_shared<ShmReadLockRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmReadLockResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc Shmem ReadLock name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_READLOCK);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcShmemReadLock " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcShmemReadLock response error : " << errCode);
        return errCode;
    }
    DBG_LOGINFO("Ipc Shmem ReadLock success, name=" << name);

    return UBSM_OK;
}

int32_t ShmIpcCommand::IpcShmemUnLock(const std::string &name)
{
    auto request = std::make_shared<ShmUnLockRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmUnLockResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc Shmem UnLock name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_UNLOCK);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcShmemUnLock " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcShmemUnLock response error : " << errCode);
        return errCode;
    }
    DBG_LOGINFO("Ipc Shmem UnLock success, name=" << name);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallRpcQuery(std::string &name)
{
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("Request is nullptr.");
        return MXM_ERR_NULLPTR;
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("Response is nullptr.");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("Ipc RpcQuery, name=" << name);
    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), RPC_AGENT_QUERY_NODE_INFO);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc RackMemLookupClusterStatistic, res is " << hr);
        return MXM_ERR_IPC_SYNC_CALL;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("RackMemLookupClusterStatistic response error : " << errCode);
        return errCode;
    }
    name = response->name_;
    DBG_LOGINFO("Ipc RpcQuery success, name=" << name);

    return HOK;
}

uint32_t ShmIpcCommand::IpcCallShmAttach(const std::string &name)
{
    auto request = std::make_shared<ShmAttachRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmAttachResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc ShmAttach, name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_ATTACH);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcCallShmAttach " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcCallShmAttach response error : " << errCode);
        return errCode;
    }
    DBG_LOGINFO("Ipc ShmAttach success, name=" << name);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmDetach(const std::string &name)
{
    auto request = std::make_shared<ShmDetachRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc ShmDetach, name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_DETACH);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcCallShmDetach " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcCallShmDetach response error=" << errCode);
        return errCode;
    }
    DBG_LOGINFO("Ipc ShmDetach success, name=" << name);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &shm_list)
{
    auto request = std::make_shared<ShmListLookupRequest>("default", prefix);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmListLookupResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc ShmListLookup, name prefix=" << prefix);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_LOOKUP_LIST);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcCallShmListLookup " << prefix);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcCallShmListLookup response error=" << errCode);
        return errCode;
    }

    shm_list.clear();
    shm_list.assign(response->shmNames_.begin(), response->shmNames_.end());
    DBG_LOGINFO("Ipc ShmListLookup success, name prefix=" << prefix);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcCallShmLookup(const std::string &name, ubsmem_shmem_info_t &shm_info)
{
    auto request = std::make_shared<ShmLookupRequest>("default", name);
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<ShmLookupResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    DBG_LOGINFO("Ipc ShmLookup, name=" << name);
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_LOOKUP);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcCallShmLookup " << name);
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcCallShmLookup response error=" << errCode);
        return errCode;
    }

    ret = memcpy_s(&shm_info, sizeof(ubsmem_shmem_info_t), &response->shmInfo_, sizeof(ubsmem_shmem_info_t));
    if (ret != EOK) {
        DBG_LOGERROR("shm_info copy failed=" << ret);
        return ret;
    }
    DBG_LOGINFO("Ipc ShmLookup success, name=" << name);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcQueryNode(std::string &NodeId, bool &isNodeReady, bool isQueryMasterNode)
{
    auto request = std::make_shared<QueryNodeRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<QueryNodeResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    request->queryMasterNodeFlag_ = isQueryMasterNode;
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_QUERY_NODE);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcQueryNode ");
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGWARN("IpcQueryNode response warning is ZenDiscovery not ready.");
        return errCode;
    }
    DBG_LOGINFO("Ipc QueryNode success, nodeId=" << NodeId << ", isNodeReady=" << isNodeReady);
    if (!response->nodeIsReady_) {
        DBG_LOGWARN("IpcQueryNode response warning is node not ready, try again later.");
        isNodeReady = false;
        return 0;
    }
    isNodeReady = true;
    NodeId = response->nodeId_;
    return UBSM_OK;
}

uint32_t ShmIpcCommand::IpcQueryDlockStatus(bool &isReady)
{
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        DBG_LOGERROR("request is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto response = std::make_shared<QueryDlockStatusResponse>();
    if (response == nullptr) {
        DBG_LOGERROR("response is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_QUERY_DLOCK_STATUS);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IpcQueryDlockStatus ");
        return ret;
    }

    int32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcQueryDlockStatus response error=" << errCode);
        return errCode;
    }
    isReady = response->isReady_;
    DBG_LOGINFO("Ipc Query UbsmLock Status success, isReady=" << isReady);

    return UBSM_OK;
}

uint32_t ShmIpcCommand::AppCheckShareMemoryResource()
{
    std::vector<std::string> names;
    auto ret = ShmMetaDataMgr::GetInstance().GetAllUsedMemoryNames(names);
    if (ret != MXM_OK) {
        DBG_LOGERROR("GetAllUsedMemoryNames returned error : " << ret);
        return MXM_ERR_MALLOC_FAIL;
    }
    std::shared_ptr<CheckShareMemoryMapRequest> request;
    std::shared_ptr<CommonResponse> response;
    try {
        request = std::make_shared<CheckShareMemoryMapRequest>(names);
        response = std::make_shared<CommonResponse>();
    } catch (std::exception &e) {
        DBG_LOGERROR("CheckMemoryLeaseRequest exception : " << e.what());
        return MXM_ERR_MALLOC_FAIL;
    } catch (...) {
        DBG_LOGERROR("CheckMemoryLeaseRequest unknown exception.");
        return MXM_ERR_MALLOC_FAIL;
    }

    uint32_t hr = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_CHECK_SHARE_MEMORY);
    if (BresultFail(hr)) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IPC_CHECK_SHARE_MEMORY, res is " << hr);
        return hr;
    }
    uint32_t errCode = response->errCode_;
    if (BresultFail(errCode)) {
        DBG_LOGERROR("AppCheckLeaseMemoryResource response error : " << errCode);
        return errCode;
    }
    return MXM_OK;
}

uint32_t ShmIpcCommand::AppGetLocalSlotId(uint32_t &slotId)
{
    std::shared_ptr<CommonRequest> request;
    std::shared_ptr<LookupSlotIdResponse> response;
    try {
        request = std::make_shared<CommonRequest>();
        response = std::make_shared<LookupSlotIdResponse>();
    } catch (std::exception &e) {
        DBG_LOGERROR("Caught exception: " << e.what());
        return MXM_ERR_MALLOC_FAIL;
    } catch (...) {
        DBG_LOGERROR("Unknown exception caught.");
        return MXM_ERR_MALLOC_FAIL;
    }
    auto ret = IpcProxy::GetInstance().Ipc(request.get(), response.get(), IPC_RACKMEMSHM_QUERY_SLOT_ID);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when IpcProxy::GetInstance().Ipc IPC_RACKMEMSHM_QUERY_SLOT_ID, res is " << ret);
        return ret;
    }
    uint32_t errCode = response->errCode_;
    if (errCode != 0) {
        DBG_LOGERROR("IpcQuerySlotId response error=" << errCode);
        return errCode;
    }
    slotId = response->slotId_;
    return UBSM_OK;
}

} // namespace ock::mxmd