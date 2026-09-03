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
#include "rpc_handler.h"
#include <securec.h>
#include <sys/types.h>
#include <unistd.h>
#include "dlock_utils/ubsm_lock.h"
#include "rack_mem_functions.h"
#include "rpc_config.h"
#include "zen_discovery.h"

namespace ock::rpc::service {
using namespace ock::mxmd;
using namespace ock::rpc;
using namespace ock::common;

int MxmServerMsgHandle::QueryNodeInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = MXM_OK;
    response->name_ = NetRpcConfig::GetInstance().GetLocalNode().name;
    DBG_LOGINFO("Query node info success, local node=" << response->name_);
    return MXM_OK;
}

int MxmServerMsgHandle::DLockClientReinit(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("RPC_DLockClientReinit: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const DLockClientReinitRequest *>(req);
    auto response = dynamic_cast<DLockClientReinitResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("RPC_DLockClientReinit: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto &cfg = dlock_utils::DLockContext::Instance().GetConfig();
    cfg.serverIp = request->serverIp_;
    auto ret = dlock_utils::UbsmLock::Instance().Reinit();
    if (ret != 0) {
        DBG_LOGERROR("DLock Client Reinit failed, retCode: " << ret);
        dlock_utils::UbsmLock::Instance().DeinitTlsConfig();
        response->dLockCode_ = ret;
        response->errCode_ = ret;
        return ret;
    }
    response->dLockCode_ = 0;
    response->errCode_ = UBSM_OK;
    DBG_LOGINFO("DLock Client Reinit success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::HandleMemLock(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("RPC_GetDLockInitInfo: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const LockRequest *>(req);
    auto response = dynamic_cast<DLockResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("RPC_DLockClientReinit: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("Handle RPC MemLock, name=" << request->memName_ << ", isExclusive=" << request->isExclusive_
                                            << ", pid=" << request->pid_ << ", uid=" << request->uid_
                                            << ", gid=" << request->gid_);
    dlock_utils::LockUdsInfo udsInfo;
    udsInfo.pid = request->pid_;
    udsInfo.uid = request->uid_;
    udsInfo.gid = request->gid_;
    udsInfo.validTime = 0;
    response->dLockCode_ = dlock_utils::UbsmLock::Instance().Lock(request->memName_, request->isExclusive_, udsInfo);
    DBG_LOGINFO("Handle RPC MemLock end, name=" << request->memName_ << ", isExclusive=" << request->isExclusive_);
    return UBSM_OK;
}

int MxmServerMsgHandle::HandleMemUnLock(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("RPC_GetDLockInitInfo: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const UnLockRequest *>(req);
    auto response = dynamic_cast<DLockResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("RPC_DLockClientReinit: invalid param.");
        return MXM_ERR_NULLPTR;
    }
    dlock_utils::LockUdsInfo udsInfo;
    udsInfo.pid = request->pid_;
    udsInfo.uid = request->uid_;
    udsInfo.gid = request->gid_;
    udsInfo.validTime = 0;
    response->dLockCode_ = dlock_utils::UbsmLock::Instance().Unlock(request->memName_, udsInfo);
    DBG_LOGINFO("Send RPC MemUnLock success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::PingRequestInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    const PingRequest *request = dynamic_cast<const PingRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }
    zenDiscovery->HandlePingRequest(request->nodeId_);

    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }
    response->errCode_ = 0;
    response->name_ = NetRpcConfig::GetInstance().GetLocalNode().name;

    return 0;
}

int MxmServerMsgHandle::JoinRequestInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    const PingRequest *request = dynamic_cast<const PingRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }

    zenDiscovery->HandleJoinRequest(request->nodeId_);

    auto response = dynamic_cast<RpcJoinInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = 0;
    response->nodetype_ = static_cast<int32_t>(zenDiscovery->type_);

    return 0;
}

int MxmServerMsgHandle::VoteRequestInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }

    const VoteRequest *request = dynamic_cast<const VoteRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }

    bool granted = zenDiscovery->HandleVoteRequest(request->nodeId_, request->masterNode_, request->term_);

    auto response = dynamic_cast<RpcVoteInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = 0;
    response->name_ = NetRpcConfig::GetInstance().GetLocalNode().name;
    response->isGranted_ = granted;
    return 0;
}

int MxmServerMsgHandle::SendTransElectedInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }

    const TransElectedRequest *request = dynamic_cast<const TransElectedRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }

    zenDiscovery->HandleSendTransElected(request->nodeId_, request->nodes_, request->term_);

    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = 0;
    response->name_ = "Transfer Master Elected Notification Processed";

    return 0;
}

int MxmServerMsgHandle::MasterElectedInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }

    const VoteRequest *request = dynamic_cast<const VoteRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }

    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = 0;
    response->name_ = "Master Elected Notification Processed";

    return 0;
}

int MxmServerMsgHandle::BroadCastRequestInfo(const MsgBase *req, MsgBase *rsp)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }

    ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_NULLPTR;
    }

    const BroadcastRequest *request = dynamic_cast<const BroadcastRequest *>(req);
    if (request == nullptr) {
        DBG_LOGERROR("invalid request type.");
        return MXM_ERR_NULLPTR;
    }
    zenDiscovery->HandleBroadCastRequest(request->nodeId_, request->nodes_, request->isSeverInited_);

    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid response type.");
        return MXM_ERR_NULLPTR;
    }

    response->errCode_ = 0;
    response->name_ = "Broadcast Request Processed";

    return 0;
}

} // namespace ock::rpc::service