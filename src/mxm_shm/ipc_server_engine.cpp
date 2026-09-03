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
#include "ipc_server_engine.h"
#include "log.h"

using namespace ock::common;
using namespace ock::com;
using namespace ock::com::ipc;
using namespace ock::mxmd;
namespace ock::share::service {

uint32_t IpcServerEngine::RegisterOpcode(MxmComEndpoint opcode, const HcomServiceHandle &handle)
{
    opcode.moduleId = 1;
    auto ret = MxmRegIpcService(
        opcode, [opcode, handle](const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *resp) -> void {
            handle.func(udsInfo, req, resp);
            return;
        });
    if (ret != 0) {
        return MXM_ERR_REGISTER_OP_CODE;
    }
    return HOK;
}
} // namespace ock::share::service