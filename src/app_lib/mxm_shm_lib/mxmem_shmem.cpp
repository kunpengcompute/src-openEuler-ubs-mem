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

#include <sys/mman.h>
#include "ubs_mem.h"
#include "RackMemShm.h"
#include "UbseMemExecutor.h"
#include "log.h"
#include "mx_shm.h"
#include "rack_mem_functions.h"
#include "rack_mem_lib.h"
#include "rack_mem_macros.h"
#include "shm_ipc_command.h"
#include "ubsm_ptracer.h"

using namespace ock::mxmd;
using namespace ock::common;
static const auto PAGE_SIZE = static_cast<size_t>(sysconf(_SC_PAGESIZE));

static int find_region_desc(const char *region_name, SHMRegionDesc *region)
{
    if (region_name == nullptr || region == nullptr) {
        DBG_LOGERROR("Region name or region is empty");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("Check region, name=" << region_name);
    if (strcmp(region_name, "default") == 0) {
        std::string baseNid;
        ShmRegionType type = ALL2ALL_SHARE;
        SHMRegions *regions = static_cast<SHMRegions *>(malloc(sizeof(SHMRegions)));
        if (regions == nullptr) {
            DBG_LOGERROR("SHMRegions malloc failed.");
            return MXM_ERR_MALLOC_FAIL;
        }
        TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGION_DEFAULT_IPC_REQUEST);
        auto ret = ShmIpcCommand::IpcCallShmLookRegionList(baseNid, type, *regions);
        TP_TRACE_END(TP_UBSM_LOOKUP_REGION_DEFAULT_IPC_REQUEST, ret);
        if (ret != 0) {
            DBG_LOGERROR("Failed to lookup shared regions, ret=" << ret);
            free(regions);
            return ret;
        }
        if (regions->num <= 0) {
            DBG_LOGERROR("Failed to lookup shared Region, region number=" << ret);
            free(regions);
            return ret;
        }

        *region = regions->region[0];
        free(regions);

        DBG_LOGINFO("Get region successfully, name=" << region_name);
        return UBSM_OK;
    }

    std::string regionName = region_name;
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGION_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().RackMemLookupResourceRegion(regionName, *region);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGION_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("LookupResourceRegion fail, ret=" << ret);
        return ret;
    }

    DBG_LOGINFO("Get region successfully, name=" << region_name);
    return UBSM_OK;
}

static int translate_to_region_attributes(SHMRegionDesc *regionDesc, ubsmem_region_attributes_t *region_attr)
{
    region_attr->host_num = regionDesc->num;

    for (int i = 0; i < regionDesc->num; i++) {
        auto ret = strcpy_s(region_attr->hosts[i].host_name, MAX_HOST_NAME_DESC_LENGTH, regionDesc->hostName[i]);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("host name copy error, ret=" << ret);
            return MXM_ERR_MEMORY;
        }
        region_attr->hosts[i].affinity = regionDesc->affinity[i];
        DBG_LOGINFO("Coping region name=" << region_attr->hosts[i].host_name
                                          << ", affinity=" << region_attr->hosts[i].affinity << " to list");
    }

    return UBSM_OK;
}

static int translate_to_regions(SHMRegions *list, ubsmem_regions_t *regions)
{
    regions->num = list->num;

    for (int i = 0; i < list->num; i++) {
        auto ret = translate_to_region_attributes(&list->region[i], &regions->region[i]);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("region attribute copy error, ret=" << ret);
            return ret;
        }
    }

    return UBSM_OK;
}

static int translate_to_region_desc(const char *region_name, SHMRegionDesc *regionDesc,
                                    ubsmem_region_desc_t *region_desc)
{
    auto ret = strcpy_s(region_desc->region_name, MAX_REGION_NAME_DESC_LENGTH, region_name);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("region name copy error, ret:" << ret);
        return MXM_ERR_MEMORY;
    }
    region_desc->size = 0;
    ret = translate_to_region_attributes(regionDesc, &region_desc->region_attr);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("region attributes copy error, ret=" << ret);
        return ret;
    }
    return UBSM_OK;
}

static bool contain_all_hosts_in_attr(SHMRegionDesc *regionDesc, const ubsmem_region_attributes_t *reg_attr)
{
    bool flag[MAX_REGION_NODE_NUM] = {false};
    int i;
    int j;

    for (i = 0; i < reg_attr->host_num; i++) {
        for (j = 0; j < regionDesc->num; j++) {
            if (flag[j]) {
                continue;
            }
            DBG_LOGDEBUG("Host name=" << reg_attr->hosts[i].host_name);
            if (strcmp(reg_attr->hosts[i].host_name, regionDesc->hostName[j]) == 0) {
                flag[j] = true;
                regionDesc->affinity[j] = reg_attr->hosts[i].affinity;
                DBG_LOGDEBUG("affinity[" << j << "]=" << regionDesc->affinity[j] << ", i=" << i);
                break;
            }
        }
        if (j == regionDesc->num) {
            DBG_LOGERROR("Failed to find host name=" << reg_attr->hosts[i].host_name);
            return false;
        }
    }
    int k = 0;
    for (j = 0; j < regionDesc->num; j++) {
        if (!flag[j]) {
            continue;
        }
        if (k != j) {
            auto ret = strcpy_s(regionDesc->nodeId[k], MEM_MAX_ID_LENGTH, regionDesc->nodeId[j]);
            if (ret != UBSM_OK) {
                DBG_LOGERROR("Failed to copy node id, ret=" << ret);
                return false;
            }
            ret = strcpy_s(regionDesc->hostName[k], MAX_HOST_NAME_DESC_LENGTH, regionDesc->hostName[j]);
            if (ret != UBSM_OK) {
                DBG_LOGERROR("Failed to copy host name, ret=" << ret);
                return false;
            }
            regionDesc->affinity[k] = regionDesc->affinity[j];
        }
        k++;
    }
    regionDesc->num = k;
    return true;
}

static int filter_all_hosts_in_attr(SHMRegions *list, const ubsmem_region_attributes_t *reg_attr, int &index)
{
    for (int i = 0; i < list->num; i++) {
        if (contain_all_hosts_in_attr(&list->region[i], reg_attr)) {
            index = i;
            return UBSM_OK;
        }
    }

    DBG_LOGERROR("Failed to find designated region.");
    return MXM_ERR_CHECK_RESOURCE;
}

static int create_region_check(const char *region_name, size_t size, const ubsmem_region_attributes_t *reg_attr)
{
    if (region_name == nullptr || reg_attr == nullptr) {
        DBG_LOGERROR("regions is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (strcmp(region_name, "default") == 0) {
        DBG_LOGERROR("Region name 'default' is reserved.");
        return MXM_ERR_REGION_NO_NEEDED;
    }

    if (size != 0 || reg_attr->host_num > MAX_REGION_NODE_NUM || reg_attr->host_num < NO_2) {
        DBG_LOGERROR("size or host num is invalid, size=" << size << ", host number=" << reg_attr->host_num
                                                          << ", it should be in (" << MAX_REGION_NODE_NUM << ", "
                                                          << NO_2 << ")");
        return MXM_ERR_PARAM_INVALID;
    }

    auto length = strnlen(region_name, MAX_REGION_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_REGION_NAME_DESC_LENGTH) {
        DBG_LOGERROR("region name(" << region_name << ") length(" << length << ") is 0 or over limit ("
                                    << MAX_REGION_NAME_DESC_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    for (int i = 0; i < reg_attr->host_num; i++) {
        auto len = strnlen(reg_attr->hosts[i].host_name, MAX_HOST_NAME_DESC_LENGTH);
        if (len == 0 || len >= MAX_HOST_NAME_DESC_LENGTH) {
            DBG_LOGERROR("host name(" << reg_attr->hosts[i].host_name << ") length(" << len << ") is 0 or over limit ("
                                      << MAX_HOST_NAME_DESC_LENGTH << ").");
            return MXM_ERR_PARAM_INVALID;
        }
    }

    return UBSM_OK;
}

bool WithCacheableFlag(uint64_t flags)
{
    return (flags & UBSM_FLAG_NONCACHE) == 0 && (flags & UBSM_FLAG_ONLY_IMPORT_NONCACHE) == 0 &&
           (flags & UBSM_FLAG_WITH_LOCK) == 0;
}

bool ValidateFlag(uint64_t flags)
{
    constexpr uint64_t FLAGS_MASK = UBSM_FLAG_CACHE | UBSM_FLAG_WITH_LOCK | UBSM_FLAG_NONCACHE |
                                    UBSM_FLAG_WR_DELAY_COMP | UBSM_FLAG_ONLY_IMPORT_NONCACHE | UBSM_FLAG_MEM_ANONYMOUS |
                                    UBSM_FLAG_MMAP_HUGETLB_PMD;

    if ((flags & FLAGS_MASK) != flags) {
        DBG_LOGERROR("ValidateFlag failed, Invalid flags:" << flags << " mask: " << FLAGS_MASK);
        return false;
    }

    constexpr uint64_t COMMON_MASK = UBSM_FLAG_MEM_ANONYMOUS | UBSM_FLAG_MMAP_HUGETLB_PMD;
    // NC flag的 只能选这几个flag
    constexpr uint64_t NC_MASK = UBSM_FLAG_NONCACHE | UBSM_FLAG_WR_DELAY_COMP | COMMON_MASK;
    if ((flags & UBSM_FLAG_NONCACHE) != 0 && ((flags & NC_MASK) != flags)) {
        DBG_LOGERROR("ValidateFlag failed, Invalid flags:" << flags);
        return false;
    }

    // IMPORT_NC flag的 只能选这几个flag
    constexpr uint64_t IMPORT_NC_MASK = UBSM_FLAG_ONLY_IMPORT_NONCACHE | UBSM_FLAG_WR_DELAY_COMP | COMMON_MASK;
    if ((flags & UBSM_FLAG_ONLY_IMPORT_NONCACHE) != 0 && ((flags & IMPORT_NC_MASK) != flags)) {
        DBG_LOGERROR("ValidateFlag failed, Invalid flags:" << flags);
        return false;
    }
    // WITH_LOCK flag的 只能选这几个flag
    constexpr uint64_t LOCK_MASK = UBSM_FLAG_WITH_LOCK | COMMON_MASK;
    if ((flags & UBSM_FLAG_WITH_LOCK) != 0 && ((flags & LOCK_MASK) != flags)) {
        DBG_LOGERROR("ValidateFlag failed, Invalid flags:" << flags);
        return false;
    }

    // CC flag的 只能选这几个flag
    constexpr uint64_t CC_MASK = UBSM_FLAG_CACHE | COMMON_MASK;
    if (WithCacheableFlag(flags) && ((flags & CC_MASK) != flags)) {
        DBG_LOGERROR("ValidateFlag failed, Invalid flags:" << flags);
        return false;
    }
    return true;
}

uint32_t ubsmem_shmem_allocate_impl(const char *region_name, const char *name, size_t size, mode_t mode, uint64_t flags)
{
    if (name == nullptr || region_name == nullptr) {
        DBG_LOGERROR("Memory name or region name is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto length = strnlen(region_name, MAX_REGION_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_REGION_NAME_DESC_LENGTH) {
        DBG_LOGERROR("Region name length(" << length << ") is 0 or over limit(" << MAX_REGION_NAME_DESC_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    length = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (length == 0 || length >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("Name length(" << length << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    const uint64_t pageSize = ROUND_UP(size, PAGE_SIZE);
    if (!CheckRackMemSize(pageSize)) {
        DBG_LOGERROR("size is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (!ValidateFlag(flags)) {
        DBG_LOGERROR("Invalid flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }
    static auto hugePageSize = GetHugeTlbPmdSize();
    if ((flags & UBSM_FLAG_MMAP_HUGETLB_PMD) == UBSM_FLAG_MMAP_HUGETLB_PMD && size % hugePageSize != 0) {
        DBG_LOGERROR("The size is not aligned to " << hugePageSize << ", size=" << size);
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("Allocating shared memory, region=" << region_name << ", name=" << name << ", size=" << size);
    std::string regionName = region_name;
    std::string baseNid;
    auto regions = static_cast<SHMRegionDesc *>(malloc(sizeof(SHMRegionDesc)));
    if (regions == nullptr) {
        DBG_LOGERROR("SHMRegions malloc failed.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto ret = find_region_desc(region_name, regions);
    if (ret != 0) {
        DBG_LOGERROR("RackMemShmLookupShareRegions failed, ret=" << ret);
        free(regions);
        return ret;
    }

    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE_IPC_REQUEST);
    ret = RackMemShm::GetInstance().UbsMemShmCreate(regionName, name, pageSize, baseNid, *regions, flags, mode);
    TP_TRACE_END(TP_UBSM_SHM_CREATE_IPC_REQUEST, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmCreate failed, ret=" << ret);
        free(regions);
        return ret;
    }

    free(regions);
    DBG_LOGINFO("Allocating shared memory successfully, memory name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_allocate(const char *region_name, const char *name, size_t size, mode_t mode, uint64_t flags)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE);
    const auto ret = ubsmem_shmem_allocate_impl(region_name, name, size, mode, flags);
    TP_TRACE_END(TP_UBSM_SHM_CREATE, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_allocate_with_provider_impl(const ubs_mem_provider_t *src_loc, const char *name, size_t size,
                                                  mode_t mode, uint64_t flags)
{
    if (name == nullptr || src_loc == nullptr) {
        DBG_LOGERROR("Memory name or src loc is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto length = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (length == 0 || length >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("Name length(" << length << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    if (src_loc->host_name[0] == '\0') {
        DBG_LOGERROR("Host name is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    length = strnlen(src_loc->host_name, MAX_HOST_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_HOST_NAME_DESC_LENGTH) {
        DBG_LOGERROR("host name length(" << length << ") is 0 or over limit (" << MAX_HOST_NAME_DESC_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    const uint64_t pageSize = ROUND_UP(size, PAGE_SIZE);
    if (!CheckRackMemSize(pageSize)) {
        DBG_LOGERROR("size is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (!ValidateFlag(flags)) {
        DBG_LOGERROR("Invalid flags=" << flags);
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Allocating shared memory, name=" << name << ", size=" << size << ", flags=" << flags
                                                  << ", mode=" << mode << ", provider host_name=" << src_loc->host_name
                                                  << ", socket_id=" << src_loc->socket_id << ", numa_id="
                                                  << src_loc->numa_id << ", port_id=" << src_loc->port_id);

    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE_WITH_PROVIDER_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().UbsMemShmCreateWithProvider(src_loc, name, pageSize, flags, mode);
    TP_TRACE_END(TP_UBSM_SHM_CREATE_WITH_PROVIDER_IPC_REQUEST, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmCreateWithProvider failed, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("Allocating shared memory successfully, memory name=" << name);
    return MXM_OK;
}

SHMEM_API int ubsmem_shmem_allocate_with_provider(const ubs_mem_provider_t *src_loc, const char *name, size_t size,
                                                  mode_t mode, uint64_t flags)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE_WITH_PROVIDER);
    const auto ret = ubsmem_shmem_allocate_with_provider_impl(src_loc, name, size, mode, flags);
    TP_TRACE_END(TP_UBSM_SHM_CREATE_WITH_PROVIDER, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_deallocate_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR("name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto length = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (length == 0 || length >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name(" << name << ") length(" << length << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH
                             << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGDEBUG("Start to delete shm, name=" << name);
    const auto ret = RackMemShm::GetInstance().UbsMemShmDelete(name);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmDelete failed, ret:" << ret);
        return ret;
    }
    DBG_LOGDEBUG("Deleting shared memory successfully, name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_deallocate(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_DELETE_IPC_REQUEST);
    const auto ret = ubsmem_shmem_deallocate_impl(name);
    TP_TRACE_END(TP_UBSM_SHM_DELETE_IPC_REQUEST, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_map_impl(void *addr, size_t length, int prot, int flags, const char *name, off_t offset,
                               void **local_ptr)
{
    if (name == nullptr) {
        DBG_LOGERROR("Name is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (local_ptr == nullptr) {
        DBG_LOGERROR("Address for storing result is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (addr != nullptr && reinterpret_cast<uintptr_t>(addr) % PAGE_SIZE != 0) {
        DBG_LOGERROR("The address is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (length <= 0) {
        DBG_LOGERROR("The length is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (len == 0 || len >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    if ((flags | MAP_PRIVATE) == MAP_PRIVATE) {
        DBG_LOGERROR("The parameter flags=" << flags << " is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Mmapping shared memory, name=" << name << ", length=" << length << ", offset=" << offset
                                                << ", prot=" << prot);
    const size_t pageLength = ROUND_UP(length, PAGE_SIZE);
    const off_t pageOffset = ROUND_UP(static_cast<unsigned long int>(offset), PAGE_SIZE);

    void *start = addr;
    if (addr == nullptr) {
        DBG_LOGINFO("address is empty, set start to 0.");
        start = reinterpret_cast<void *>(0);
    }

    void *localPtr = nullptr;
    auto ret = RackMemShm::GetInstance().UbsMemShmMmap(start, pageLength, prot, flags, name, pageOffset, &localPtr);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to mmap shared memory, ret=" << ret);
        return ret;
    }
    if (localPtr == nullptr) {
        DBG_LOGERROR("Failed to get address");
        return MXM_ERR_MMAP_VA_FAILED;
    }
    *local_ptr = localPtr;
    DBG_LOGDEBUG("Mmapping shared memory successfully, name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_map(void *addr, size_t length, int prot, int flags, const char *name, off_t offset,
                               void **local_ptr)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_MAP);
    const auto ret = ubsmem_shmem_map_impl(addr, length, prot, flags, name, offset, local_ptr);
    TP_TRACE_END(TP_UBSM_SHM_MAP, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_unmap_impl(void *local_ptr, size_t length)
{
    if (local_ptr == nullptr) {
        DBG_LOGERROR("local_ptr is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (length <= 0) {
        DBG_LOGERROR("length is invalid, length=" << length);
        return MXM_ERR_PARAM_INVALID;
    }
    const size_t pageLength = ROUND_UP(length, PAGE_SIZE);
    DBG_LOGINFO("Unmmapping shared memory, length=" << length);
    const auto ret = RackMemShm::GetInstance().UbsMemShmUnmmap(local_ptr, pageLength);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmUnmmap failed, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("Unmmapping shared memory successfully, length=" << length);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_unmap(void *local_ptr, size_t length)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_UNMAP);
    const auto ret = ubsmem_shmem_unmap_impl(local_ptr, length);
    TP_TRACE_END(TP_UBSM_SHM_UNMAP, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_set_ownership_impl(const char *name, void *start, size_t length, int prot)
{
    if (name == nullptr) {
        DBG_LOGERROR("name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (len == 0 || len >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    if (start == nullptr) {
        DBG_LOGERROR("start is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (length <= 0 || ((length % FOUR_KB) != 0)) {
        DBG_LOGERROR("length is invalid : " << length);
        return MXM_ERR_PARAM_INVALID;
    }
    const size_t pageLength = ROUND_UP(length, PAGE_SIZE);
    ShmOwnStatus status;
    switch (prot) {
        case PROT_NONE:
            status = ShmOwnStatus::UNACCESS;
            break;
        case PROT_READ:
            status = ShmOwnStatus::READ;
            break;
        case (PROT_READ | PROT_WRITE):
            status = ShmOwnStatus::READWRITE;
            break;
        default:
            DBG_LOGERROR("prot is invalid : " << prot);
            return MXM_ERR_PARAM_INVALID;
    }

    auto ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShip(name, start, pageLength, status);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmSetOwnerShip failed, ret:" << ret);
        return ret;
    }
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_set_ownership(const char *name, void *start, size_t length, int prot)
{
    TP_TRACE_BEGIN(TP_UBSM_SET_SHMEM_OWERSHIP);
    const auto hr = ubsmem_shmem_set_ownership_impl(name, start, length, prot);
    TP_TRACE_END(TP_UBSM_SET_SHMEM_OWERSHIP, hr);
    return GetErrCode(hr);
}

uint32_t ubsm_lookup_regions_ompl(ubsmem_regions_t *regions)
{
    if (regions == nullptr) {
        DBG_LOGERROR("regions is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (RackMemLib::GetInstance().StartRackMem() != 0) {
        DBG_LOGERROR("library of ubsm has not been initilized");
        return MXM_ERR_MEMLIB;
    }

    DBG_LOGINFO("Start to looking up regions");
    std::string baseNid;
    ShmRegionType type = ALL2ALL_SHARE;
    SHMRegions *list = static_cast<SHMRegions *>(malloc(sizeof(SHMRegions)));
    if (list == nullptr) {
        DBG_LOGERROR("Failed to malloc, error info=no memory.");
        return MXM_ERR_MALLOC_FAIL;
    }
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGIONS_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().RackMemLookupResourceRegions(baseNid, type, *list);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGIONS_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("LookupResourceRegions fail, ret is: " << ret);
        free(list);
        return ret;
    }

    ret = translate_to_regions(list, regions);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Translating regiosn to nodes list, ret=" << ret);
        free(list);
        return ret;
    }
    free(list);
    DBG_LOGINFO("Looking up regions successfully");
    return UBSM_OK;
}

int ubsmem_lookup_regions(ubsmem_regions_t *regions)
{
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGIONS);
    uint32_t hr = ubsm_lookup_regions_ompl(regions);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGIONS, hr);
    return GetErrCode(hr);
}

uint32_t ubsmem_create_region_impl(const char *region_name, size_t size, const ubsmem_region_attributes_t *reg_attr)
{
    auto ret = create_region_check(region_name, size, reg_attr);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Check param fail, ret=" << ret);
        return ret;
    }

    if (RackMemLib::GetInstance().StartRackMem() != 0) {
        DBG_LOGERROR("Library of ubsm has not been initilized");
        return MXM_ERR_MEMLIB;
    }

    std::string baseNid;
    ShmRegionType type = ALL2ALL_SHARE;
    SHMRegions *list = static_cast<SHMRegions *>(malloc(sizeof(SHMRegions)));
    if (list == nullptr) {
        DBG_LOGERROR("Failed to SHMRegions, error info=no memory");
        return MXM_ERR_MALLOC_FAIL;
    }

    DBG_LOGINFO("Start to create region, region name=" << region_name << ", size=" << size);
    for (int i = 0; i < reg_attr->host_num; ++i) {
        DBG_LOGINFO("Host name=" << reg_attr->hosts[i].host_name << ", affinity=" << reg_attr->hosts[i].affinity);
    }
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGIONS_IPC_REQUEST);
    ret = RackMemShm::GetInstance().RackMemLookupResourceRegions(baseNid, type, *list);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGIONS_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to look up regions, ret=" << ret);
        free(list);
        return ret;
    }

    int index = 0;
    ret = filter_all_hosts_in_attr(list, reg_attr, index);
    if (ret != UBSM_OK || index >= MAX_REGIONS_NUM) {
        DBG_LOGERROR("Failed to filter designated region, index=" << index << ", ret=" << ret);
        free(list);
        return ret;
    }

    std::string regionName = region_name;
    TP_TRACE_BEGIN(TP_UBSM_CREATE_REGIONS_IPC_REQUEST);
    ret = RackMemShm::GetInstance().RackMemCreateResourceRegion(regionName, list->region[index]);
    TP_TRACE_END(TP_UBSM_CREATE_REGIONS_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to create resource region, ret=" << ret);
        free(list);
        return ret;
    }

    free(list);

    DBG_LOGINFO("Create region successfully, region name=" << region_name);
    return UBSM_OK;
}

int ubsmem_create_region(const char *region_name, size_t size, const ubsmem_region_attributes_t *reg_attr)
{
    TP_TRACE_BEGIN(TP_UBSM_CREATE_REGIONS);
    uint32_t hr = ubsmem_create_region_impl(region_name, size, reg_attr);
    TP_TRACE_END(TP_UBSM_CREATE_REGIONS, hr);
    return GetErrCode(hr);
}

uint32_t ubsmem_lookup_region_impl(const char *region_name, ubsmem_region_desc_t *region_desc)
{
    if (region_name == nullptr || region_desc == nullptr) {
        DBG_LOGERROR("regions is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }

    auto length = strnlen(region_name, MAX_REGION_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_REGION_NAME_DESC_LENGTH) {
        DBG_LOGERROR("region_name length(" << length << ") is 0 or over limit (" << MAX_REGION_NAME_DESC_LENGTH
                                           << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    if (RackMemLib::GetInstance().StartRackMem() != 0) {
        DBG_LOGERROR("init mem lib error!");
        return MXM_ERR_MEMLIB;
    }

    SHMRegionDesc *region = static_cast<SHMRegionDesc *>(malloc(sizeof(SHMRegionDesc)));
    if (region == nullptr) {
        DBG_LOGERROR("SHMRegion malloc failed.");
        return MXM_ERR_MALLOC_FAIL;
    }

    DBG_LOGINFO("Start to looking up region, region name=" << region_name);
    std::string regionName = region_name;
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGION_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().RackMemLookupResourceRegion(regionName, *region);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGION_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to look up resource region, ret=" << ret);
        free(region);
        return ret;
    }

    ret = translate_to_region_desc(region_name, region, region_desc);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("translate region info, ret=" << ret);
        free(region);
        return ret;
    }
    free(region);
    DBG_LOGINFO("Looking up region successfully, region name=" << region_name);
    return UBSM_OK;
}

int ubsmem_lookup_region(const char *region_name, ubsmem_region_desc_t *region_desc)
{
    TP_TRACE_BEGIN(TP_UBSM_LOOKUP_REGION);
    uint32_t hr = ubsmem_lookup_region_impl(region_name, region_desc);
    TP_TRACE_END(TP_UBSM_LOOKUP_REGION, hr);
    return GetErrCode(hr);
}

uint32_t ubsmem_destroy_region_impl(const char *region_name)
{
    if (region_name == nullptr) {
        DBG_LOGERROR("regions is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (strcmp(region_name, "default") == 0) {
        DBG_LOGERROR("Region name 'default' is reserved.");
        return MXM_ERR_REGION_NO_NEEDED;
    }

    auto length = strnlen(region_name, MAX_REGION_NAME_DESC_LENGTH);
    if (length == 0 || length >= MAX_REGION_NAME_DESC_LENGTH) {
        DBG_LOGERROR("region_name length(" << length << ") is 0 or over limit (" << MAX_REGION_NAME_DESC_LENGTH
                                           << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    if (RackMemLib::GetInstance().StartRackMem() != 0) {
        DBG_LOGERROR("init mem lib error!");
        return MXM_ERR_MEMLIB;
    }

    DBG_LOGINFO("Start to destory region, region name=" << region_name);
    std::string regionName = region_name;
    TP_TRACE_BEGIN(TP_UBSM_DESTORY_REGION_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().RackMemDestroyResourceRegion(regionName);
    TP_TRACE_END(TP_UBSM_DESTORY_REGION_IPC_REQUEST, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Fail to sestroy reesource, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("Start to destory region, region name=" << region_name);
    return UBSM_OK;
}

int ubsmem_destroy_region(const char *region_name)
{
    TP_TRACE_BEGIN(TP_UBSM_DESTORY_REGION);
    auto ret = ubsmem_destroy_region_impl(region_name);
    TP_TRACE_END(TP_UBSM_DESTORY_REGION, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_write_lock_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR(" desc name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (len == 0 || len >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    auto ret = RackMemShm::GetInstance().UbsMemShmWriteLock(name);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmWriteLock failed, ret:" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsMemShmWriteLock success, name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_write_lock(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_WRITE_LOCK);
    uint32_t ret = ubsmem_shmem_write_lock_impl(name);
    TP_TRACE_END(TP_UBSM_WRITE_LOCK, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_read_lock_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR(" desc name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (len == 0 || len >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    auto ret = RackMemShm::GetInstance().UbsMemShmReadLock(name);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmReadLock failed, ret:" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsMemShmReadLock success, name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_read_lock(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_READ_LOCK);
    uint32_t ret = ubsmem_shmem_read_lock_impl(name);
    TP_TRACE_END(TP_UBSM_READ_LOCK, ret);
    return GetErrCode(ret);
}
uint32_t ubsmem_shmem_unlock_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR(" desc name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH);
    if (len == 0 || len >= MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }

    auto ret = RackMemShm::GetInstance().UbsMemShmUnLock(name);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmUnLock failed, ret:" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsMemShmUnLock success, name=" << name);
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_unlock(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_UNLOCK);
    uint32_t ret = ubsmem_shmem_unlock_impl(name);
    TP_TRACE_END(TP_UBSM_UNLOCK, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_attach_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR("name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH + 1);
    if (len == 0 || len > MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    TP_TRACE_BEGIN(TP_UBSM_SHM_ATTACH_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().UbsMemShmAttach(name);
    TP_TRACE_END(TP_UBSM_SHM_ATTACH_IPC_REQUEST, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmAttach failed, ret:" << ret);
        return ret;
    }
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_attach(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_ATTACH);
    uint32_t ret = ubsmem_shmem_attach_impl(name);
    TP_TRACE_END(TP_UBSM_SHM_ATTACH, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_detach_impl(const char *name)
{
    if (name == nullptr) {
        DBG_LOGERROR("name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH + 1);
    if (len == 0 || len > MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    TP_TRACE_BEGIN(TP_UBSM_SHM_ATTACH_IPC_REQUEST);
    auto ret = RackMemShm::GetInstance().UbsMemShmDetach(name);
    TP_TRACE_END(TP_UBSM_SHM_ATTACH_IPC_REQUEST, ret);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmDetach failed, ret:" << ret);
        return ret;
    }

    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_detach(const char *name)
{
    TP_TRACE_BEGIN(TP_UBSM_SHM_DETACH);
    uint32_t ret = ubsmem_shmem_detach_impl(name);
    TP_TRACE_END(TP_UBSM_SHM_DETACH, ret);
    return GetErrCode(ret);
}

uint32_t ubsmem_shmem_listlookup_impl(const char *prefix, ubsmem_shmem_desc_t *shm_list, uint32_t *shm_cnt)
{
    if (prefix == nullptr) {
        DBG_LOGERROR("prefix is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (shm_list == nullptr) {
        DBG_LOGERROR("shm_list is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (shm_cnt == nullptr) {
        DBG_LOGERROR("shm_cnt is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(prefix, MAX_SHM_NAME_LENGTH + 1);
    if (len == 0 || len > MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("prefix length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    std::vector<ubsmem_shmem_desc_t> shm_vec;
    auto ret = RackMemShm::GetInstance().UbsMemShmListLookup(prefix, shm_vec);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmListLookup failed, ret:" << ret);
        return ret;
    }
    if (shm_vec.size() > UINT32_MAX) {
        DBG_LOGERROR("shm_vec.size() exceeds uint32_t limit");
        return MXM_ERR_PARAM_INVALID;
    }
    if (shm_vec.size() > *shm_cnt) {
        DBG_LOGERROR("Input shm_cnt (" << *shm_cnt << ") is less than required (" << shm_vec.size() << ").");
        *shm_cnt = static_cast<uint32_t>(shm_vec.size());
        return MXM_ERR_PARAM_INVALID;
    }
    *shm_cnt = static_cast<uint32_t>(shm_vec.size());
    for (uint32_t i = 0; i < *shm_cnt; i++) {
        ret = strcpy_s(shm_list[i].name, MAX_SHM_NAME_LENGTH + 1, shm_vec[i].name);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("shm name copy error, ret:" << ret);
            return MXM_ERR_MEMORY;
        }
        shm_list[i].size = shm_vec[i].size;
    }
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_list_lookup(const char *prefix, ubsmem_shmem_desc_t *shm_list, uint32_t *shm_cnt)
{
    return GetErrCode(ubsmem_shmem_listlookup_impl(prefix, shm_list, shm_cnt));
}

uint32_t ubsmem_shmem_lookup_impl(const char *name, ubsmem_shmem_info_t *shm_info)
{
    if (name == nullptr) {
        DBG_LOGERROR("name is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto len = strnlen(name, MAX_SHM_NAME_LENGTH + 1);
    if (len == 0 || len > MAX_SHM_NAME_LENGTH) {
        DBG_LOGERROR("name length(" << len << ") is 0 or over limit (" << MAX_SHM_NAME_LENGTH << ").");
        return MXM_ERR_PARAM_INVALID;
    }
    auto ret = RackMemShm::GetInstance().UbsMemShmLookup(name, *shm_info);
    if (ret != 0) {
        DBG_LOGERROR("UbsMemShmLookup failed, ret:" << ret);
        return ret;
    }
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_lookup(const char *name, ubsmem_shmem_info_t *shm_info)
{
    return GetErrCode(ubsmem_shmem_lookup_impl(name, shm_info));
}

uint32_t ubsmem_local_nid_query_impl(uint32_t *nid)
{
    uint32_t slotId = 0;
    auto ret = ShmIpcCommand::AppGetLocalSlotId(slotId);
    if (ret != 0) {
        DBG_LOGERROR("AppGetLocalSlotId failed ret: " << ret);
        return ret;
    }
    *nid = slotId;
    return UBSM_OK;
}

SHMEM_API int ubsmem_local_nid_query(uint32_t *nid)
{
    if (nid == nullptr) {
        DBG_LOGERROR("nid is nullptr.");
        return UBSM_ERR_PARAM_INVALID;
    }
    return GetErrCode(ubsmem_local_nid_query_impl(nid));
}

uint32_t ubsmem_shmem_faults_register_impl(shmem_faults_func registerFunc)
{
    if (registerFunc == nullptr) {
        DBG_LOGERROR("registerFunc is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    auto ret = UbseMemExecutor::GetInstance().Initialize();
    if (ret != 0) {
        DBG_LOGERROR("UbseMemExecutor Initialize failed ret: " << ret);
        return ret;
    }
    ret = UbseMemExecutor::GetInstance().RegisterShmFaultEvent(registerFunc);
    if (ret != 0) {
        DBG_LOGERROR("RegisterShmFaultEvent failed ret: " << ret);
        return ret;
    }
    return UBSM_OK;
}

SHMEM_API int ubsmem_shmem_faults_register(shmem_faults_func registerFunc)
{
    return GetErrCode(ubsmem_shmem_faults_register_impl(registerFunc));
}