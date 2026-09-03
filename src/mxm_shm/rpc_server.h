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
#ifndef MXM_SHM_RPC_SERVER_H
#define MXM_SHM_RPC_SERVER_H
#include <unordered_set>

#include "mxm_msg.h"
#include "mxm_rpc_server_interface.h"

#include "log.h"
#include "rpc_server_engine.h"

using namespace ock::com::rpc;
using namespace ock::common;
using namespace ock::mxmd;

namespace ock::rpc::service {
using MsgExecFuc = std::function<int(const MsgBase *, MsgBase *)>;
class RpcServer {
    DAGGER_DEFINE_REF_COUNT_FUNCTIONS
public:
    uint32_t InitExecMap();
    const std::unordered_map<int, HcomServiceHandle> &GetRpcCallbackTable();
    static uint32_t RackMemConBaseInitialize();
    uint32_t Stop();

    void Handle(uint16_t opCode, const MsgBase *req, MsgBase *rsp, const MsgExecFuc &exePtr);

    void MsgHandle(uint16_t opCode, const MsgBase *req, MsgBase *rsp, MsgExecFuc exePtr)
    {
        Handle(opCode, req, rsp, exePtr);
    }
    int32_t SendMsg(uint16_t opCode, MsgBase *req, MsgBase *rsp, const std::string &nodeId);
    int32_t Connect(const RpcNode &nodeId);
    static RpcServer &GetInstance()
    {
        static RpcServer instance;
        return instance;
    }
    RpcServer(const RpcServer &other) = delete;
    RpcServer(RpcServer &&other) = delete;
    RpcServer &operator=(const RpcServer &other) = delete;
    RpcServer &operator=(RpcServer &&other) noexcept = delete;
    DAGGER_DEFINE_REF_COUNT_VARIABLE;

private:
    uint32_t InitializeRpcCallbackTable();
    RpcServer() = default;
};
} // namespace ock::rpc::service
#endif