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

#ifndef MXM_IPC_CLIENT_H
#define MXM_IPC_CLIENT_H

#include <utility>
#include "mxm_com_base.h"

namespace ock::com::ipc {
class MxmIpcClient : public MxmComBase {
public:
    explicit MxmIpcClient(std::string udsPath, const std::string &name, const std::string &nodeId)
        : MxmComBase(nodeId, name),
          udsPath(std::move(udsPath))
    {
    }

    /* *
   * @brief 启动Client
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT Start() override;

    /* *
   * @brief 与Server建连
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT Connect() override;

    /* *
   * @brief 停止Client
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    void Stop() override;

    int SetPostReconnectHandler(MxmComPostReconnectHandler handler);

private:
    std::string udsPath;
};
} // namespace ock::com::ipc
#endif // MXM_IPC_CLIENT_H
