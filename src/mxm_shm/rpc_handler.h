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
#ifndef RPC_HANDLER_H
#define RPC_HANDLER_H

#include <unistd.h>
#include "log.h"
#include "mxm_msg.h"
#include "mxm_rpc_server_interface.h"

namespace ock::rpc::service {
using namespace ock::mxmd;

class MxmServerMsgHandle {
public:
    static int QueryNodeInfo(const MsgBase *req, MsgBase *rsp);
    // 处理 Ping 请求
    static int PingRequestInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 Join 请求
    static int JoinRequestInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 Vote 请求
    static int VoteRequestInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 Send Master Elected 通知
    static int SendTransElectedInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 Master Elected 通知
    static int MasterElectedInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 Broadcast 请求
    static int BroadCastRequestInfo(const MsgBase *req, MsgBase *rsp);

    // 处理 DLock reinit请求
    static int DLockClientReinit(const MsgBase *req, MsgBase *rsp);

    // 处理lock请求
    static int HandleMemLock(const MsgBase *req, MsgBase *rsp);

    // 处理unlock请求
    static int HandleMemUnLock(const MsgBase *req, MsgBase *rsp);
};
} // namespace ock::rpc::service

#endif