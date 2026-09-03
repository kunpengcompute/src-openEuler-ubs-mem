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

#ifndef MEMORYFABRIC_RACKMEMSHM_H
#define MEMORYFABRIC_RACKMEMSHM_H

#include <mx_shm.h>
#include "ShmMetaDataMgr.h"
#include "rack_mem_lib_common.h"

namespace ock {
namespace mxmd {

class RackMemShm {
public:
    int32_t UbsMemShmSetOwnerShip(const std::string &name, void *start, size_t length, ShmOwnStatus status);

    int32_t UbsMemShmSetOwnerShipOpt(ShmAppMetaData &meta, void *start, size_t length, ShmOwnStatus status,
                                     uint64_t usedFirst, std::vector<int> &indices);

    uint32_t UbsMemShmCreate(const std::string &regionName, const std::string &name, uint64_t size,
                             const std::string &baseNid, const SHMRegionDesc &shmRegion, uint64_t flags, mode_t mode);

    uint32_t UbsMemShmCreateWithProvider(const ubs_mem_provider_t *srcLoc, const std::string &name, uint64_t size,
                                         uint64_t flags, mode_t mode);

    static uint32_t CheckRegionPar(const std::string &baseNid, const SHMRegionDesc &shmRegion);

    int UbsMemShmMmap(void *start, size_t mapSize, int prot, int flags, const std::string &name, off_t off,
                      void **local_ptr);

    uint32_t UbsMemShmUnmmap(void *start, size_t size);

    void RollBackUnmapFail(ShmAppMetaData &metaData, void *start);

    uint32_t UbsMemShmDelete(const std::string &name);

    uint32_t RackMemLookupResourceRegions(const std::string &baseNid, ShmRegionType type, SHMRegions &list);

    uint32_t RackMemCreateResourceRegion(const std::string &regionName, const SHMRegionDesc &region);

    uint32_t RackMemLookupResourceRegion(const std::string &regionName, SHMRegionDesc &region);

    uint32_t RackMemDestroyResourceRegion(const std::string &regionName);

    int32_t UbsMemShmWriteLock(const std::string &name);

    int32_t UbsMemShmReadLock(const std::string &name);

    int32_t UbsMemShmUnLock(const std::string &name);

    int32_t UbsMemShmAttach(const std::string &name);

    int32_t UbsMemShmDetach(const std::string &name);

    int32_t UbsMemShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &shm_list);

    int32_t UbsMemShmLookup(const std::string &name, ubsmem_shmem_info_t &shm_info);

    int RpcQueryInfo(std::string &name);

    int32_t UbsMemQueryNode(std::string &NodeId, bool &isNodeReady, bool isQueryMasterNode);

    int32_t UbsMemQueryDlockStatus(bool &isReady);

    int GetPreAllocateAddress(void *start, size_t length, int flags, void *&result);
    int GetPreAllocateAddressAligned(void *start, size_t length, int flags, void *&result, size_t align);

public:
    static RackMemShm &GetInstance()
    {
        static RackMemShm instance;
        return instance;
    }
    RackMemShm(const RackMemShm &other) = default;
    RackMemShm(RackMemShm &&other) = default;
    RackMemShm &operator=(const RackMemShm &other) = default;
    RackMemShm &operator=(RackMemShm &&other) noexcept = default;

private:
    RackMemShm() = default;
    std::unordered_map<void *, RackMemShmOwnStatus> addr2OwnStatus{};
    std::mutex mapMtx;
};
} // namespace mxmd
} // namespace ock

#endif // MEMORYFABRIC_RACKMEMSHM_H