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
#ifndef IOAK_CACHE_IPC_SERVER_LEASE_H
#define IOAK_CACHE_IPC_SERVER_LEASE_H

#include <utility>
#include "mxm_ipc_server_interface.h"

using namespace ock::common;
using namespace ock::com;
using namespace ock::mxmd;

namespace ock::lease::service {
struct HcomServiceHandle {
    void (*func)(const MxmComUdsInfo &udsInfo, const MsgBase *, MsgBase *);
};
class IpcServerEngine final {
public:
    void Initialize() {}

    uint32_t RegisterOpcode(MxmComEndpoint opcode, const HcomServiceHandle &handle);

    static IpcServerEngine &GetInstance()
    {
        static IpcServerEngine instance;
        return instance;
    }

private:
    IpcServerEngine() = default;
};
} // namespace ock::lease::service
#endif // IOAK_CACHE_IPC_SERVER_LEASE_H