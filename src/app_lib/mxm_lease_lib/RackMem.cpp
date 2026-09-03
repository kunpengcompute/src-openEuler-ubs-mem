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

#include "RackMem.h"
#include <numaif.h>
#include <securec.h>
#include <sys/mman.h>
#include "ubs_mem_def.h"
#include "BorrowAppMetaMgr.h"
#include "ipc_command.h"
#include "log.h"
#include "rack_mem_fd_map.h"
#include "system_adapter.h"
#include "ubsm_ptracer.h"

namespace ock::mxmd {
using namespace ock::common;
using namespace ock::ubsm;
void *RackMem::MemoryIDUsedByNuma(AppBorrowMetaDesc &desc, const std::vector<uint64_t> &memIds, int64_t numaId,
                                  size_t unitSize, const std::string &name)
{
    if (numaId < 0) {
        DBG_LOGERROR("numaid invalid, numaid is " << numaId);
        return nullptr;
    }
    void *mappedMemory = nullptr;
    DBG_LOGINFO("Borrowing memory by numa, name=" << name << ", numa id=" << numaId << ", size=" << desc.GetFileSize()
                                                  << "mem ids");
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_NUMA_MAP);
    mappedMemory = SystemAdapter::MemoryMap(nullptr, desc.GetFileSize(), PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    TP_TRACE_END(TP_UBSM_MALLOC_NUMA_MAP, mappedMemory == nullptr ? -1L : 0L);
    if (mappedMemory == MAP_FAILED) {
        DBG_LOGERROR("mmap failed, size=" << desc.GetFileSize());
        return nullptr;
    }
    int mode = MPOL_PREFERRED;
    unsigned long nodemask[1];
    nodemask[0] = 1UL << numaId;
    unsigned long maxNode = 16;
    unsigned int flags = 0;
    errno = 0;
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_NUMA_BIND);
    long bindRet = SystemAdapter::MemoryBind(mappedMemory, desc.GetFileSize(), mode, nodemask, maxNode, flags);
    TP_TRACE_END(TP_UBSM_MALLOC_NUMA_BIND, bindRet == 0 ? 0L : 1L);
    if (bindRet == -1) {
        DBG_LOGERROR("Mbind error, error code=" << errno << ", numa id=" << numaId << ", addr=" << mappedMemory
                                                << ", size=" << desc.GetFileSize());
        SystemAdapter::MemoryUnMap(mappedMemory, desc.GetFileSize());
        return nullptr;
    }
    // 存元数据
    desc.UpdateBorrowMetaDesc(memIds, desc.GetFileSize(), true, name);
    auto ret = BorrowAppMetaMgr::GetInstance().AddMeta(mappedMemory, desc);
    DBG_LOGINFO("mem id=" << MemToStr(memIds) << ", numa id=" << numaId << ", ret=" << ret);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Add meta data error code is " << ret);
        SystemAdapter::MemoryUnMap(mappedMemory, desc.GetFileSize());
        return nullptr;
    }

    return mappedMemory;
}

void *RackMem::MemoryIDUsedByFd(AppBorrowMetaDesc &desc, const std::vector<uint64_t> &memIds, size_t unitSize,
                                const std::string &name, uint64_t flags)
{
    // 向上取整后去mmap
    uint64_t mmapCount{};
    uint64_t totalSize{};
    desc.SetUnitSize(unitSize);
    RackMemFdMap::GetInstance().GetActualMapSize(desc.GetFileSize(), unitSize, totalSize, mmapCount);

    if (mmapCount > memIds.size()) {
        DBG_LOGERROR("mmapCount=" << mmapCount << " larger than memIds size=" << memIds.size()
                                  << ", name=" << desc.GetName() << ", unitSize=" << unitSize);
        return nullptr;
    }

    void *mmapAddr = nullptr;
    off_t offset = 0;
    uint32_t ret = 0;
    if ((flags & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD) {
        offset |= OBMM_MMAP_FLAG_HUGETLB_PMD;
        TP_TRACE_BEGIN(TP_UBSM_MALLOC_FD_MAP);
        static auto alignment = GetHugeTlbPmdSize();
        ret = RackMemFdMap::MemoryMapAligned(totalSize, mmapAddr, alignment);
        TP_TRACE_END(TP_UBSM_MALLOC_FD_MAP, ret);
    } else {
        TP_TRACE_BEGIN(TP_UBSM_MALLOC_FD_MAP);
        ret = RackMemFdMap::GetInstance().FileMapByTotalSize(totalSize, mmapAddr, PROT_WRITE | PROT_READ);
        TP_TRACE_END(TP_UBSM_MALLOC_FD_MAP, ret);
    }
    if (BresultFail(ret)) {
        DBG_LOGERROR("FileMapByTotalSize failed, name is " << desc.GetName() << ", size is " << unitSize);
        return nullptr;
    }
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_FD_BIND);
    for (uint64_t i = 0; i < mmapCount; i++) {
        const int fd = ObmmOpenInternal(memIds[i], 0, O_RDWR);
        DBG_LOGINFO("It is " << i << "th mmap, mem id is " << memIds[i] << ", fd is " << fd);
        if (fd < 0) {
            DBG_LOGERROR("Obmm open error fd is " << fd);
            SystemAdapter::MemoryUnMap(mmapAddr, totalSize);
            desc.CloseAllFd();
            return nullptr;
        }
        desc.AddFd(fd);
        auto startAddr = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mmapAddr) + i * unitSize);
        ret = RackMemFdMap::GetInstance().MapForEachFd(startAddr, unitSize, fd, PROT_WRITE | PROT_READ,
                                                       MAP_SHARED | MAP_FIXED, offset);
        if (BresultFail(ret)) {
            DBG_LOGERROR("MapForEachFd failed, fd is " << fd << ", ret is " << ret);
            SystemAdapter::MemoryUnMap(mmapAddr, totalSize);
            desc.CloseAllFd();
            return nullptr;
        }
        desc.AddAddr(startAddr);
    }
    TP_TRACE_END(TP_UBSM_MALLOC_FD_BIND, 0);
    // 存元数据
    desc.UpdateBorrowMetaDesc(memIds, totalSize, false, name);
    ret = BorrowAppMetaMgr::GetInstance().AddMeta(mmapAddr, desc);
    DBG_LOGINFO("UpdateBorrowMetaDesc, name=" << name << ", ret=" << ret);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Add meta data error code is " << ret);
        SystemAdapter::MemoryUnMap(mmapAddr, totalSize);
        desc.CloseAllFd();
        return nullptr;
    }
    return mmapAddr;
}

int RackMem::UbsMemMap(size_t size, uint64_t flags, std::shared_ptr<AppMallocMemoryResponse> &response,
                       void *&mappedMemory) const
{
    bool isNuma = ((flags & UBSM_FLAG_MALLOC_WITH_NUMA) == UBSM_FLAG_MALLOC_WITH_NUMA);
    AppBorrowMetaDesc desc(size);
    if (isNuma) {
        mappedMemory = GetInstance().MemoryIDUsedByNuma(desc, response->memIds_, response->numaId_, response->unitSize_,
                                                        response->name_);
    } else {
        if (response->memIds_.empty()) {
            DBG_LOGERROR("MemIds is empty, malloc size=" << size);
            return MXM_ERR_PARAM_INVALID;
        }
        mappedMemory =
            GetInstance().MemoryIDUsedByFd(desc, response->memIds_, response->unitSize_, response->name_, flags);
    }
    if (mappedMemory == nullptr) {
        DBG_LOGERROR("Lease memory map failed, flags=" << flags);
        auto ret = IpcCommand::AppFreeMemory(desc.GetRegionName(), response->name_, response->memIds_);
        if (ret != 0) {
            DBG_LOGERROR("Failed to free memory, name=" << response->name_ << ", ret=" << ret);
        }
        return MXM_ERR_MMAP_VA_FAILED;
    }
    return MXM_OK;
}

int RackMem::UbsMemMalloc(std::string &region, size_t size, PerfLevel level, uint64_t flags, void **local_ptr) const
{
    AppBorrowMetaDesc desc(size, region);
    bool isNuma = ((flags & UBSM_FLAG_MALLOC_WITH_NUMA) == UBSM_FLAG_MALLOC_WITH_NUMA);
    auto request = std::make_shared<AppMallocMemoryRequest>(size, level, isNuma, region);
    auto response = std::make_shared<AppMallocMemoryResponse>();
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("request or response is nullptr, size=" << size << ", flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("UbsMemMalloc, region name=" << region << ", size=" << size << " flags=" << flags);
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_IPC_REQUEST);
    const uint32_t hr = IpcCommand::AppMallocMemory(request, response);
    TP_TRACE_END(TP_UBSM_MALLOC_IPC_REQUEST, hr);
    if (hr != MXM_OK) {
        DBG_LOGERROR("AppMallocMemory failed, ret=" << hr << ", region=" << region);
        return hr;
    }
    DBG_LOGINFO("AppMallocMemory success, region name=" << region << ", size=" << size << " flags=" << flags);
    void *mappedMemory = nullptr;
    auto ret = UbsMemMap(size, flags, response, mappedMemory);
    if (ret != MXM_OK) {
        DBG_LOGERROR("UbsMemMap failed, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsMemMalloc success, size=" << size << " flags=" << flags << ", name=" << response->name_);
    *local_ptr = mappedMemory;
    return MXM_OK;
}

int RackMem::UbsMemMallocWithLoc(const ubs_mem_location_t *src_loc, size_t size, uint64_t flags, void **local_ptr) const
{
    if (src_loc == nullptr || local_ptr == nullptr) {
        DBG_LOGERROR("src_loc or local_ptr is nullptr, size=" << size << ", flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }

    bool isNuma = ((flags & UBSM_FLAG_MALLOC_WITH_NUMA) == UBSM_FLAG_MALLOC_WITH_NUMA);
    auto request = std::make_shared<AppMallocMemoryWithLocRequest>(size, isNuma, src_loc->slot_id, src_loc->socket_id,
                                                                   src_loc->numa_id, src_loc->port_id);
    auto response = std::make_shared<AppMallocMemoryResponse>();
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("request or response is nullptr, size=" << size << ", flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("UbsMemMallocWithLoc, size=" << size << " flags=" << flags << ", location slotId=" << src_loc->slot_id
                                             << ", socketId=" << src_loc->socket_id << ", numaId=" << src_loc->numa_id
                                             << ", portId=" << src_loc->port_id << ", isNuma=" << isNuma);
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_LOC_IPC_REQUEST);
    auto hr = IpcCommand::AppMallocMemoryWithLoc(request, response);
    TP_TRACE_END(TP_UBSM_MALLOC_LOC_IPC_REQUEST, hr);
    if (hr != MXM_OK) {
        DBG_LOGERROR("AppMallocMemoryWithLoc failed, ret=" << hr);
        return hr;
    }
    DBG_LOGINFO("AppMallocMemoryWithLoc success, size=" << size << " flags=" << flags);
    void *mappedMemory = nullptr;
    auto ret = UbsMemMap(size, flags, response, mappedMemory);
    if (ret != MXM_OK) {
        DBG_LOGERROR("UbsMemMap failed, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsMemMallocWithLoc success, size=" << size << " flags=" << flags << ", name=" << response->name_);
    *local_ptr = mappedMemory;
    return MXM_OK;
}

uint32_t RackMem::UbsMemFree(void *ptr)
{
    if (ptr == nullptr) {
        DBG_LOGERROR("ptr is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("Start to get borrow meta.");
    AppBorrowMetaDesc metaDesc(0);
    auto getMeta = BorrowAppMetaMgr::GetInstance().GetMeta(ptr, metaDesc);
    if (BresultFail(getMeta)) {
        DBG_LOGERROR("Rack mem free invalid addr: " << ptr);
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("UbsMemFree Start, name=" << metaDesc.GetName());
    if (metaDesc.GetIsNuma() == false) {
        TP_TRACE_BEGIN(TP_UBSM_FREE_FD_UNMMAP);
        if (SystemAdapter::MemoryMap(ptr, metaDesc.GetActualMapFileSize(), PROT_NONE,
                                     MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0L) != ptr) {
            DBG_LOGWARN("Failed to lock memory, error string=" << strerror(errno));
        }
        TP_TRACE_END(TP_UBSM_FREE_FD_UNMMAP, 0UL);
        TP_TRACE_BEGIN(TP_UBSM_FREE_FD_CLOSE);
        metaDesc.CloseAllFd();
        BorrowAppMetaMgr::GetInstance().UpdateMeta(ptr, metaDesc);
        TP_TRACE_END(TP_UBSM_FREE_FD_CLOSE, 0UL);
    }

    TP_TRACE_BEGIN(TP_UBSM_FREE_IPC_REQUEST);
    auto ret2 = IpcCommand::AppFreeMemory(metaDesc.GetRegionName(), metaDesc.GetName(), metaDesc.GetMemIds());
    TP_TRACE_END(TP_UBSM_FREE_IPC_REQUEST, ret2);
    if (BresultFail(ret2)) {
        DBG_LOGERROR("Failed to free memory, memid=" << metaDesc.GetMinMemId() << ", name=" << metaDesc.GetName()
                                                     << ", error=" << ret2);
    } else {
        if (!metaDesc.GetIsNuma()) {
            TP_TRACE_BEGIN(TP_UBSM_FREE_NUMA_UNMMAP);
            SystemAdapter::MemoryUnMap(ptr, metaDesc.GetActualMapFileSize());
            TP_TRACE_END(TP_UBSM_FREE_NUMA_UNMMAP, 0L);
        }
        if (BresultFail(BorrowAppMetaMgr::GetInstance().RemoveMeta(ptr))) {
            DBG_LOGWARN("Failed to remove meta, name= " << metaDesc.GetName());
        }
    }

    DBG_LOGINFO("UbsMemFree end. ret is " << ret2);
    return ret2;
}

uint32_t RackMem::RackMemLookupClusterStatistic(ubsmem_cluster_info_t *info)
{
    uint32_t hresult = IpcCommand::IpcRackMemLookupClusterStatistic(info);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("RackMemLookupClusterStatistic fail, res is " << hresult);
        return hresult;
    }
    return UBSM_OK;
}

std::pair<int32_t, std::vector<std::string>> RackMem::QueryLeaseRecord()
{
    std::vector<std::string> result;
    auto hr = IpcCommand::AppQueryLeaseRecord(result);
    if (BresultFail(hr)) {
        DBG_LOGERROR("QueryLeaseRecord fail, res is " << hr);
        return std::make_pair(hr, result);
    }
    return std::make_pair(HOK, result);
}

uint32_t RackMem::ForceFreeCachedMemory()
{
    auto hr = IpcCommand::AppForceFreeCachedMemory();
    if (BresultFail(hr)) {
        DBG_LOGERROR("QueryLeaseRecord fail, res is " << hr);
        return hr;
    }
    return HOK;
}

} // namespace ock::mxmd
  // ock
