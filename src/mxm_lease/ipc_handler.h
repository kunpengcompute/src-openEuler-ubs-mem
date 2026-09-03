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

#ifndef MXM_LEASE_SERVER_IPC_HANDLER_H
#define MXM_LEASE_SERVER_IPC_HANDLER_H

#include <unistd.h>
#include "log.h"
#include "mxm_ipc_server_interface.h"
#include "mxm_msg.h"

namespace ock::lease::service {
using namespace ock::mxmd;
using namespace ock::com;
using namespace ock::common;

class MxmServerMsgHandle {
public:
    static int AppMallocMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int AppMallocMemoryWithLoc(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int AppFreeMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int AppQueryClusterInfo(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int AppForceFreeCachedMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int AppQueryCachedMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int AppCheckMemoryLease(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
};
} // namespace ock::lease::service

#endif