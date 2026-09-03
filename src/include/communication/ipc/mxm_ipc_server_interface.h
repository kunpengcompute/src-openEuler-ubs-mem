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

#ifndef MXM_IPC_SERVER_INTERFACE_H
#define MXM_IPC_SERVER_INTERFACE_H

#include <comm_def.h>
#include "comm_def.h"
#include "mxm_msg.h"

namespace ock::com::ipc {
using namespace ock::mxmd;
/**
 * @brief 启动Ipc客户端
 * @return 成功返回0, 失败返回错误码
 */
int MxmComStartIpcServer();

/**
 * @brief 停止Ipc客户端
 */
void MxmComStopIpcServer();

void MXMSetLinkEventHandler(const MXMLinkEventHandler &handler);

uint32_t MxmRegIpcService(const MxmComEndpoint &endpoint, const MxmComIpcServiceHandler &handler);

} // namespace ock::com::ipc

#endif // MXM_IPC_SERVER_INTERFACE_H
