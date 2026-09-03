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

#ifndef OCK_HPCIPCSERVER_SHM_H
#define OCK_HPCIPCSERVER_SHM_H
#include <unordered_set>

#include "mxm_ipc_server_interface.h"
#include "mxm_msg.h"

#include "ipc_server_engine.h"
#include "log.h"

using namespace ock::com;
using namespace ock::common;
using namespace ock::mxmd;

namespace ock::share::service {
using MsgExecFuc = std::function<int(const MsgBase *, MsgBase *, const MxmComUdsInfo &)>;
class IpcServer {
    DAGGER_DEFINE_REF_COUNT_FUNCTIONS
public:
    uint32_t InitExecMap();
    const std::unordered_map<int, HcomServiceHandle> &GetIpcCallbackTable();
    static uint32_t RackMemConBaseInitialize();
    uint32_t Stop();

    void Handle(const MxmComUdsInfo &udsInfo, uint16_t opCode, const MsgBase *req, MsgBase *rsp,
                const MsgExecFuc &exePtr);

    void MsgHandle(const MxmComUdsInfo &udsInfo, uint16_t opCode, const MsgBase *req, MsgBase *rsp, MsgExecFuc exePtr)
    {
        Handle(udsInfo, opCode, req, rsp, exePtr);
    }
    static IpcServer &GetInstance()
    {
        static IpcServer instance;
        return instance;
    }
    IpcServer(const IpcServer &other) = delete;
    IpcServer(IpcServer &&other) = delete;
    IpcServer &operator=(const IpcServer &other) = delete;
    IpcServer &operator=(IpcServer &&other) noexcept = delete;
    DAGGER_DEFINE_REF_COUNT_VARIABLE;

private:
    uint32_t InitializeIpcCallbackTable();
    void GetIpcCallbackTablePartition();
    IpcServer() = default;
};
} // namespace ock::share::service
#endif