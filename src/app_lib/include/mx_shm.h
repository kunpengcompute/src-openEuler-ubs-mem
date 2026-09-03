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
#ifndef RACK_MEM_SHM_H
#define RACK_MEM_SHM_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "ubs_mem_def.h"
#include "mx_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RackMemShmRegionType {
    ALL2ALL_SHARE = 0, /* *SHM域类型为域内任何节点提供内存都可以被域内所有节点共享访问 */
    ONE2ALL_SHARE,     /* *SHM域类型为域内单一节点作为提供方都可以被域内所有节点共享访问 */
    INCLUDE_ALL_TYPE,  /* *请求所有类型 */
} ShmRegionType;

typedef enum RackMemShmCacheOptType { RACK_FLUSH = 0, RACK_INVALID } ShmCacheOpt;

typedef enum RackMemShmOwnStatus {
    UNACCESS = 0,
    WRITE,
    READ,
    READWRITE,
    OWNBUFF,
} ShmOwnStatus;

typedef struct TagRackMemSHMRegionDesc {
    PerfLevel perfLevel;
    ShmRegionType type;
    int num;
    bool affinity[MEM_TOPOLOGY_MAX_HOSTS];
    char nodeId[MEM_TOPOLOGY_MAX_HOSTS][MEM_MAX_ID_LENGTH];
    char hostName[MEM_TOPOLOGY_MAX_HOSTS]
                 [MAX_HOST_NAME_DESC_LENGTH]; // nodeId对应的hostname，最长MAX_HOST_NAME_DESC_LENGTH
} SHMRegionDesc;

typedef struct TagRackMemSHMRegions {
    int num;
    SHMRegionDesc region[MAX_REGIONS_NUM];
} SHMRegions;

typedef struct TagRackMemSHMRegionInfo {
    int num;
    int64_t info[0];
} SHMRegionInfo;

int RpcQueryNodeInfo();
#ifdef __cplusplus
}
#endif

#endif