/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifndef MEMORYFABRIC_IPC_COMMAND_H
#define MEMORYFABRIC_IPC_COMMAND_H

#include <unistd.h>
#include "mxm_msg.h"
#include "rack_mem_err.h"
#include "rack_mem_libobmm.h"

namespace ock::mxmd {
class IpcCommand {
public:
    IpcCommand() = delete;
    ~IpcCommand() = default;
    static uint32_t AppMallocMemory(std::shared_ptr<AppMallocMemoryRequest> &request,
                                    std::shared_ptr<AppMallocMemoryResponse> &response);

    static uint32_t AppMallocMemoryWithLoc(std::shared_ptr<AppMallocMemoryWithLocRequest> &request,
                                           std::shared_ptr<AppMallocMemoryResponse> &response);

    static uint32_t AppFreeMemory(const std::string &regionName, const std::string &name,
                                  const std::vector<mem_id> &memIds);

    static uint32_t IpcRackMemLookupClusterStatistic(ubsmem_cluster_info_t *clusterInfo);

    static uint32_t AppQueryLeaseRecord(std::vector<std::string> &recordInfo);

    static uint32_t AppForceFreeCachedMemory();
    static uint32_t AppCheckLeaseMemoryResource();
};
} // namespace ock::mxmd

#endif // MEMORYFABRIC_IPC_COMMAND_H