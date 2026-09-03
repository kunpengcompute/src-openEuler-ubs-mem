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

#ifndef MXM_IPC_SERVER_H
#define MXM_IPC_SERVER_H

#include "mxm_com_base.h"

namespace ock::com::ipc {
class MxmIpcServer : public MxmComBase {
public:
    MxmIpcServer(std::string udsPath, std::string name, std::string nodeId, uint16_t udsMode)
        : MxmComBase(nodeId, name),
          udsPath(std::move(udsPath)),
          udsMode(udsMode)
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

private:
    std::string udsPath; // 监听路径
    uint16_t udsMode;    // UDS 文件权限，默认推荐使用 600
};
} // namespace ock::com::ipc
#endif // MXM_IPC_SERVER_H
