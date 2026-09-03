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
#ifndef UBSE_MEM_ADAPTER_H
#define UBSE_MEM_ADAPTER_H

#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "mx_shm.h"
#include "rack_mem_functions.h"
#include "region_repository.h"
#include "ubs_common_types.h"
#include "ubs_engine.h"
#include "ubs_engine_log.h"
#include "ubs_engine_mem.h"
#include "ubs_engine_topo.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace ock {
namespace mxm {

constexpr auto MIN_NODE_NUM = 2;

// 使用方进程信息
typedef struct ubse_uds_info {
    uid_t uid; /* 使用方进程的运行用户的uid */
    gid_t gid; /* 使用方进程的的运行用户的groupid */
    pid_t pid; /* 使用方进程的的运行用户的pid */
} ubse_uds_info_t;

// 用户信息
typedef struct ubse_user_info_t {
    ubse_uds_info udsInfo; /* 使用方进程信息 */
    mode_t mode;
    uint64_t flag;
    uint64_t seqNo;
} ubse_user_info_t;

enum MemNodeDistance {
    MEM_DISTANCE_L0, /* *L0对应直连节点 */
    MEM_DISTANCE_L1, /* *L1对应通过1跳节点，暂不支持 */
    MEM_DISTANCE_L2  /* *L2对应过超过1跳节点 ，暂不支持 */
};

struct LeaseMallocParam {
    std::string name;
    std::string regionName;
    size_t size{0};
    bool isNuma{false};
    MemNodeDistance distance{MEM_DISTANCE_L0};
    ubsm::AppContext appContext{};
};

struct LeaseMallocWithLocParam {
    std::string name;
    std::string regionName;
    size_t size{0};
    bool isNuma{false};
    ubsm::AppContext appContext{};
    uint32_t slotId;
    uint32_t socketId;
    uint32_t numaId;
    uint32_t portId;
};

struct CreateShmParam {
    std::string name;
    size_t size{0};
    ubsm::AppContext appContext{};
    std::vector<uint8_t> userData;
    std::string baseNid;
    SHMRegionDesc desc;
    uint64_t flag{};
    mode_t mode;
    ubs_mem_nodes_t privider;
};

struct CreateShmWithProviderParam {
    std::string hostName;
    uint32_t socketId;
    uint32_t numaId;
    uint32_t portId;
    std::string name;
    size_t size{0};
    ubsm::AppContext appContext{};
    uint64_t flag{};
    mode_t mode;
    CreateShmWithProviderParam(std::string name, uint32_t socket, uint32_t numa, uint32_t port) noexcept
        : hostName(name),
          socketId(socket),
          numaId(numa),
          portId(port)
    {
    }
};

struct ImportShmParam {
    std::string name;
    ubsm::AppContext appContext{};
    off_t offset{0};
    size_t length{0};
    mode_t mode;
    int prot;
};

struct AttachShmResult {
    size_t shmSize;
    size_t unitSize;
    std::vector<uint64_t> memIds;
    uint64_t flag{};
    int oflag{};
};

/********** 控制面接口 ***********/
using UbseClientInitializeFunc = int32_t (*)(const char *ubse_uds_path);
using UbseClientFinalizeFunc = void (*)();
using UbseNodeListFunc = int32_t (*)(ubs_topo_node_t **node_list, uint32_t *node_cnt);
using UbseNodeLocalGetFunc = int32_t (*)(ubs_topo_node_t *node);
using UbseTopLinkListFunc = int32_t (*)(ubs_topo_link_t **cpu_links, uint32_t *cpu_link_cnt);
using UbseNumaStatGetFunc = int32_t (*)(uint32_t slot_id, ubs_mem_numastat_t **numa_mems, uint32_t *numa_mem_cnt);
using UbseMemFdCreateFunc = int32_t (*)(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                        ubs_mem_distance_t distance, ubs_mem_fd_desc_t *fd_desc);
using UbseMemFdCreateWithLenderFunc = int32_t (*)(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                                  const ubs_mem_lender_t *lender, uint32_t lender_cnt,
                                                  ubs_mem_fd_desc_t *fd_desc);
using UbseMemFdCreateWithCandidateFunc = int32_t (*)(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner,
                                                     mode_t mode, const uint32_t *slot_ids, uint32_t slot_cnt,
                                                     ubs_mem_fd_desc_t *fd_desc);
using UbseMemFdPermissionFunc = int32_t (*)(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode);
using UbseMemFdGetFunc = int32_t (*)(const char *name, ubs_mem_fd_desc_t *fd_desc);
using UbseMemFdListFunc = int32_t (*)(ubs_mem_fd_desc_t **fd_descs, uint32_t *fd_desc_cnt);
using UbseMemFdDeleteFunc = int32_t (*)(const char *name);

using UbseMemNumaCreateFunc = int32_t (*)(const char *name, uint64_t size, ubs_mem_distance_t distance,
                                          ubs_mem_numa_desc_t *numa_desc);
using UbseMemNumaCreateWithLenderFunc = int32_t (*)(const char *name, const ubs_mem_lender_t *lender,
                                                    uint32_t lender_cnt, ubs_mem_numa_desc_t *numa_desc);
using UbseMemNumaCreateWithCandidateFunc = int32_t (*)(const char *name, uint64_t size, const uint32_t *slot_ids,
                                                       uint32_t slot_cnt, ubs_mem_numa_desc_t *numa_desc);
using UbseMemNumaGetFunc = int32_t (*)(const char *name, ubs_mem_numa_desc_t *numa_desc);
using UbseMemNumaListFunc = int32_t (*)(ubs_mem_numa_desc_t **numa_descs, uint32_t *numa_desc_cnt);
using UbseMemNumaDeleteFunc = int32_t (*)(const char *name);
using UbseMemShmCreateFunc = int32_t (*)(const char *name, uint64_t size, uint8_t usr_info[UBS_MEM_MAX_USR_INFO_LEN],
                                         uint64_t flag, const ubs_mem_nodes_t *region, const ubs_mem_nodes_t *provier);
using UbseMemShmCreateWithLenderFunc = int32_t (*)(const char *name, uint8_t usr_info[UBS_MEM_MAX_USR_INFO_LEN],
                                                   uint64_t flag, const ubs_mem_nodes_t *region,
                                                   const ubs_mem_lender_t *lender);
using UbseMemShmCreateWithAffinityFunc = int32_t (*)(const char *name, uint64_t size, uint32_t affinity_socket_id,
                                                     uint8_t usr_info[32], uint64_t flag, const ubs_mem_nodes_t *region,
                                                     const ubs_mem_nodes_t *provider);
using UbseMemShmAttachFunc = int32_t (*)(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                         ubs_mem_shm_desc_t **shm_desc);
using UbseMemShmGetFunc = int32_t (*)(const char *name, ubs_mem_shm_desc_t **shm_desc);
using UbseMemShmListFunc = int32_t (*)(ubs_mem_shm_desc_t **shm_descs, uint32_t *shm_desc_cnt);
using UbseMemShmDetachFunc = int32_t (*)(const char *name);
using UbseMemShmDeleteFunc = int32_t (*)(const char *name);
using UbseLogCallBackRegisterFunc = void (*)(ubs_engine_log_handler log_handler);

class UbseMemAdapter {
public:
    static int Initialize();
    static void Destroy();

    static int32_t LookupRegionList(SHMRegions &regions);
    static int LeaseMalloc(const LeaseMallocParam &param, ock::ubsm::LeaseMallocResult &result);
    static int LeaseMallocWithLoc(const LeaseMallocWithLocParam &param, ock::ubsm::LeaseMallocResult &result);
    static int LeaseFree(const std::string &name, bool isNuma);
    static int ShmCreate(const CreateShmParam &param);
    static int ShmCreateWithProvider(const CreateShmWithProviderParam &param);
    static int ShmDelete(const std::string &name, const ubsm::AppContext &appContext);
    static int FdPermissionChange(const std::string &name, const ubsm::AppContext &appContext, mode_t mode);
    static int32_t DlopenLibUbse(const std::string &controllerPath);
    static int LookUpClusterStatistic(ock::ubsm::ubsmemClusterInfo &clusterInfo);
    static int GetTimeOutTaskStatus(const std::string &name, bool isLease, bool isNumaLease, ubs_mem_stage &status,
                                    bool isAttach);
    static int GetTimeOutShmTaskStatus(const std::string &name, ubs_mem_stage &status, bool isAttach, uint64_t &seqNo);
    static int ShmAttach(const std::string &name, const ubse_user_info_t &ubsUserInfo, AttachShmResult &result);
    static int ShmDetach(const std::string &name);
    static int ShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &nameList);
    static int ShmLookup(const std::string &name, ubsmem_shmem_info_t &shmInfo);
    static int ShmGetUserData(const std::string &name, ubse_user_info_t &ubsUserInfo);
    static int GetRegionInfo(const std::string &regionName, std::vector<uint32_t> &slotIds);
    static int32_t MemFdCreateWithCandidate(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner,
                                            mode_t mode, const uint32_t *slot_ids, uint32_t slot_cnt,
                                            ubs_mem_fd_desc_t *fd_desc);
    static int32_t MemFdCreateWithLender(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                         const ubs_mem_lender_t *lender, uint32_t lender_cnt,
                                         ubs_mem_fd_desc_t *fd_desc);

    static int GetLeaseMemoryStage(const std::string &name, bool isNuma, ubs_mem_stage &state);
    static int GetShareMemoryStage(const std::string &name, ubs_mem_stage &state);
    static int GetLocalNodeId(uint32_t &nid);
    static int EnsureGetLocalNodeId(uint32_t &nid);
    static uint64_t GenerateCreateSeqNo(uint32_t slotId);

private:
    UbseMemAdapter() = default;
    static void ResetLibUbseDl();
    static int32_t PopulateHostNameMap(SHMRegions &regions);
    static int32_t InitializeRegionList(SHMRegions &regions);
    static void FreeShmDesc(ubs_mem_shm_desc_t *shmDes);
    static uint32_t GetSocketIdWithNumaNode(int numaNode, uint32_t *socketId);
    static int ShmCreateWithAffinity(const CreateShmParam &param, const ubs_mem_nodes_t &region,
                                     const uint64_t ubseFlags, const uint8_t *usrInfo, uint64_t createSeqNo);
    static void UseLog(uint32_t level, const char *str);
    static int32_t CheckAndCopyRegionStatus(SHMRegions &staticRegions, SHMRegions &activeRegion);
    static int GetSlotIdFromHostName(const std::string &hostName, uint32_t *slotId);
    static int PrepareShmCreateWithProviderParams(const CreateShmWithProviderParam &param, uint8_t *usrInfo,
                                                  uint64_t *ubseFlags, uint32_t *slotId, uint64_t &seqNo);

private:
    static std::mutex gMutex;
    static bool initialized_;
    static std::unordered_map<std::string, uint32_t> hostnameMapping_;
    static std::atomic<uint32_t> nodeId_;

    static UbseClientInitializeFunc pUbseClientInitialize;
    static UbseClientFinalizeFunc pUbseClientFinalize;
    static UbseNodeListFunc pUbseNodeList;
    static UbseNodeLocalGetFunc pUbseNodeLocalGet;
    static UbseTopLinkListFunc pUbseTopLinkList;
    static UbseNumaStatGetFunc pUbseNumaStatGet;
    static UbseMemFdCreateFunc pUbseMemFdCreate;
    static UbseMemFdCreateWithLenderFunc pUbseMemFdCreateWithLender;
    static UbseMemFdCreateWithCandidateFunc pUbseMemFdCreateWithCandidate;
    static UbseMemFdPermissionFunc pUbseMemFdPermission;
    static UbseMemFdGetFunc pUbseMemFdGet;
    static UbseMemFdDeleteFunc pUbseMemFdDelete;
    static UbseMemNumaCreateFunc pUbseMemNumaCreate;
    static UbseMemNumaCreateWithLenderFunc pUbseMemNumaCreateWithLender;
    static UbseMemNumaCreateWithCandidateFunc pUbseMemNumaCreateWithCandidate;
    static UbseMemNumaGetFunc pUbseMemNumaGet;
    static UbseMemNumaDeleteFunc pUbseMemNumaDelete;
    static UbseMemShmCreateFunc pUbseMemShmCreate;
    static UbseMemShmCreateWithLenderFunc pUbseMemShmCreateWithLender;
    static UbseMemShmCreateWithAffinityFunc pUbseMemShmCreateWithAffinity;
    static UbseMemShmAttachFunc pUbseMemShmAttach;
    static UbseMemShmGetFunc pUbseMemShmGet;
    static UbseMemShmListFunc pUbseMemShmList;
    static UbseMemShmDetachFunc pUbseMemShmDetach;
    static UbseMemShmDeleteFunc pUbseMemShmDelete;
    static UbseLogCallBackRegisterFunc pUbseLogCallBackRegister;
};
} // namespace mxm
} // namespace ock

#ifdef __cplusplus
}
#endif

#endif // UBSE_MEM_ADAPTER_H
