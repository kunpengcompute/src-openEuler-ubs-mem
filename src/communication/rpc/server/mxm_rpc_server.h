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

#ifndef RPC_MXM_RPC_SERVER_H
#define RPC_MXM_RPC_SERVER_H

#include <utility>
#include "mxm_com_base.h"

namespace ock::com::rpc {
class MxmRpcServer : public ock::com::MxmComBase {
public:
    MxmRpcServer(std::string name, RpcNode ipAndPort)
        : MxmComBase(ipAndPort.name, name),
          ipAndPort(std::move(ipAndPort))
    {
    }

    /* *
   * @brief 启动Server
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT Start() override;

    /* *
   * @brief 停止Server
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    void Stop() override;

    /* *
   * @brief 与Server建连
   * @param remoteNodeId [in] 对端nodeId
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT Connect(const RpcNode &remoteNodeId) override;

private:
    RpcNode ipAndPort;
};
} // namespace ock::com::rpc
#endif // RPC_MXM_RPC_SERVER_H
