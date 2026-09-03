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
#include "ubsm_lock_event.h"
#include "mxm_msg.h"
#include "rpc_config.h"
#include "rpc_server.h"
#include "ubs_common_config.h"
#include "ubsm_lock.h"

using namespace ock::dlock_utils;
using RpcServer = ock::rpc::service::RpcServer;
using ZenDiscovery = ock::zendiscovery::ZenDiscovery;
using ZenElectionEventType = ock::zendiscovery::ElectionModule::ZenElectionEventType;

void UbsmLockEvent::HandleElectionEvent(ZenElectionEventType eventType, const std::string &masterId,
                                        const std::string &clientId)
{
    switch (eventType) {
        case ZenElectionEventType::ELECTION_STARTED:
            DBG_LOGINFO("Election started event received");
            DoPreElection();
            break;
        case ZenElectionEventType::VOTING_COMPLETED:
            DBG_LOGINFO("Voting completed");
            break;
        case ZenElectionEventType::HAVE_JOINED:
            DBG_LOGINFO("master try to reinit dlock.");
            OnDLockServerRecovery(masterId, clientId);
            break;
        case ZenElectionEventType::MASTER_ELECTED:
            DBG_LOGINFO("Master elected event received, masterId: " << masterId);
            OnMasterElected(masterId);
            break;
        case ZenElectionEventType::NODE_DEMOTED:
            DBG_LOGINFO("node demoted event received, former masterId: " << masterId);
            OnDLockDemoted();
            break;
        case ZenElectionEventType::BECOME_VOTE_NODE:
            DBG_LOGDEBUG("Broadcast event received from masterId: " << masterId);
            OnDLockClientInit(masterId);
            break;
        case ZenElectionEventType::ELECTION_FAILED:
            DBG_LOGWARN("Election failed");
            break;
        default:
            break;
    }
}

void UbsmLockEvent::DoPreElection()
{
    // 选主前关闭dlock心跳，暂未使用dlock 心跳
    DBG_LOGDEBUG("In DoPreElection.");
}

void UbsmLockEvent::OnMasterElected(const std::string &masterId)
{
    auto &rpcConfig = rpc::NetRpcConfig::GetInstance();
    auto localNode = rpcConfig.GetLocalNode();
    DBG_LOGDEBUG("Local node: " << localNode.name << ", Elected master: " << masterId);
    if (localNode.name != masterId) {
        DBG_LOGINFO("current node is not master, node ID: " << localNode.name);
        return;
    }
    DBG_LOGINFO("Current node elected as master, initializing DLock services");
    if (UbsmLock::Instance().IsUbsmLockInit()) {
        DBG_LOGINFO("master has been initialized.");
        return;
    }
    DoDLockServerInit(localNode.ip);
}

void UbsmLockEvent::DoDLockServerInit(const std::string &serverIp)
{
    // 主节Init
    DBG_LOGINFO("Initializing DLock server, serverIp=" << serverIp);
    int32_t ret = dlock::DLOCK_SUCCESS;
    auto &cfg = DLockContext::Instance().GetConfig();
    //    rpc::RpcNode masterNode;
    cfg.serverIp = serverIp;
    cfg.isDlockServer = true;
    size_t retryCount = 0;
    do {
        ret = UbsmLock::Instance().Init();
        if (ret != UBSM_OK) {
            UbsmLock::Instance().DeInit();
            DBG_LOGERROR("UbsmLock server init failed, ret=" << ret << ", retryCount=" << retryCount);
        }
    } while (ret != UBSM_OK && ++retryCount < DEFAULT_TRY_COUNT);
    DBG_LOGINFO("UbsmLock server initialized end, serverIp=" << serverIp << ", ret=" << ret);
}

void UbsmLockEvent::OnDLockClientInit(const std::string &masterId)
{
    int32_t ret = dlock::DLOCK_SUCCESS;
    if (UbsmLock::Instance().IsUbsmLockInit()) { // 锁状态
        DBG_LOGDEBUG("UbsmLock client already initialized, skipping reinitialization");
        return;
    }
    DBG_LOGINFO("Processing UbsmLock client initialization event, master=" << masterId);
    rpc::RpcNode masterNode;
    if (rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(masterId, masterNode) != UBSM_OK) {
        DBG_LOGERROR("Invalid master Id: " << masterId);
        return;
    }
    auto &rpcConfig = rpc::NetRpcConfig::GetInstance();
    auto localNode = rpcConfig.GetLocalNode();
    DBG_LOGINFO("UbsmLock client init process, serverIp=" << masterNode.ip << ", clientIp=" << localNode.ip);
    auto &cfg = DLockContext::Instance().GetConfig();
    cfg.serverIp = masterNode.ip;
    cfg.clientIp = localNode.ip;
    cfg.isDlockServer = false;
    cfg.isDlockClient = true;
    size_t retryCount = 0;
    do {
        ret = UbsmLock::Instance().Init();
        if (ret != UBSM_OK) {
            UbsmLock::Instance().DeInit();
            DBG_LOGERROR("UbsmLock client init failed, retCode: " << ret << ", retryCount=" << retryCount);
        }
    } while (ret != UBSM_OK && ++retryCount < DEFAULT_TRY_COUNT);
    DBG_LOGINFO("Client initialize end, serverIp=" << cfg.serverIp << ", clientIp=" << cfg.clientIp << ", ret=" << ret);
}

void UbsmLockEvent::OnDLockDemoted()
{
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();

    if (cfg.isDlockServer) {
        DBG_LOGINFO("Process master demotion, serverIP=" << cfg.serverIp << ", clientIP=" << cfg.clientIp);
        DLockExecutor::GetInstance().DLockSeverLibDeInitFunc();
        ctx.SetServerDeinitFlag(false);
        cfg.isDlockServer = false;
        cfg.recoveryClientNum = 0;
    }
    if (cfg.isDlockClient) {
        DBG_LOGINFO("Process client demotion, serverIP=" << cfg.serverIp << ", clientIP=" << cfg.clientIp);
        DLockExecutor::GetInstance().DLockClientLibDeInitFunc();
        cfg.isDlockClient = false;
    }
    DBG_LOGERROR("Change lock flag to false in OnDLockDemoted.");
    UbsmLock::Instance().UbsmLockInitSet(false);
}
void UbsmLockEvent::OnDLockServerRecovery(const std::string &masterId, const std::string &clientId)
{
    DBG_LOGINFO("Ubsm Lock server recovery, masterId=" << masterId << ", clientId=" << clientId);
    rpc::RpcNode masterNode;
    rpc::RpcNode reinitNode;
    if (rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(masterId, masterNode) != UBSM_OK) {
        DBG_LOGERROR("Get rpcNode from masterId failed, masterId: " << masterId);
        return;
    }
    if (rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(clientId, reinitNode) != UBSM_OK) {
        DBG_LOGERROR("Get rpcNode from clientId failed, clientId: " << clientId);
        return;
    }
    auto ret = DoRecovery(masterNode, reinitNode);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("DLock server recovery failed for node ID: " << masterId << ", Triggering leader re-election.");
    }
}

int32_t UbsmLockEvent::DoRecovery(const rpc::RpcNode &masterNode, const rpc::RpcNode &reinitNode)
{
    DBG_LOGINFO("Starting recovery process for master node=" << masterNode.name << " client node=" << reinitNode.name);
    int32_t ret = dlock::DLOCK_SUCCESS;
    size_t retryCount = 0;
    auto &cfg = DLockContext::Instance().GetConfig();
    cfg.isDlockServer = true;
    cfg.isDlockClient = false;
    cfg.serverIp = masterNode.ip;
    // 单一client
    cfg.recoveryClientNum = 1;
    DBG_LOGINFO("UbsmLock do recovery, serverIP=" << cfg.serverIp << ", client node ip=" << reinitNode.ip);
    do {
        ret = UbsmLock::Instance().Reinit();
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("UbsmLock server Promoted failed, retCode: " << ret);
            UbsmLock::Instance().DeinitTlsConfig();
            continue;
        }
        // send rpc message to client
        auto rpcReq = std::make_shared<DLockClientReinitRequest>(cfg.serverIp);
        auto rpcRsp = std::make_shared<DLockClientReinitResponse>();
        if (rpcReq == nullptr || rpcRsp == nullptr) {
            DBG_LOGERROR("make_shared failed, retry=" << retryCount << ", serverIp=" << cfg.serverIp);
            continue;
        }

        ret = rpc::service::RpcServer::GetInstance().Connect(reinitNode);
        if (ret != 0) {
            DBG_LOGERROR("Connect node=" << reinitNode.name << " failed, ret=" << ret << ", retry=" << retryCount);
            continue;
        }
        ret = RpcServer::GetInstance().SendMsg(RPC_DLOCK_CLIENT_REINIT, rpcReq.get(), rpcRsp.get(), reinitNode.name);
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("DLock server recovery failed: Send RPC message failed.");
            continue;
        }
        ret = rpcRsp->dLockCode_;
        if (ret != UBSM_OK) {
            DBG_LOGERROR("RPC_DLOCK_CLIENT_REINIT request failed, node=" << reinitNode.name << ", ret=" << ret);
            continue;
        }
        UbsmLock::Instance().UbsmLockInitSet(true);
    } while (ret != dlock::DLOCK_SUCCESS && ++retryCount < DEFAULT_TRY_COUNT);
    return ret;
}
