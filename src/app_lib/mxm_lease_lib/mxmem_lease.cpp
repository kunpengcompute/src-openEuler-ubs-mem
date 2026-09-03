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

#include <mutex>
#include "ubs_mem.h"
#include "RackMem.h"
#include "log.h"
#include "rack_mem_lib.h"
#include "ubsm_ptracer.h"

using namespace ock::mxmd;
using namespace ock::common;

static bool ubsmem_lease_validate_flags(uint64_t flags)
{
    // 目前只支持两个flag
    if (flags != 0 && flags != UBSM_FLAG_MMAP_HUGETLB_PMD && flags != UBSM_FLAG_MALLOC_WITH_NUMA) {
        return false;
    }
    return true;
}

uint32_t ubsmem_lease_malloc_impl(const char *region_name, size_t size, ubsmem_distance_t mem_distance, uint64_t flags,
                                  void **local_ptr)
{
    if (region_name == nullptr) {
        DBG_LOGERROR("region_name is nullptr");
        return MXM_ERR_PARAM_INVALID;
    }
    auto length = strnlen(region_name, MAX_REGION_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_REGION_NAME_DESC_LENGTH) {
        DBG_LOGERROR("region_name is" << region_name << ", region_name length(" << length << ") is 0 or over limit ("
                                      << MAX_REGION_NAME_DESC_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    if (local_ptr == nullptr) {
        DBG_LOGERROR("region_name is " << region_name << ", local_ptr is nullptr");
        return MXM_ERR_PARAM_INVALID;
    }
    if (!CheckRackMemSize(size)) {
        DBG_LOGERROR("Size is " << size);
        return MXM_ERR_PARAM_INVALID;
    }
    if (!ubsmem_lease_validate_flags(flags)) {
        DBG_LOGERROR("The flags are invalid, flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }
    static auto hugePagesSize = GetHugeTlbPmdSize();
    if ((flags & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD && size % hugePagesSize != 0) {
        DBG_LOGERROR("The size is not aligned to " << hugePagesSize << ", size=" << size);
        return MXM_ERR_PARAM_INVALID;
    }
    if (mem_distance != ubsmem_distance_t::DISTANCE_DIRECT_NODE) {
        DBG_LOGERROR("The ubsmem_distance_t is " << mem_distance);
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Appling remote memory, region name=" << region_name << ", size=" << size << " distance="
                                                      << static_cast<int>(mem_distance) << " flags=" << flags);
    auto perfLevel = static_cast<PerfLevel>(mem_distance);
    std::string regionName = region_name;
    void *localPtr = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(regionName, size, perfLevel, flags, &localPtr);
    if (ret != MXM_OK) {
        DBG_LOGERROR("UbsMemMalloc failed, region=" << regionName << ", ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("Applying remote memory successfully, region name=" << region_name);
    *local_ptr = localPtr;
    return UBSM_OK;
}

uint32_t ubsmem_lease_malloc_with_location_impl(const ubs_mem_location_t *src_loc, size_t size, uint64_t flags,
                                                void **local_ptr)
{
    if (src_loc == nullptr) {
        DBG_LOGERROR("src_loc is nullptr");
        return MXM_ERR_PARAM_INVALID;
    }
    if (local_ptr == nullptr) {
        DBG_LOGERROR("local_ptr is nullptr");
        return MXM_ERR_PARAM_INVALID;
    }
    if (!CheckRackMemSize(size)) {
        DBG_LOGERROR("Check size failed, Size=" << size);
        return MXM_ERR_PARAM_INVALID;
    }
    if (!ubsmem_lease_validate_flags(flags)) {
        DBG_LOGERROR("The flags are invalid, flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Applying remote memory with loc,  size="
                << size << ", flags=" << flags << ", location slotId=" << src_loc->slot_id << ", socketId="
                << src_loc->socket_id << ", numaId=" << src_loc->numa_id << ", portId=" << src_loc->port_id);
    void *localPtr = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMallocWithLoc(src_loc, size, flags, &localPtr);
    if (ret != MXM_OK) {
        DBG_LOGERROR("UbsMemMallocWithLoc failed, location slotId="
                     << src_loc->slot_id << ", socketId=" << src_loc->socket_id << ", numaId=" << src_loc->numa_id
                     << ", portId=" << src_loc->port_id << ", ret=" << ret);
        return ret;
    }

    if (localPtr == nullptr) {
        DBG_LOGERROR("Failed to get address");
        return MXM_ERR_MMAP_VA_FAILED;
    }

    DBG_LOGINFO("Applying remote memory with loc success, size=" << size << ", flags=" << flags);
    *local_ptr = localPtr;
    return MXM_OK;
}

SHMEM_API int ubsmem_lease_malloc(const char *region_name, size_t size, ubsmem_distance_t mem_distance, uint64_t flags,
                                  void **local_ptr)
{
    TP_TRACE_BEGIN(TP_UBSM_MALLOC);
    const auto ret = ubsmem_lease_malloc_impl(region_name, size, mem_distance, flags, local_ptr);
    TP_TRACE_END(TP_UBSM_MALLOC, ret);
    return GetErrCode(ret);
}

SHMEM_API int ubsmem_lease_malloc_with_location(const ubs_mem_location_t *src_loc, size_t size, uint64_t flags,
                                                void **local_ptr)
{
    TP_TRACE_BEGIN(TP_UBSM_MALLOC_LOC);
    const auto ret = ubsmem_lease_malloc_with_location_impl(src_loc, size, flags, local_ptr);
    TP_TRACE_END(TP_UBSM_MALLOC_LOC, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_lease_free_impl(void *local_ptr)
{
    if (local_ptr == nullptr) {
        DBG_LOGERROR("Param local_ptr is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    return RackMem::GetInstance().UbsMemFree(local_ptr);
}

SHMEM_API int ubsmem_lease_free(void *local_ptr)
{
    TP_TRACE_BEGIN(TP_UBSM_FREE);
    uint32_t hr = ubsmem_lease_free_impl(local_ptr);
    TP_TRACE_END(TP_UBSM_FREE, hr);
    return GetErrCode(hr);
}

int ubsmem_lookup_cluster_statistic_impl(ubsmem_cluster_info_t *info)
{
    if (info == nullptr) {
        DBG_LOGERROR("Param info is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }

    return RackMem::GetInstance().RackMemLookupClusterStatistic(info);
}

SHMEM_API int ubsmem_lookup_cluster_statistic(ubsmem_cluster_info_t *info)
{
    return GetErrCode(ubsmem_lookup_cluster_statistic_impl(info));
}