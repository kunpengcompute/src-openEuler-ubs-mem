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

#include "RackMemShm.h"
#include <sys/mman.h>
#include "ubs_mem_def.h"
#include "ubs_mem.h"
#include "ShmMetaDataMgr.h"
#include "log.h"
#include "obmm.h"
#include "rack_mem_fd_map.h"
#include "rack_mem_functions.h"
#include "rack_mem_lib_common.h"
#include "rack_mem_macros.h"
#include "shm_ipc_command.h"
#include "system_adapter.h"
#include "ubsm_ptracer.h"

namespace ock::mxmd {
using namespace ock::common;
using namespace ock::ubsm;
uint32_t RackMemShm::UbsMemShmCreate(const std::string &regionName, const std::string &name, uint64_t size,
                                     const std::string &baseNid, const SHMRegionDesc &shmRegion, uint64_t flags,
                                     mode_t mode)
{
    DBG_LOGDEBUG("Start to create share memory, region name=" << regionName << ", memory name=" << name
                                                              << ", size=" << size << ", node id" << baseNid
                                                              << ", flags=" << flags << ", mode=" << mode);
    auto ret = CheckRegionPar(baseNid, shmRegion);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Base node id is not in regions, node id=" << baseNid);
        return ret;
    }
    ret = ShmIpcCommand::IpcCallShmCreate(regionName, name, size, baseNid, shmRegion, flags, mode);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Ipc call create interface, ret=" << ret);
        return ret;
    }
    DBG_LOGDEBUG("Create share memory successfully, name=" << name);
    return UBSM_OK;
}

uint32_t RackMemShm::UbsMemShmCreateWithProvider(const ubs_mem_provider_t *srcLoc, const std::string &name,
                                                 uint64_t size, uint64_t flags, mode_t mode)
{
    DBG_LOGDEBUG("Start to create share memory with provider, memory name="
                 << name << ", size=" << size << ", flags=" << flags << ", mode=" << mode
                 << ", provider host_name=" << srcLoc->host_name << ", socket_id=" << srcLoc->socket_id
                 << ", numa_id=" << srcLoc->numa_id << ", port_id=" << srcLoc->port_id);
    auto ret = ShmIpcCommand::IpcCallShmCreateWithProvider(srcLoc->host_name, srcLoc->socket_id, srcLoc->numa_id,
                                                           srcLoc->port_id, name, size, flags, mode);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Ipc call create with provider failed, ret=" << ret);
        return ret;
    }
    DBG_LOGDEBUG("Create share memory with provider successfully, name=" << name);
    return UBSM_OK;
}

static void UnMapWhenError(void *startAddr, size_t totalSize, const std::string &name, ShmAppMetaData &shmAppMetaData)
{
    shmAppMetaData.CloseAllFd();
    if (startAddr == nullptr) {
        DBG_LOGERROR("startAddr is nullptr, name=" << name);
        return;
    }
    if (totalSize == 0) {
        DBG_LOGERROR("totalSize is zero, name=" << name);
        return;
    }
    if (SystemAdapter::MemoryUnMap(startAddr, totalSize) == -1) {
        DBG_LOGERROR("Munmap failed, name=" << name << ", totalSize=" << totalSize);
    }
    // 这里unmmap不进行处理, 失败了靠巡检解决
    if (BresultFail(ShmIpcCommand::IpcCallShmUnMap(name))) {
        DBG_LOGERROR("IpcCallShmUnMap error " << name);
    }
}

static bool MapWithFixedFlags(int flags)
{
    return (flags & MAP_FIXED) == MAP_FIXED || (flags & MAP_FIXED_NOREPLACE) == MAP_FIXED_NOREPLACE;
}

uint32_t RackMemShmMmapParameterCheck(void *start, size_t mapSize, int prot, int flags, const std::string &name,
                                      off_t off)
{
    if (ShmMetaDataMgr::GetInstance().GetAddr(name) != nullptr) {
        // 同一个name只能map一次
        DBG_LOGERROR("shared memory name is repeated, name=" << name);
        return MXM_ERR_PARAM_INVALID;
    }
    if (mapSize == 0) {
        DBG_LOGERROR("Size is zero, name=" << name);
        return MXM_ERR_PARAM_INVALID;
    }
    if (off != 0) {
        DBG_LOGERROR("Offset is non-zero, name=" << name << ", offset=" << off);
        return MXM_ERR_PARAM_INVALID;
    }
    if ((prot != PROT_READ) && (prot != PROT_WRITE) && (prot != (PROT_READ | PROT_WRITE)) && prot != PROT_NONE) {
        DBG_LOGERROR("Prot is invalid, name=" << name << ", prot=" << prot);
        return MXM_ERR_PARAM_INVALID;
    }
    if (start == nullptr && MapWithFixedFlags(flags)) {
        DBG_LOGERROR("Invalid flags, start is nullptr, name=" << name << ", flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }
    auto pStart = reinterpret_cast<uintptr_t>(start);
    if (UNLIKELY(mapSize > std::numeric_limits<uintptr_t>::max() - pStart)) {
        DBG_LOGERROR("Address region is overflow. mapSize=" << mapSize);
        return MXM_ERR_PARAM_INVALID;
    }
    if (start != nullptr && MapWithFixedFlags(flags) &&
        ShmMetaDataMgr::GetInstance().HasOverlapWithKnownVmas(pStart, pStart + mapSize)) {
        DBG_LOGERROR("Overlap with known vmas, name=" << name);
        return MXM_ERR_PARAM_INVALID;
    }
    return UBSM_OK;
}

uint32_t RackMemShmMmapByMemID(void *mmapAddr, size_t unitSize, uint64_t mmapCount, std::vector<uint64_t> &memIds,
                               ShmAppMetaData &shmAppMetaData, uint64_t open_flag, int oflag, int prot, int flags)
{
    if (mmapCount > memIds.size()) {
        DBG_LOGERROR("mmapCount=" << mmapCount << " larger than memIds size=" << memIds.size());
        return MXM_ERR_PARAM_INVALID;
    }
    shmAppMetaData.addrs.clear();
    shmAppMetaData.fds.clear();
    uint32_t ret;
    off_t offset = 0;
    if ((open_flag & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD) {
        offset |= OBMM_MMAP_FLAG_HUGETLB_PMD;
    }
    for (uint64_t i = 0; i < mmapCount; i++) {
        const int fd = ObmmOpenInternal(memIds[i], open_flag, oflag);
        DBG_LOGINFO("It is " << i << "th mmap, mem id=" << memIds[i] << ", fd=" << fd << ", open_flag=" << open_flag);
        if (fd < 0) {
            DBG_LOGERROR("Obmm open error fd is " << fd);
            shmAppMetaData.CloseAllFd();
            return MXM_ERR_SHM_OBMM_OPEN;
        }
        shmAppMetaData.AddFds(fd);

        auto startAddr = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mmapAddr) + i * unitSize);
        if (i == mmapCount - 1) {
            size_t tmpLength = shmAppMetaData.mapSize - (mmapCount - 1) * unitSize;
            ret = RackMemFdMap::GetInstance().MapForEachFd(startAddr, tmpLength, fd, prot, flags, offset);
            if (BresultFail(ret)) {
                DBG_LOGERROR("MapForEachFd failed: fd=" << fd << ", ret=" << ret);
                shmAppMetaData.CloseAllFd();
                return ret;
            }
            shmAppMetaData.AddAddr(startAddr, tmpLength);
            break;
        }
        ret = RackMemFdMap::GetInstance().MapForEachFd(startAddr, unitSize, fd, prot, flags, offset);
        if (BresultFail(ret)) {
            DBG_LOGERROR("MapForEachFd failed: fd=" << fd << ", ret=" << ret);
            shmAppMetaData.CloseAllFd();
            return ret;
        }
        shmAppMetaData.AddAddr(startAddr, unitSize);
    }
    return UBSM_OK;
}

static inline ShmOwnStatus GetOwnStatus(int prot)
{
    switch (prot) {
        case PROT_NONE:
            return ShmOwnStatus ::UNACCESS;
        case PROT_READ:
            return ShmOwnStatus ::READ;
        case PROT_WRITE:
            return ShmOwnStatus ::WRITE;
        case PROT_READ | PROT_WRITE:
            return ShmOwnStatus ::READWRITE;
        default:
            return OWNBUFF;
    }
}

mode_t convertToFileMode(ShmOwnStatus status)
{
    mode_t mode = 0;
    switch (status) {
        case WRITE:
            mode = S_IWUSR;
            break;
        case READ:
            mode = S_IRUSR;
            break;
        case READWRITE:
            mode = S_IRUSR | S_IWUSR;
            break;
        default:
            mode = 0;
    }
    return mode;
}

int RackMemShm::GetPreAllocateAddressAligned(void *start, size_t length, int flags, void *&result, size_t align)
{
    void *mmapAddr = nullptr;
    if (start == nullptr) { // 未指定地址时，内部预先分配一段地址
        TP_TRACE_BEGIN(TP_UBSM_SHM_MMAP_ADDR);
        auto ret = RackMemFdMap::MemoryMapAligned(length, mmapAddr, align);
        TP_TRACE_END(TP_UBSM_SHM_MMAP_ADDR, ret);
        if (BresultFail(ret)) {
            DBG_LOGERROR("MemoryMapAligned failed, length is " << length);
            return MXM_ERR_MMAP_VA_FAILED;
        }
        result = mmapAddr;
        return MXM_OK;
    }
    bool isAlignedTo2M = reinterpret_cast<uintptr_t>(start) % HUGE_PAGES_2M == 0;
    if (!isAlignedTo2M) {
        DBG_LOGERROR("PMD mapping is not aligned to 2M");
        return MXM_ERR_PARAM_INVALID;
    }

    if (MapWithFixedFlags(flags)) {
        result = start;
        return MXM_OK;
    }

    // 指定地址，但是不带MAP_FIXED，先尝试能否匿名map这段地址，如果返回的地址是align对齐，就用这个地址，否则内部重新分配一段地址
    auto ret = RackMemFdMap::MapAnonymousMemory(start, length, mmapAddr);
    if (BresultFail(ret)) {
        DBG_LOGWARN("MapAnonymousMemory failed, length is " << length);
        return ret;
    }
    if (reinterpret_cast<uintptr_t>(mmapAddr) % align != 0) {
        (void)SystemAdapter::MemoryUnMap(mmapAddr, length);
        TP_TRACE_BEGIN(TP_UBSM_SHM_MMAP_ADDR);
        ret = RackMemFdMap::MemoryMapAligned(length, mmapAddr, align);
        TP_TRACE_END(TP_UBSM_SHM_MMAP_ADDR, ret);
        if (BresultFail(ret)) {
            DBG_LOGERROR("MemoryMapAligned failed, length is " << length);
            return MXM_ERR_MMAP_VA_FAILED;
        }
        result = mmapAddr;
        return MXM_OK;
    }
    result = mmapAddr;
    return MXM_OK;
}
// 前置有校验start为空时，flags一定不会有MAP_FIXED和MAP_FIXED_NOREPLACE
int RackMemShm::GetPreAllocateAddress(void *start, size_t length, int flags, void *&result)
{
    void *mmapAddr = nullptr;
    if (start == nullptr) { // 未指定地址时，内部预先分配一段地址
        TP_TRACE_BEGIN(TP_UBSM_SHM_MMAP_ADDR);
        auto ret = RackMemFdMap::MapAnonymousMemory(nullptr, length, mmapAddr);
        TP_TRACE_END(TP_UBSM_SHM_MMAP_ADDR, ret);
        if (BresultFail(ret)) {
            DBG_LOGERROR("MemoryMap2MAligned failed, length is " << length);
            return MXM_ERR_MMAP_VA_FAILED;
        }
        result = mmapAddr;
        return MXM_OK;
    }
    if (MapWithFixedFlags(flags)) {
        result = start;
        return MXM_OK;
    }
    // 指定地址，但是不带MAP_FIXED，先尝试能否匿名map这段地址，无论返回什么地址，都用返回的地址
    auto ret = RackMemFdMap::MapAnonymousMemory(start, length, mmapAddr);
    if (BresultFail(ret)) {
        DBG_LOGERROR("MapAnonymousMemory failed, length is " << length);
        return ret;
    }
    result = mmapAddr;
    return MXM_OK;
}

/**
 * 1.参数检查，2.重复map检查，3。从agent获取memid。4.obmm_memid_to_fd.5.poxsi map 6.update meta data
 */
int RackMemShm::UbsMemShmMmap(void *start, size_t mapSize, int prot, int flags, const std::string &name, off_t off,
                              void **local_ptr)
{
    if (local_ptr == nullptr) {
        DBG_LOGERROR("address is empty.");
        return MXM_ERR_PARAM_INVALID;
    }

    auto ownStatus = GetOwnStatus(prot);
    if (ownStatus >= OWNBUFF) {
        DBG_LOGERROR("Invalid param, prot=" << prot);
        return MXM_ERR_PARAM_INVALID;
    }

    uint32_t ret = RackMemShmMmapParameterCheck(start, mapSize, prot, flags, name, off);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Param invalid, name=" << name);
        return MXM_ERR_PARAM_INVALID;
    }
    std::vector<uint64_t> memIds{};
    size_t returnShmSize{0};
    size_t unitSize{0};
    uint64_t createFlags{0};
    int oflag{0};

    mode_t mode = convertToFileMode(ownStatus);
    DBG_LOGINFO("Shm mmap name=" << name << ", prot=" << prot << ", mode=" << mode << ", own status=" << ownStatus);
    TP_TRACE_BEGIN(TP_UBSM_SHM_MAP_IPC_REQUEST);
    ret = ShmIpcCommand::IpcCallShmMap(name, mapSize, memIds, returnShmSize, unitSize, createFlags, oflag, mode, prot);
    TP_TRACE_END(TP_UBSM_SHM_MAP_IPC_REQUEST, ret);
    if (BresultFail(ret)) {
        DBG_LOGERROR("Failed to mmap, name=" << name << ", mapSize=" << mapSize << ", ret=" << ret);
        return ret;
    }

    if (returnShmSize == 0 || mapSize != returnShmSize) {
        DBG_LOGERROR("Size error, mapSize=" << mapSize << ", ShmSize=" << returnShmSize);
        ShmIpcCommand::IpcCallShmUnMap(name);
        return ret;
    }
    static uint64_t alignment = GetHugeTlbPmdSize();
    if ((createFlags & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD && mapSize % alignment != 0) {
        DBG_LOGERROR("The mapSize is not aligned to " << alignment << ", mapSize=" << mapSize);
        return MXM_ERR_PARAM_INVALID;
    }

    void *mmapAddr = nullptr;
    uint64_t mmapCount = mapSize % unitSize == 0 ? mapSize / unitSize : mapSize / unitSize + 1;

    if ((createFlags & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD) {
        ret = GetPreAllocateAddressAligned(start, mapSize, flags, mmapAddr, alignment);
    } else {
        ret = GetPreAllocateAddress(start, mapSize, flags, mmapAddr);
    }
    if (BresultFail(ret)) {
        DBG_LOGERROR("GetPreAllocateAddress failed, name=" << name);
        ShmIpcCommand::IpcCallShmUnMap(name);
        return ret;
    }
    if (!MapWithFixedFlags(flags)) {
        flags |= MAP_FIXED;
    }

    DBG_LOGINFO("name=" << name << ", open flag=" << oflag << ", open_flag=" << createFlags << ", prot=" << prot);
    ShmAppMetaData shmAppMetaData{name, memIds, mmapAddr, returnShmSize, ownStatus, createFlags};
    shmAppMetaData.SetOflag(oflag);
    shmAppMetaData.SetUnitSize(unitSize);
    TP_TRACE_BEGIN(TP_UBSM_SHM_BIND_ADDR);
    ret = RackMemShmMmapByMemID(mmapAddr, unitSize, mmapCount, memIds, shmAppMetaData, createFlags, oflag, prot, flags);
    TP_TRACE_END(TP_UBSM_SHM_BIND_ADDR, ret);
    if (BresultFail(ret)) {
        DBG_LOGERROR("RackMemShmMmapByMemID failed, name=" << name);
        SystemAdapter::MemoryUnMap(mmapAddr, mapSize);
        ShmIpcCommand::IpcCallShmUnMap(name);
        return ret;
    }

    auto addResult = ShmMetaDataMgr::GetInstance().AddMetaData(name, mmapAddr, shmAppMetaData);
    if (BresultFail(addResult)) {
        DBG_LOGERROR("AddMetaData error " << addResult);
        UnMapWhenError(mmapAddr, mapSize, name, shmAppMetaData);
        return addResult;
    }
    *local_ptr = mmapAddr;
    return UBSM_OK;
}

void RackMemShm::RollBackUnmapFail(ShmAppMetaData &metaData, void *mmapAddr)
{
    uint64_t mmapCount{};
    uint64_t totalSize{};
    RackMemFdMap::GetInstance().GetActualMapSize(metaData.mapSize, metaData.unitSize, totalSize, mmapCount);
    auto ret = RackMemShmMmapByMemID(mmapAddr, metaData.unitSize, mmapCount, metaData.memIds, metaData, metaData.flag,
                                     metaData.oflag, PROT_NONE, MAP_SHARED | MAP_FIXED);
    if (BresultFail(ret)) {
        DBG_LOGWARN("RackMemShmMmapByMemID failed, name=" << metaData.name);
    }
    ShmMetaDataMgr::GetInstance().AddMetaData(metaData.name, mmapAddr, metaData);
}

uint32_t RackMemShm::UbsMemShmUnmmap(void *start, size_t size)
{
    uint32_t ret = UBSM_OK;
    if (start == nullptr) {
        DBG_LOGERROR("Param start is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    ShmAppMetaData metaData;
    ret = ShmMetaDataMgr::GetInstance().CheckNameExistAndGet(start, size, metaData);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("CheckNameExistAndGet error, size=" << size << ", ret=" << ret);
        return ret;
    }

    DBG_LOGINFO("Unmmapping shared memory, name=" << metaData.name << ", memIds=" << MemToStr(metaData.memIds));
    if (metaData.isLocked) {
        DBG_LOGERROR("name=" << metaData.name << " isLocked=" << metaData.isLocked);
        return MXM_ERR_LOCK_ALREADY_LOCKED;
    }

    TP_TRACE_BEGIN(TP_UBSM_UNMAP_ADDR);
    auto address =
        SystemAdapter::MemoryMap(start, metaData.mapSize, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0L);
    if (UNLIKELY(address == MAP_FAILED)) {
        DBG_LOGERROR("Failed to mmap memory with MAP_FIXED, " << strerror(errno));
        return MXM_ERR_MMAP_VA_FAILED;
    }
    TP_TRACE_END(TP_UBSM_UNMAP_ADDR, 0UL);

    TP_TRACE_BEGIN(TP_UBSM_CLOSE_FD);
    metaData.CloseAllFd();
    TP_TRACE_END(TP_UBSM_CLOSE_FD, 0UL);
    ShmMetaDataMgr::GetInstance().UpdateMetaData(metaData.name, metaData);

    TP_TRACE_BEGIN(TP_UBSM_SHM_UNMAP_IPC_UNMMAP);
    ret = ShmIpcCommand::IpcCallShmUnMap(metaData.name);
    TP_TRACE_END(TP_UBSM_SHM_UNMAP_IPC_UNMMAP, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("ShmIpcCommand::IpcCallShmUnMap Unmmap, ret=" << ret);
        RollBackUnmapFail(metaData, start);
        return ret;
    }
    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(start, metaData.name);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("RemoveMetaDataAfterUnMap error. name " << metaData.name << " code " << ret);
        return ret;
    }
    SystemAdapter::MemoryUnMap(start, metaData.mapSize);

    return ret;
}

uint32_t RackMemShm::UbsMemShmDelete(const std::string &name)
{
    auto ret = ShmIpcCommand::IpcCallShmDelete(name);
    if (BresultFail(ret)) {
        DBG_LOGERROR("ShmIpcCommand::IpcCallShmDelete Delete error " << ret);
        return ret;
    }
    return UBSM_OK;
}

/*
 * baseNid为-1，则在agent端进行检验，不为-1，则分为all2all和one2all两种类型进行检验
 *                                 */
uint32_t RackMemShm::CheckRegionPar(const std::string &baseNid, const SHMRegionDesc &shmRegion)
{
    if (shmRegion.num > MEM_TOPOLOGY_MAX_HOSTS || shmRegion.num <= 0) {
        DBG_LOGERROR("Number(" << shmRegion.num << ") in Region is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (shmRegion.type >= RackMemShmRegionType::INCLUDE_ALL_TYPE || shmRegion.type < 0) {
        DBG_LOGERROR("The shmRegions type is error, type=" << shmRegion.type);
        return MXM_ERR_PARAM_INVALID;
    }

    if (baseNid.empty()) {
        return UBSM_OK;
    }
    if (shmRegion.type == RackMemShmRegionType::ONE2ALL_SHARE) {
        if (shmRegion.nodeId[0] != baseNid) {
            DBG_LOGERROR("The region type is one2all, base node id is "
                         << baseNid << ", nodeId[0] is " << shmRegion.nodeId[0] << ", they should be the same.");
            return MXM_ERR_PARAM_INVALID;
        }
    }
    if (shmRegion.type == RackMemShmRegionType::ALL2ALL_SHARE) {
        for (int i = 0; i < shmRegion.num; i++) {
            if (baseNid == shmRegion.nodeId[i]) {
                return UBSM_OK;
            }
        }
        DBG_LOGERROR("The region type is all2all, but the node id " << baseNid << " is not in region.");
        return MXM_ERR_PARAM_INVALID;
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmSetOwnerShipOpt(ShmAppMetaData &meta, void *start, size_t length, ShmOwnStatus status,
                                             uint64_t usedFirst, std::vector<int> &indices)
{
    int prot = PROT_NONE;
    switch (status) {
        case RackMemShmOwnStatus::UNACCESS:
            DBG_LOGDEBUG("Access state change to OBMM_ACCESS_NONE metaData.name=" << meta.name);
            prot = PROT_NONE;
            break;
        case RackMemShmOwnStatus::READ:
            DBG_LOGDEBUG("Access state change to OBMM_ACCESS_READONLY metaData.name=" << meta.name);
            prot = PROT_READ;
            break;
        case RackMemShmOwnStatus::READWRITE:
            DBG_LOGDEBUG("Access state change to OBMM_ACCESS_READWRITE metaData.name=" << meta.name);
            prot = PROT_READ | PROT_WRITE;
            break;
        case RackMemShmOwnStatus::WRITE:
            DBG_LOGDEBUG("Access state change to OBMM_ACCESS_WRITE metaData.name=" << meta.name);
            prot = PROT_WRITE;
            break;
        default:
            break;
    }
    auto fun = RmLibObmmExecutor::GetInstance().obmmOwnershipSwitch;

    for (int i = 0; i < indices.size(); ++i) {
        auto localFd = meta.fds[indices[i]];
        auto offset = (i == 0) ? 0 : static_cast<size_t>(indices[i]) - static_cast<size_t>(indices[0]) - 1;

        if ((meta.unitSize == 0) || (offset > std::numeric_limits<size_t>::max() / meta.unitSize)) {
            DBG_LOGERROR("Overflow in offset * meta.unitSize, unitSize=" << meta.unitSize << ", offset=" << offset
                                                                         << "indices[i]=" << indices[i]
                                                                         << "indices[0]=" << indices[0]);
            return MXM_ERR_PARAM_INVALID;
        }
        size_t totalOffset = offset * meta.unitSize;

        if (usedFirst > length || totalOffset > length || totalOffset + usedFirst > length) {
            DBG_LOGERROR("Invalid parameters: usedFirst or totalOffset exceeds length");
            return MXM_ERR_PARAM_INVALID;
        }
        auto curLen = (i == 0) ? usedFirst : std::min((length - totalOffset - usedFirst), meta.unitSize);
        void *startAddr = (i == 0) ? start : meta.addrs[indices[i]].first;
        void *endAddr = static_cast<uint8_t *>(startAddr) + curLen;
        TP_TRACE_BEGIN(TP_USBM_OBMM_SET_OWERSHIP);
        auto ret = fun(localFd, startAddr, endAddr, prot);
        TP_TRACE_END(TP_USBM_OBMM_SET_OWERSHIP, ret);
        DBG_LOGDEBUG("Access state change fd=" << localFd << ", prot=" << prot << ", ret=" << ret);
        if (ret != 0) {
            DBG_LOGERROR("obmmOwnershipSwitch error, code is " << ret);
            return MXM_ERR_OBMM_INNER;
        }
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmSetOwnerShip(const std::string &name, void *start, size_t length, ShmOwnStatus status)
{
    if (start == nullptr) {
        DBG_LOGERROR("Param start is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (name.empty()) {
        DBG_LOGERROR("Param name is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (length <= 0) {
        DBG_LOGERROR("Param length is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    ShmAppMetaData meta;
    uint64_t usedFirst;
    std::vector<int> indices{};
    auto result = ShmMetaDataMgr::GetInstance().CheckAddr(name, start, length, meta, usedFirst, indices);
    if (BresultFail(result) || indices.size() <= 0) {
        DBG_LOGERROR("Find shm by addr failed, ret: " << result << ", length is " << length << ", size is "
                                                      << indices.size());
        return result;
    }
    if (meta.addrs.size() != meta.fds.size()) {
        DBG_LOGERROR("Meta addrs.size(" << meta.addrs.size() << ")is not equal to fds.size(" << meta.fds.size() << ")");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    if ((meta.flag & UBSM_FLAG_ONLY_IMPORT_NONCACHE) != 0 || (meta.flag & UBSM_FLAG_NONCACHE) != 0) {
        DBG_LOGERROR("shm name( " << name << ") allocate flag is invalid, flag: " << meta.flag);
        return MXM_ERR_PARAM_INVALID;
    }
    auto ret = UbsMemShmSetOwnerShipOpt(meta, start, length, status, usedFirst, indices);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmSetOwnerShipOpt failed, ret: " << ret);
        return ret;
    }
    return UBSM_OK;
}

uint32_t RackMemShm::RackMemLookupResourceRegions(const std::string &baseNid, ShmRegionType type, SHMRegions &list)
{
    const uint32_t hresult = ShmIpcCommand::IpcCallLookupRegionList(baseNid, type, list);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("Failed to calling IpcCallLookupRegionList, error=" << hresult);
        return hresult;
    }
    return UBSM_OK;
}

uint32_t RackMemShm::RackMemCreateResourceRegion(const std::string &regionName, const SHMRegionDesc &region)
{
    const uint32_t hresult = ShmIpcCommand::IpcCallCreateRegion(regionName, region);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("Failed to calling IpcCallCreateRegion, error=" << hresult);
        return hresult;
    }
    return UBSM_OK;
}

uint32_t RackMemShm::RackMemLookupResourceRegion(const std::string &regionName, SHMRegionDesc &region)
{
    const uint32_t hresult = ShmIpcCommand::IpcCallLookupRegion(regionName, region);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("Failed to calling IpcCallLookupRegion, error=" << hresult);
        return hresult;
    }
    return UBSM_OK;
}

uint32_t RackMemShm::RackMemDestroyResourceRegion(const std::string &regionName)
{
    const uint32_t hresult = ShmIpcCommand::IpcCallDestroyRegion(regionName);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("Failed to calling IpcCallDestroyRegion error=" << hresult);
        return hresult;
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmWriteLock(const std::string &name)
{
    ShmAppMetaData meta;
    auto result = ShmMetaDataMgr::GetInstance().GetShmMetaFromName(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Find shm by addr failed, ret: " << result);
        return result;
    }
    if (meta.addrs.size() != meta.fds.size()) {
        DBG_LOGERROR("Meta addrs.size(" << meta.addrs.size() << ")is not equal to fds.size(" << meta.fds.size() << ")");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (meta.name != name) {
        DBG_LOGERROR("Found shm name(" << meta.name << ") is not equal to param name(" << name << ").");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    if (meta.flag != UBSM_FLAG_WITH_LOCK) {
        DBG_LOGERROR("shm name(" << name << ") allocate flag(" << meta.flag << ") is not UBSM_FLAG_WITH_LOCK.");
        return MXM_ERR_LOCK_NOT_READY;
    }
    TP_TRACE_BEGIN(TP_UBSM_WRITE_LOCK_IPC_REQUEST);
    auto ret = ShmIpcCommand::IpcShmemWriteLock(name);
    TP_TRACE_END(TP_UBSM_WRITE_LOCK_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("IpcShmemWriteLock failed, ret is:" << ret);
        return ret;
    }
    std::vector<int> indexs{};
    for (int i = 0; i < meta.addrs.size(); ++i) {
        indexs.push_back(i);
    }

    TP_TRACE_BEGIN(TP_UBSM_WRITE_LOCK_SET_OWERSHIP);
    ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShipOpt(meta, meta.addr, meta.mapSize, READWRITE, meta.unitSize,
                                                             indexs);
    TP_TRACE_END(TP_UBSM_WRITE_LOCK_SET_OWERSHIP, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmSetOwnerShipOpt failed, ret:" << ret);
        ShmIpcCommand::IpcShmemUnLock(name);
        return ret;
    }

    meta.isLocked++; // 对应name的shm 加锁

    result = ShmMetaDataMgr::GetInstance().UpdateMetaData(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Update shm meta by name failed, ret: " << result);
        return result;
    }
    DBG_LOGINFO("Ipc Shm WriteLock success, name=" << name);

    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmReadLock(const std::string &name)
{
    ShmAppMetaData meta;
    auto result = ShmMetaDataMgr::GetInstance().GetShmMetaFromName(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Find shm by name failed, ret: " << result);
        return result;
    }
    if (meta.addrs.size() != meta.fds.size()) {
        DBG_LOGERROR("Meta addrs.size(" << meta.addrs.size() << ")is not equal to fds.size(" << meta.fds.size() << ")");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (meta.name != name) {
        DBG_LOGERROR("Found shm name(" << meta.name << ") is not equal to param name(" << name << ").");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    if (meta.flag != UBSM_FLAG_WITH_LOCK) {
        DBG_LOGERROR("shm name(" << name << ") allocate flag(" << meta.flag << ") is not UBSM_FLAG_WITH_LOCK.");
        return MXM_ERR_LOCK_NOT_READY;
    }
    TP_TRACE_BEGIN(TP_UBSM_READ_LOCK_IPC_REQUEST);
    auto ret = ShmIpcCommand::IpcShmemReadLock(name);
    TP_TRACE_END(TP_UBSM_READ_LOCK_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("IpcShmemReadLock failed, ret is:" << ret);
        return ret;
    }
    std::vector<int> indexs{};
    for (int i = 0; i < meta.addrs.size(); ++i) {
        indexs.push_back(i);
    }
    TP_TRACE_BEGIN(TP_UBSM_READ_LOCK_SET_OWNERSHIP);
    ret =
        RackMemShm::GetInstance().UbsMemShmSetOwnerShipOpt(meta, meta.addr, meta.mapSize, READ, meta.unitSize, indexs);
    TP_TRACE_END(TP_UBSM_READ_LOCK_SET_OWNERSHIP, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmSetOwnerShipOpt failed, ret:" << ret);
        ShmIpcCommand::IpcShmemUnLock(name);
        return ret;
    }

    meta.isLocked++; // 对应name的shm 加锁
    result = ShmMetaDataMgr::GetInstance().UpdateMetaData(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Update shm meta by name failed, ret: " << result);
        return result;
    }
    DBG_LOGINFO("Ipc Shm ReadLock success, name=" << name);

    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmUnLock(const std::string &name)
{
    ShmAppMetaData meta;

    auto result = ShmMetaDataMgr::GetInstance().GetShmMetaFromName(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Find shm by addr failed, ret: " << result);
        return result;
    }
    if (meta.addrs.size() != meta.fds.size()) {
        DBG_LOGERROR("Meta addrs.size(" << meta.addrs.size() << ")is not equal to fds.size(" << meta.fds.size() << ")");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (meta.name != name) {
        DBG_LOGERROR("Found shm name(" << meta.name << ") is not equal to param name(" << name << ").");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    if (meta.flag != UBSM_FLAG_WITH_LOCK) {
        DBG_LOGERROR("shm name(" << name << ") allocate flag(" << meta.flag << ") is not UBSM_FLAG_WITH_LOCK.");
        return MXM_ERR_LOCK_NOT_READY;
    }

    if (meta.isLocked <= 0) {
        DBG_LOGERROR("shm name(" << name << ") is not locked. ");
        return MXM_ERR_LOCK_NOT_FOUND;
    }

    std::vector<int> indexs{};
    for (int i = 0; i < meta.addrs.size(); ++i) {
        indexs.push_back(i);
    }

    TP_TRACE_BEGIN(TP_UBSM_UNLOCK_SET_OWNERSHIP);
    auto ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShipOpt(meta, meta.addr, meta.mapSize, UNACCESS,
                                                                  meta.unitSize, indexs);
    TP_TRACE_END(TP_UBSM_UNLOCK_SET_OWNERSHIP, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmSetOwnerShipOpt failed, ret:" << ret);
        return ret;
    }

    meta.isLocked--; // 对应name的shm 解锁
    result = ShmMetaDataMgr::GetInstance().UpdateMetaData(name, meta);
    if (BresultFail(result)) {
        DBG_LOGERROR("Update shm meta by name failed, ret: " << result);
        return result;
    }
    DBG_LOGINFO("Ipc Shm UnLock name=" << name);
    TP_TRACE_BEGIN(TP_UBSM_UNLOCK_IPC_REQUEST);
    ret = ShmIpcCommand::IpcShmemUnLock(name);
    TP_TRACE_END(TP_UBSM_UNLOCK_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("IpcShmemUnLock failed, ret is:" << ret);
        return ret;
    }
    DBG_LOGINFO("Ipc Shm UnLock success, name=" << name);

    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmAttach(const std::string &name)
{
    auto ret = ShmIpcCommand::IpcCallShmAttach(name);
    if (BresultFail(ret)) {
        DBG_LOGERROR("attach failed ret :" << ret << ", name is " << name);
        return ret;
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmDetach(const std::string &name)
{
    auto ret = ShmIpcCommand::IpcCallShmDetach(name);
    if (BresultFail(ret)) {
        DBG_LOGERROR("detach failed ret :" << ret << ", name is " << name);
        return ret;
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &shm_list)
{
    auto ret = ShmIpcCommand::IpcCallShmListLookup(prefix, shm_list);
    if (BresultFail(ret)) {
        DBG_LOGERROR("listlookup failed ret :" << ret << ", prefix is " << prefix);
        return ret;
    }
    return UBSM_OK;
}

int32_t RackMemShm::UbsMemShmLookup(const std::string &name, ubsmem_shmem_info_t &shm_info)
{
    auto ret = ShmIpcCommand::IpcCallShmLookup(name, shm_info);
    if (BresultFail(ret)) {
        DBG_LOGERROR("lookup failed ret :" << ret << ", name is " << name);
        return ret;
    }
    return UBSM_OK;
}

int RackMemShm::RpcQueryInfo(std::string &name)
{
    auto hresult = ShmIpcCommand::IpcCallRpcQuery(name);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("RpcQueryInfo fail, res is " << hresult);
        return MXM_ERR_RPC_QUERY_ERROR;
    }
    return 0;
}

int32_t RackMemShm::UbsMemQueryNode(std::string &NodeId, bool &isNodeReady, bool isQueryMasterNode)
{
    auto hresult = ShmIpcCommand::IpcQueryNode(NodeId, isNodeReady, isQueryMasterNode);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("UbsMemQueryNode fail, res is " << hresult);
        return MXM_ERR_MEMLIB;
    }
    return 0;
}

int32_t RackMemShm::UbsMemQueryDlockStatus(bool &isReady)
{
    auto hresult = ShmIpcCommand::IpcQueryDlockStatus(isReady);
    if (BresultFail(hresult)) {
        DBG_LOGERROR("UbsMemQueryDlockStatus fail, res is " << hresult);
        return MXM_ERR_MEMLIB;
    }
    return 0;
}
} // namespace ock::mxmd
  // ock