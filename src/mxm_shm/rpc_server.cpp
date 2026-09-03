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
#include "rpc_server.h"
#include "log.h"
#include "rpc_server_engine.h"

#include "referable/dg_ref.h"

#include "mxm_rpc_server_interface.h"
#include "rpc_handler.h"

using namespace ock::mxmd;
using namespace ock::com::rpc;
using namespace ock::dagger;

namespace ock::rpc::service {
static std::map<uint32_t, MsgExecFuc> execMap;
static std::unordered_map<int, HcomServiceHandle> callBackMap;

void RpcServer::Handle(uint16_t opCode, const MsgBase *req, MsgBase *rsp, const MsgExecFuc &exePtr)
{
    if (exePtr == nullptr) {
        DBG_LOGERROR("Execution function is nullptr, opCode=" << opCode);
        return;
    }
    DBG_LOGDEBUG("server handle rpc msg, opCode=" << opCode);
    auto ret = exePtr(req, rsp);
    if (ret != 0) {
        DBG_LOGERROR("server handle msg failed, ret=" << ret << ", opCode=" << opCode);
    }
}

int32_t RpcServer::SendMsg(uint16_t opCode, MsgBase *req, MsgBase *rsp, const std::string &nodeId)
{
    return MxmComRpcServerSend(opCode, req, rsp, nodeId);
}

int32_t RpcServer::Connect(const RpcNode &nodeId)
{
    return MxmComRpcServerConnect(nodeId);
}

const std::unordered_map<int, HcomServiceHandle> &RpcServer::GetRpcCallbackTable()
{
    RpcServer::GetInstance().InitExecMap();
    callBackMap.clear();
    callBackMap[RPC_AGENT_QUERY_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_AGENT_QUERY_NODE_INFO, req, rsp, execMap.at(RPC_AGENT_QUERY_NODE_INFO));
    }};
    callBackMap[RPC_PING_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_PING_NODE_INFO, req, rsp, execMap.at(RPC_PING_NODE_INFO));
    }};
    callBackMap[RPC_JOIN_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_JOIN_NODE_INFO, req, rsp, execMap.at(RPC_JOIN_NODE_INFO));
    }};
    callBackMap[RPC_MASTER_ELECTED_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_MASTER_ELECTED_NODE_INFO, req, rsp, execMap.at(RPC_MASTER_ELECTED_NODE_INFO));
    }};
    callBackMap[RPC_SEND_ELECTED_MASTER_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_SEND_ELECTED_MASTER_INFO, req, rsp, execMap.at(RPC_SEND_ELECTED_MASTER_INFO));
    }};
    callBackMap[RPC_VOTE_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_VOTE_NODE_INFO, req, rsp, execMap.at(RPC_VOTE_NODE_INFO));
    }};
    callBackMap[RPC_BROADCAST_NODE_INFO] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_BROADCAST_NODE_INFO, req, rsp, execMap.at(RPC_BROADCAST_NODE_INFO));
    }};
    callBackMap[RPC_DLOCK_CLIENT_REINIT] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_DLOCK_CLIENT_REINIT, req, rsp, execMap.at(RPC_DLOCK_CLIENT_REINIT));
    }};
    callBackMap[RPC_LOCK] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_LOCK, req, rsp, execMap.at(RPC_LOCK));
    }};
    callBackMap[RPC_UNLOCK] = {[](const MsgBase *req, MsgBase *rsp) {
        GetInstance().MsgHandle(RPC_UNLOCK, req, rsp, execMap.at(RPC_UNLOCK));
    }};
    return callBackMap;
}

uint32_t RpcServer::InitializeRpcCallbackTable()
{
    auto map = RpcServer::GetInstance().GetRpcCallbackTable();
    for (auto &op : map) {
        MxmComEndpoint endpoint;
        endpoint.address = "mem_rpc";
        endpoint.serviceId = op.first;
        uint32_t hr = RpcServerEngine::GetInstance().RegisterOpcode(endpoint, op.second);
        if (BresultFail(hr)) {
            DBG_LOGERROR("RegisterOpcode failed, ret << " << hr);
            return hr;
        }
    }
    return HOK;
}

uint32_t RpcServer::InitExecMap()
{
    DBG_LOGINFO("start init exec map");
    execMap.emplace(RPC_AGENT_QUERY_NODE_INFO, MxmServerMsgHandle::QueryNodeInfo);
    execMap.emplace(RPC_PING_NODE_INFO, MxmServerMsgHandle::PingRequestInfo);
    execMap.emplace(RPC_JOIN_NODE_INFO, MxmServerMsgHandle::JoinRequestInfo);
    execMap.emplace(RPC_MASTER_ELECTED_NODE_INFO, MxmServerMsgHandle::MasterElectedInfo);
    execMap.emplace(RPC_SEND_ELECTED_MASTER_INFO, MxmServerMsgHandle::SendTransElectedInfo);
    execMap.emplace(RPC_VOTE_NODE_INFO, MxmServerMsgHandle::VoteRequestInfo);
    execMap.emplace(RPC_BROADCAST_NODE_INFO, MxmServerMsgHandle::BroadCastRequestInfo);
    execMap.emplace(RPC_DLOCK_CLIENT_REINIT, MxmServerMsgHandle::DLockClientReinit);
    execMap.emplace(RPC_LOCK, MxmServerMsgHandle::HandleMemLock);
    execMap.emplace(RPC_UNLOCK, MxmServerMsgHandle::HandleMemUnLock);
    for (auto &iter : execMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("RPC init exe map error, no func");
            return MXM_ERR_REGISTER_HANDLER_NULL;
        }
    }
    DBG_LOGINFO("started init exec map");
    return HOK;
}

uint32_t RpcServer::RackMemConBaseInitialize()
{
    RpcServerEngine::GetInstance().Initialize();
    auto hr = RpcServer::GetInstance().InitializeRpcCallbackTable();
    if (BresultFail(hr)) {
        DBG_LOGERROR("RackMemConInitializeRpcCallbackTable failed!");
        return hr;
    }

    DBG_LOGINFO("Mxm lease hpc Server rpc is initialized");
    return HOK;
}

uint32_t RpcServer::Stop()
{
    return HOK;
}
} // namespace ock::rpc::service