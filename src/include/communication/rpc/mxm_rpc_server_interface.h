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
#ifndef MXM_RPC_SERVER_INTERFACE_H
#define MXM_RPC_SERVER_INTERFACE_H

#include <functional>
#include "comm_def.h"
#include "mxm_msg.h"

namespace ock::com::rpc {
using namespace ock::mxmd;
/**
 * @brief 启动Rpc客户端
 * @return 成功返回0, 失败返回错误码
 */
int MxmComStartRpcServer();

/**
 * @brief 停止Rpc客户端
 */
void MxmComStopRpcServer();

uint32_t MxmRegRpcService(const MxmComEndpoint &endpoint, const MxmComServiceHandler &handler);

int MxmComRpcServerSend(uint16_t opCode, MsgBase *request, MsgBase *response, const std::string &nodeId);

int MxmComRpcServerConnect(const ock::rpc::RpcNode &nodeId);
} // namespace ock::com::rpc

#endif // MXM_RPC_SERVER_INTERFACE_H
