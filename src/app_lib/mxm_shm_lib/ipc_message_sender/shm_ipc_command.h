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

#ifndef SHM_MEMORYFABRIC_IPC_COMMAND_H
#define SHM_MEMORYFABRIC_IPC_COMMAND_H

#include <unistd.h>
#include "mx_shm.h"
#include "rack_mem_err.h"

namespace ock::mxmd {
class ShmIpcCommand {
public:
    ShmIpcCommand() = delete;
    ~ShmIpcCommand() = default;

    static uint32_t IpcCallShmMap(const std::string &name, uint64_t mapSize, std::vector<uint64_t> &memIds,
                                  size_t &shmSize, size_t &unitSize, uint64_t &flag, int &oflag, mode_t mode, int prot);
    static uint32_t IpcCallShmCreate(const std::string &regionName, const std::string &name, uint64_t size,
                                     const std::string &nid, const SHMRegionDesc &shmRegionDesc, uint64_t flags,
                                     mode_t mode);
    static uint32_t IpcCallShmCreateWithProvider(const std::string &hostName, uint32_t socketId, uint32_t numaId,
                                                 uint32_t portId, const std::string &name, uint64_t size,
                                                 uint64_t flags, mode_t mode);
    static uint32_t IpcCallShmDelete(const std::string &name);
    static uint32_t IpcCallShmLookRegionList(const std::string &baseNid, ShmRegionType type, SHMRegions &list);
    static uint32_t IpcCallShmUnMap(const std::string &name);
    static int32_t IpcShmemReadLock(const std::string &name);
    static int32_t IpcShmemUnLock(const std::string &name);
    static uint32_t IpcCallRpcQuery(std::string &name);

    static uint32_t IpcCallLookupRegionList(const std::string &baseNid, ShmRegionType type, SHMRegions &list);
    static uint32_t IpcCallCreateRegion(const std::string &name, const SHMRegionDesc &region);
    static uint32_t IpcCallLookupRegion(const std::string &name, SHMRegionDesc &region);
    static uint32_t IpcCallDestroyRegion(const std::string &name);

    static int32_t IpcShmemWriteLock(const std::string &name);

    static uint32_t IpcCallShmAttach(const std::string &name);
    static uint32_t IpcCallShmDetach(const std::string &name);
    static uint32_t IpcCallShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &shm_list);
    static uint32_t IpcCallShmLookup(const std::string &name, ubsmem_shmem_info_t &shm_info);
    static uint32_t IpcQueryNode(std::string &NodeId, bool &isNodeReady, bool isQueryMasterNode);
    static uint32_t IpcQueryDlockStatus(bool &isReady);

    static uint32_t AppCheckShareMemoryResource();

    static uint32_t AppGetLocalSlotId(uint32_t &slotId);

private:
};
} // namespace ock::mxmd

#endif // SHM_MEMORYFABRIC_IPC_COMMAND_H