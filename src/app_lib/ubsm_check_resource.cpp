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

#include "ubsm_check_resource.h"

#include "ipc_command.h"
#include "shm_ipc_command.h"

namespace ock::ubsm {

int UbsmCheckResource::UbsmCheckResourceHandler()
{
    auto leaseRes = mxmd::IpcCommand::AppCheckLeaseMemoryResource();
    if (leaseRes != 0) {
        DBG_LOGERROR("AppCheckLeaseMemoryResource failed. ret=" << leaseRes);
    }

    auto shmRes = mxmd::ShmIpcCommand::AppCheckShareMemoryResource();
    if (shmRes != 0) {
        DBG_LOGERROR("AppCheckShareMemoryResource failed. ret=" << shmRes);
    }

    return ((leaseRes != 0) || (shmRes != 0)) ? MXM_ERR_CHECK_RESOURCE : 0;
}

} // namespace ock::ubsm