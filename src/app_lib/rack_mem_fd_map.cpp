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
#include "rack_mem_fd_map.h"
#include "ubs_mem_def.h"
#include "log.h"
#include "system_adapter.h"

namespace ock::mxmd {
using namespace ock::common;
uint32_t RackMemFdMap::FileMapByTotalSize(const uint64_t size, void *&virtualAddr, int prot)
{
    if (size == 0) {
        DBG_LOGERROR("Size=" << size << " is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto tempVirtualAddr = ubsm::SystemAdapter::MemoryMap(nullptr, size, prot, MAP_SHARED | MAP_ANONYMOUS, -1, 0L);
    if (tempVirtualAddr == MAP_FAILED) {
        DBG_LOGERROR("Failed to mmap, size=" << size << ", error info=" << strerror(errno));
        return MXM_ERR_MMAP_VA_FAILED;
    }
    virtualAddr = tempVirtualAddr;
    return UBSM_OK;
}

uint32_t RackMemFdMap::MapAnonymousMemory(void *start, uint64_t size, void *&virtualAddr)
{
    if (size == 0) {
        DBG_LOGERROR("Size=" << size << " is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto tempVirtualAddr = ubsm::SystemAdapter::MemoryMap(start, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0L);
    if (tempVirtualAddr == MAP_FAILED) {
        DBG_LOGERROR("Failed to mmap, size=" << size << ", error info=" << strerror(errno));
        return MXM_ERR_MMAP_VA_FAILED;
    }
    virtualAddr = tempVirtualAddr;
    return UBSM_OK;
}

// 固定映射区域
uint32_t RackMemFdMap::MapForEachFd(void *startAddr, size_t length, int fd, int prot, int flags, off_t offset)
{
    void *virtualAddr = ubsm::SystemAdapter::MemoryMap(startAddr, length, prot, flags, fd, offset);
    if (virtualAddr == MAP_FAILED) {
        DBG_LOGERROR("Failed to mmap, fd=" << fd << ", errno info=" << strerror(errno) << ", length=" << length
                                           << ", prot=" << prot << ", flags=" << flags << " offset=" << offset);
        return MXM_ERR_MMAP_VA_FAILED;
    }

    if (virtualAddr != startAddr) {
        DBG_LOGERROR("Failed to mmap, fd=" << fd << ", length=" << length << ", prot=" << prot << ", flags=" << flags);
        return MXM_ERR_MMAP_VA_FAILED;
    }
    return UBSM_OK;
}

void RackMemFdMap::GetActualMapSize(const uint64_t appMmapSize, const uint64_t unitSize, uint64_t &actualMapSize,
                                    uint64_t &mmapCount)
{
    if (unitSize == 0) {
        DBG_LOGERROR("error, unit size=" << unitSize);
        actualMapSize = 0;
        return;
    }
    mmapCount = appMmapSize % unitSize == 0 ? appMmapSize / unitSize : appMmapSize / unitSize + 1;

    // 检查乘法溢出
    const auto maxValue = static_cast<uint64_t>(-1);
    if (mmapCount > maxValue / unitSize) {
        DBG_LOGERROR("Overflow in actualMapSize calculation");
        actualMapSize = 0;
        return;
    }
    actualMapSize = mmapCount * unitSize;
    DBG_LOGINFO("Input mmap size=" << appMmapSize << ", actual mmap size=" << actualMapSize
                                   << ", unit size=" << unitSize << ", mmapCount=" << mmapCount);
}

int RackMemFdMap::MemoryMapAligned(size_t size, void *&result, size_t align)
{
    void *alignedMemory = nullptr;
    auto ret = posix_memalign(&alignedMemory, align, size);
    if (ret != 0) {
        DBG_LOGERROR("Failed to align memory size, align=" << align << ", error info=" << std::strerror(errno));
        return MXM_ERR_MALLOC_FAIL;
    }
    result = mmap(alignedMemory, size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (result == MAP_FAILED) {
        DBG_LOGERROR("Failed to mmap memory, error info=" << std::strerror(errno) << ", size=" << size
                                                          << ", prot=" << PROT_NONE);
        free(alignedMemory);
        return MXM_ERR_MMAP_VA_FAILED;
    }
    return MXM_OK;
}
} // namespace ock::mxmd
