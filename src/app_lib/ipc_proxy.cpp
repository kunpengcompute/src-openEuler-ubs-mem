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

#include "ipc_proxy.h"
#include <dlfcn.h>
#include "ubsm_check_resource.h"

using namespace ock::dagger;

namespace ock::mxmd {
using namespace ock::common;
uint32_t IpcProxy::Initialize()
{
    const auto start = Monotonic::TimeUs();
    auto ret = MxmComStartIpcClient();
    if (ret != 0) {
        const auto end = Monotonic::TimeUs();
        DBG_LOGERROR("Failed to init ipc, ret=" << ret << ", cost time=" << (end - start) << "us");
        return MXM_ERR_IPC_INIT_CALL;
    }
    ret = MxmSetPostReconnectHandler(ock::ubsm::UbsmCheckResource::UbsmCheckResourceHandler);
    if (ret != 0) {
        DBG_LOGERROR("MxmSetPostReconnectHandler failed. ret=" << ret);
        MxmComStopIpcClient();
        return MXM_ERR_IPC_INIT_CALL;
    }
    return UBSM_OK;
}

uint32_t IpcProxy::Destroy()
{
    MxmComStopIpcClient();
    return UBSM_OK;
}

uint32_t IpcProxy::SyncCall(int opcode, MsgBase *request, MsgBase *response)
{
    auto ret = MxmComIpcClientSend(opcode, request, response);
    if (ret != 0) {
        return MXM_ERR_IPC_HCOM_INNER_SYNC_CALL;
    }
    return UBSM_OK;
}
} // namespace ock::mxmd