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

#ifndef RACK_MEMORYFABRIC_ERRORS_H
#define RACK_MEMORYFABRIC_ERRORS_H

#include <regex.h>
#include <securec.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <list>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "dg_common.h"

#include "lock/dg_lock.h"
#include "rack_mem_constants.h"
#include "referable/dg_ref.h"
#include "strings/dg_str_util.h"
#include "thread_pool/dg_execution_service.h"
#include "time/dg_monotonic.h"
#include "util/defines.h"

namespace ock::mxmd {

inline bool BresultFail(uint32_t res)
{
    return res != HOK;
}

} // namespace ock::mxmd

namespace ock {
namespace common {
typedef enum {
    MXM_OK = 0,
    // common error
    MXM_ERR_MEMLIB = 100,
    MXM_ERR_NULLPTR,
    MXM_ERR_MALLOC_FAIL,
    MXM_ERR_MEMORY,
    MXM_ERR_PARAM_INVALID,
    MXM_ERR_MMAP_VA_FAILED,
    MXM_ERR_CHECK_RESOURCE,
    MXM_ERR_DAEMON_NO_FEATURE_ENABLED,

    // net error
    MXM_ERR_IPC_INIT_CALL = 200,
    MXM_ERR_IPC_SYNC_CALL,
    MXM_ERR_IPC_HCOM_INNER_SYNC_CALL,
    MXM_ERR_IPC_CRC_CHECK_ERROR,
    MXM_ERR_IPC_SERIALIZE_DESERIALIZE_ERROR,
    MXM_ERR_RPC_QUERY_ERROR,
    MXM_ERR_REGISTER_OP_CODE,
    MXM_ERR_REGISTER_HANDLER_NULL,

    // cc lock
    MXM_ERR_LOCK_NOT_READY = 300,
    MXM_ERR_LOCK_ALREADY_LOCKED,
    MXM_ERR_LOCK_NOT_FOUND,
    MXM_ERR_DLOCK_LIB,
    MXM_ERR_DLOCK_INNER,

    // recorde store
    MXM_ERR_RECORD_DELETE = 400,
    MXM_ERR_RECORD_ADD,
    MXM_ERR_RECORD_CHANGE_STATE,
    // region
    MXM_ERR_REGION_NOT_FOUND = 500,
    MXM_ERR_REGION_PARAM_INVALID,
    MXM_ERR_REGION_EXIST,
    MXM_ERR_REGION_NOT_EXIST,
    MXM_ERR_REGION_NO_NEEDED,

    // shm obj
    MXM_ERR_SHM_NOT_FOUND = 600,
    MXM_ERR_SHM_ALREADY_EXIST,
    MXM_ERR_SHM_PERMISSION_DENIED,
    MXM_ERR_SHM_IN_USING, // 仅用作删除共享时，引用计数不为0的错误返回
    MXM_ERR_SHM_OBMM_OPEN,
    MXM_ERR_SHM_INVALID_NUMA,
    MXM_ERR_SHM_NOT_EXIST,

    MXM_ERR_NAME_BUSY = 650, // this name is existed in DelayRemoveKey,include shm and lease
    // lease obj
    MXM_ERR_LEASE_NOT_FOUND = 700,
    MXM_ERR_LEASE_NOT_EXIST,

    // ubse
    MXM_ERR_UBSE_INNER = 800,
    MXM_ERR_UBSE_LIB,
    MXM_ERR_UBSE_NOT_ATTACH,
    MXM_ERR_UBSE_NOT_SUPPORTED,

    // obmm
    MXM_ERR_OBMM_INNER = 900,

    // 任务需要重试
    MXM_TIME_OUT_TASK_NEED_RETRY = 950,

    MXM_ERR_UNKNOWN = 1000,
} UbsmdRetErr;
} // namespace common
} // namespace ock

#endif