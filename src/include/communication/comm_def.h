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

#ifndef COMM_DEF_H
#define COMM_DEF_H
#include <functional>
#include "mxm_msg.h"

namespace ock::com {
using namespace ock::mxmd;

using MxmByteBufferFreeFunc = std::function<void(uint8_t *data)>;

struct MxmByteBuffer {
    uint8_t *data = nullptr;        // 数据指针
    size_t len = 0;                 // 数据长度
    MxmByteBufferFreeFunc freeFunc; // 非空代表接收方需要释放内存；空代表接收方不需要释放内存
};

struct MxmComEndpoint {
    uint16_t moduleId;   // 模块Id
    uint32_t serviceId;  // server端的serviceId
    std::string address; // 通信地址
};

struct MxmComUdsInfo {
    uint32_t pid = 0; // 进程Id
    uint32_t uid = 0; // 用户Id
    uint32_t gid = 0; // 用户组Id
};

using MxmPostReconnectHandler = std::function<int32_t()>;

using MXMLinkEventHandler = std::function<void(const int32_t pid)>;

using MxmComServiceHandler = std::function<void(const MsgBase *req, MsgBase *resp)>;

using MxmComIpcServiceHandler = std::function<void(const MxmComUdsInfo &udsInfo, const MsgBase *req, MsgBase *resp)>;
} // namespace ock::com

#endif // COMM_DEF_H