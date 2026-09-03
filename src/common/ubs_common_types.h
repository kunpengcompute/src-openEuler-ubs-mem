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

#ifndef SIMPLE_SAMPLES_UBS_COMMON_TYPES_H
#define SIMPLE_SAMPLES_UBS_COMMON_TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include "ubs_mem_def.h"

namespace ock {
namespace ubsm {

enum class RecordState : int16_t {
    PRE_ADD,
    FINISH,
    PRE_DEL,
    DELETED,
};
struct AppContext {
    uint32_t pid;
    uint32_t uid;
    uint32_t gid;

    AppContext() noexcept : AppContext{0, 0, 0} {}
    AppContext(uint32_t p, uint32_t u, uint32_t g) noexcept : pid{p}, uid{u}, gid{g} {}

    void Fill(uint32_t p, uint32_t u, uint32_t g) noexcept
    {
        pid = p;
        uid = u;
        gid = g;
    }
    std::string GetString() const
    {
        return std::to_string(pid) + ":" + std::to_string(uid) + ":" + std::to_string(gid);
    }
};

struct CreateRegionInput {
    std::string name;
    uint64_t size{0};
    std::vector<uint32_t> nodeIds;
    std::vector<bool> affinity;
    CreateRegionInput() noexcept = default;
    CreateRegionInput(std::string nm, uint64_t sz, std::vector<uint32_t> ns, std::vector<bool> as) noexcept
        : name{std::move(nm)},
          size{sz},
          nodeIds{std::move(ns)},
          affinity{std::move(as)}
    {
    }
};

struct LeaseMallocInput {
    bool fdMode{true}; // true means fd, false means numa
    std::string name;
    size_t size{0};
    uint8_t distance{0};
    AppContext appContext{};
    RecordState state{};
};

struct LeaseMallocResult {
    std::vector<uint64_t> memIds;
    int64_t numaId{-1L};
    size_t unitSize{0};
    uint32_t slotId; // 记录借出内存节点
};

struct ShareMemImportInput {
    std::string name;
    uint64_t size{0};
    pid_t pid{-1};
};

struct ShareMemImportResult {
    std::vector<uint64_t> memIds;
    size_t unitSize{0};
    uint64_t createFlags{}; // create parameters
    int openFlag{};         // O_RDWR
    uid_t ownerUserId;
    gid_t ownerGroupId;
};

struct ShareMemImportInfo {
    std::string name;
    uint64_t size{0};
    AppContext appContext{};
    std::vector<uint64_t> memIds;
    size_t unitSize{0};
    uint64_t flag{};
    int oflag{};
    RecordState state{};
};

typedef struct {
    uint32_t slot_id;        // 节点唯一标识, 采用slotid, 与lcne保持一致
    uint32_t socket_id;      // socket id
    uint32_t numa_id;        // 节点中的numa id
    uint32_t mem_lend_ratio; // 池化内存借出比例上限
    uint64_t mem_total;      // 内存总量, 单位字节
    uint64_t mem_free;       // 内存空闲量, 单位字节
    uint64_t mem_borrow;     // 借用的内存，单位字节
    uint64_t mem_lend;       // 借出的内存，单位字节
    uint8_t resv[MAX_NUMA_RESV_LEN];
} ubsmemNumaMem;

typedef struct {
    char host_name[MAX_HOST_NAME_DESC_LENGTH];
    int numa_num;
    ubsmemNumaMem numa[MAX_NUMA_NUM];
} ubsmemHostFnfo;

typedef struct {
    int host_num; // 集群可用节点数量
    ubsmemHostFnfo host[MAX_HOST_NUM];
} ubsmemClusterInfo;

} // namespace ubsm
} // namespace ock

#endif // SIMPLE_SAMPLES_UBS_COMMON_TYPES_H
