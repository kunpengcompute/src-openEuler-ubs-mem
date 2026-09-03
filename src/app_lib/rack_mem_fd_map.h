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
#ifndef MEMORYFABRIC_RACK_MEM_FD_MAP_H
#define MEMORYFABRIC_RACK_MEM_FD_MAP_H

#include "rack_mem_err.h"
namespace ock::mxmd {

class RackMemFdMap {
public:
    static RackMemFdMap &GetInstance()
    {
        static RackMemFdMap instance;
        return instance;
    }
    RackMemFdMap(const RackMemFdMap &other) = default;
    RackMemFdMap(RackMemFdMap &&other) = default;
    RackMemFdMap &operator=(const RackMemFdMap &other) = default;
    RackMemFdMap &operator=(RackMemFdMap &&other) noexcept = default;

    uint32_t FileMapByTotalSize(uint64_t size, void *&virtualAddr, int prot);

    static uint32_t MapAnonymousMemory(void *start, uint64_t size, void *&virtualAddr);

    uint32_t MapForEachFd(void *startAddr, size_t length, int fd, int prot, int flags, off_t offset = 0);

    void GetActualMapSize(uint64_t appMmapSize, uint64_t unitSize, uint64_t &actualMapSize, uint64_t &mmapCount);

    static int MemoryMapAligned(size_t size, void *&result, size_t align);

    DAGGER_DEFINE_REF_COUNT_FUNCTIONS;

private:
    DAGGER_DEFINE_REF_COUNT_VARIABLE;

    RackMemFdMap() = default;
};
} // namespace ock::mxmd
#endif // MEMORYFABRIC_RACK_MEM_FD_MAP_H
