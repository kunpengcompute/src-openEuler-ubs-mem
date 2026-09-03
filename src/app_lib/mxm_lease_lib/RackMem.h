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

#ifndef MEMORYFABRIC_RACKMEM_H
#define MEMORYFABRIC_RACKMEM_H

#include <cstdint>
#include "BorrowAppMetaMgr.h"
#include "mx_def.h"
#include "mxm_msg.h"

namespace ock::mxmd {
class RackMem {
public:
    int UbsMemMalloc(std::string &region, size_t size, PerfLevel level, uint64_t flags, void **local_ptr) const;
    int UbsMemMallocWithLoc(const ubs_mem_location_t *src_loc, size_t size, uint64_t flags, void **local_ptr) const;

    uint32_t UbsMemFree(void *ptr);

    void *MemoryIDUsedByFd(AppBorrowMetaDesc &desc, const std::vector<uint64_t> &memIds, size_t unitSize,
                           const std::string &cachedName, uint64_t flags);
    void *MemoryIDUsedByNuma(AppBorrowMetaDesc &desc, const std::vector<uint64_t> &memIds, int64_t numaId,
                             size_t unitSize, const std::string &cachedName);
    static uint32_t RackMemLookupClusterStatistic(ubsmem_cluster_info_t *info);

    std::pair<int32_t, std::vector<std::string>> QueryLeaseRecord();
    uint32_t ForceFreeCachedMemory();

    static RackMem &GetInstance()
    {
        static RackMem instance;
        return instance;
    }
    RackMem(const RackMem &other) = delete;
    RackMem(RackMem &&other) = delete;
    RackMem &operator=(const RackMem &other) = delete;
    RackMem &operator=(RackMem &&other) noexcept = delete;

private:
    RackMem() = default;
    int UbsMemMap(size_t size, uint64_t flags, std::shared_ptr<AppMallocMemoryResponse> &response,
                  void *&mappedMemory) const;
};
} // namespace ock::mxmd
// ock

#endif // MEMORYFABRIC_RACKMEM_H