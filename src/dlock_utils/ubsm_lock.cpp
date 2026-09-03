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
#include "ubsm_lock.h"
#include <arpa/inet.h>
#include <securec.h>
#include <string>
#include "dlock_context.h"
#include "dlock_types.h"
#include "mxm_message/mxm_msg.h"
#include "mxm_shm/rpc_server.h"
#include "rack_mem_functions.h"
#include "ubs_common_config.h"
#include "ubsm_ptracer.h"
#include "zen_discovery.h"

using namespace ock::dlock_utils;
using RpcServer = ock::rpc::service::RpcServer;
constexpr int32_t STARTING_REF_COUNT = 1;
using ZenDiscovery = ock::zendiscovery::ZenDiscovery;
std::atomic<bool> UbsmLock::isCleanupThreadRunning{false};
std::thread UbsmLock::cleanupThread;
std::mutex UbsmLock::cleanupMutex;
std::mutex UbsmLock::initMutex;
int32_t UbsmLock::Init()
{
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();
    // 链接DLock库
    DBG_LOGINFO("Loading DLock dynamic library");
    auto ret = DLockExecutor::GetInstance().InitDLockDlopenLib();
    if (ret != UBSM_OK) {
        DLockExecutor::GetInstance().DestroyDLockDlopenLib();
        DBG_LOGERROR("InitDLockDlopenLib failed, ret: " << ret);
        return ret;
    }
    DBG_LOGINFO("DLock library loaded success. isServer=" << cfg.isDlockServer << ", isClient=" << cfg.isDlockClient);

    struct dlock::ssl_cfg sslConfig = {0};
    ret = InitTlsConfig(sslConfig);
    if (ret != MXM_OK) {
        DBG_LOGERROR("Failed to init tls config of ubsm lock, ret: " << ret);
        return ret;
    }

    if (cfg.isDlockServer) {
        DBG_LOGINFO("Initializing DLock server (ServerID: " << cfg.serverId << "), serverIp=" << cfg.serverIp);
        ret = DlockServerInit(sslConfig);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("DlockServerInit failed, ret: " << ret);
            return ret;
        }
        ctx.SetServerDeinitFlag(true);
    }
    if (cfg.isDlockClient) {
        DBG_LOGINFO("Initialize DLock client library (ServerID=" << cfg.serverId << "), serverIp=" << cfg.serverIp);
        ret = DlockClientLibInit(sslConfig);
        if (ret != UBSM_OK) {
            DLockExecutor::GetInstance().DLockClientLibDeInitFunc();
            DBG_LOGERROR("DlockClientLibInit failed, ret: " << ret);
            return ret;
        }

        DBG_LOGINFO("Creating DLock clients (Count: " << cfg.dlockClientNum << ")");
        ret = ctx.InitDlockClient();
        if (ret != UBSM_OK) {
            DBG_LOGERROR("InitDlockClient failed, ret: " << ret);
            return ret;
        }
    }

    UbsmLock::Instance().UbsmLockInitSet(true); // 锁状态成功
    DBG_LOGINFO("UbsmLock init success, isServer=" << cfg.isDlockServer << ", isClient=" << cfg.isDlockClient);
    return UBSM_OK;
}

int32_t UbsmLock::SetEid(dlock::dlock_eid_t *eid, std::string &devEid)
{
    DBG_LOGINFO("Setting device EID: " << devEid);
    if (eid == nullptr) {
        DBG_LOGERROR("eid is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (devEid.empty()) {
        DBG_LOGERROR("Empty IPv6 address string");
        return MXM_ERR_PARAM_INVALID;
    }
    uint8_t rawEid[SCRLOCK_DEV_EID_SIZE] = {0};
    if (inet_pton(AF_INET6, devEid.c_str(), rawEid) <= 0) {
        DBG_LOGERROR("Invalid IPv6 address: " << devEid);
        return MXM_ERR_PARAM_INVALID;
    }

    auto ret = memcpy_s(eid->raw, DLOCK_EID_SIZE, rawEid, SCRLOCK_DEV_EID_SIZE);
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed, ret: " << ret);
        return MXM_ERR_MEMORY;
    }
    DBG_LOGINFO("Device eid set successfully");
    return MXM_OK;
}

int32_t UbsmLock::DlockServerInit(struct dlock::ssl_cfg ssl)
{
    auto &ctx = DLockContext::Instance();
    DLockConfig cfg = ctx.GetConfig();

    DBG_LOGINFO("Server Config: "
                << "IP=" << cfg.serverIp << ":" << cfg.serverPort << ", CPUs=" << cfg.cmdCpuSet
                << ", Replicas=" << cfg.numOfReplica << ", RecoveryClients=" << cfg.recoveryClientNum);

    struct dlock::primary_cfg primaryCfg = {0};
    primaryCfg.num_of_replica = cfg.numOfReplica;
    primaryCfg.recovery_client_num = cfg.recoveryClientNum;
    primaryCfg.cmd_cpuset = const_cast<char *>(cfg.cmdCpuSet.c_str());
    primaryCfg.ctrl_cpuset = nullptr;
    primaryCfg.server_port = cfg.serverPort;
    primaryCfg.replica_enable = false;
    primaryCfg.server_ip_str = const_cast<char *>(cfg.serverIp.c_str());

    struct dlock::server_cfg conf {
    };
    conf.type = dlock::server_type::SERVER_PRIMARY;
    conf.log_level = cfg.dlockLogLevel;
    conf.sleep_mode_enable = cfg.sleepMode;
    conf.primary = primaryCfg;
    conf.tp_mode = dlock::UNI_CONN;
    conf.ssl = ssl;
    conf.dev_name = const_cast<char *>(cfg.dlockDevName.c_str());
    conf.ub_token_disable = !cfg.ubToken;

    DBG_LOGINFO("Setting device EID for server");
    if (SetEid(&conf.eid, cfg.dlockDevEid) != 0) {
        DBG_LOGERROR("SetEid failed");
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Initializing server library (MaxServers: " << cfg.maxServerNum << ")");
    auto ret = DLockExecutor::GetInstance().DLockServerLibInitFunc(cfg.maxServerNum);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("DLockServerLibInitFunc failed, ret: " << ret);
        return MXM_ERR_DLOCK_INNER;
    }
    // server_start
    ret = DLockExecutor::ServerStartWrapper(conf, cfg.serverId);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("ServerStartWrapper failed, ret: " << ret);
        if (ret == dlock::DLOCK_SERVER_NO_RESOURCE) {
            DBG_LOGERROR("ServerStartWrapper dlock server no resource");
        }
        return MXM_ERR_DLOCK_INNER;
    }
    DBG_LOGINFO("Server started successfully on " << cfg.serverIp << ":" << cfg.serverPort);
    return UBSM_OK;
}

int32_t UbsmLock::DlockClientLibInit(const struct dlock::ssl_cfg &ssl)
{
    auto &ctx = DLockContext::Instance();
    DLockConfig cfg = ctx.GetConfig();

    DBG_LOGINFO("Client Config Device=" << cfg.dlockDevName << ", LogLevel=" << cfg.dlockLogLevel);
    struct dlock::client_cfg conf = {nullptr};
    conf.dev_name = const_cast<char *>(cfg.dlockDevName.c_str());
    conf.log_level = cfg.dlockLogLevel;
    conf.primary_port = cfg.serverPort;
    conf.ssl = ssl;
    conf.tp_mode = dlock::UNI_CONN;
    conf.ub_token_disable = !cfg.ubToken;
    auto res = SetEid(&conf.eid, cfg.dlockDevEid);
    if (res != MXM_OK) {
        DBG_LOGERROR("SetEid failed, ret: " << res);
        return res;
    }

    DBG_LOGINFO("Initializing client library, dev name=" << cfg.dlockDevName);
    auto ret = DLockExecutor::GetInstance().DLockClientLibInitFunc(&conf);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("DLockClientLibInitFunc failed, ret: " << ret);
        return MXM_ERR_DLOCK_INNER;
    }
    DBG_LOGINFO("DlockClientLibInit success");
    return MXM_OK;
}

int32_t UbsmLock::DeInit()
{
    UbsmLock::Instance().UbsmLockInitSet(false);
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();
    DBG_LOGINFO("DeInit UbsmLock, isDlockServer=" << cfg.isDlockServer << ", isDlockClient=" << cfg.isDlockClient);
    if (cfg.isDlockServer) {
        auto ret = DeInitDlockServer();
        if (ret != MXM_OK) {
            DBG_LOGERROR("Client DeInitDLockServer failed, serverIp=" << cfg.serverIp);
        }
    }
    if (cfg.isDlockClient) {
        // 注销已创建的客户端实例
        DBG_LOGINFO("Releasing client instances");
        DLockContext::Instance().UnInitDlockClient();
        // 反初始化DLock客户端库上下文
        if (DLockExecutor::GetInstance().DLockClientLibDeInitFunc != nullptr) {
            DBG_LOGINFO("Deinitializing client library");
            DLockExecutor::GetInstance().DLockClientLibDeInitFunc();
        }
    }
    // 关闭DLock库
    DBG_LOGINFO("Unloading DLock library");
    DLockExecutor::GetInstance().DestroyDLockDlopenLib();
    UbsmLock::Instance().DeinitTlsConfig();
    DBG_LOGINFO("UbsmLock deinitialized completely");
    return dlock::DLOCK_SUCCESS;
}

int32_t UbsmLock::DeInitDlockServer()
{
    auto &ctx = DLockContext::Instance();
    DLockConfig cfg = ctx.GetConfig();
    if (!cfg.isDlockServer || !ctx.IsNeedServerDeinit()) {
        return dlock::DLOCK_SUCCESS;
    }
    if (DLockExecutor::GetInstance().DLockServerStopFunc == nullptr) {
        DBG_LOGERROR("DLockServerStopFunc is nullptr, init first.");
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("Stopping DLock server (ID: " << cfg.serverId << ")");
    auto ret = DLockExecutor::GetInstance().DLockServerStopFunc(cfg.serverId);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("Failed to deInit dlock server, serverId: " << cfg.serverId << ", dlock retcode: " << ret);
        return MXM_ERR_DLOCK_INNER;
    }
    DBG_LOGINFO("Deinitializing server library");
    DLockExecutor::GetInstance().DLockSeverLibDeInitFunc();
    ctx.SetServerDeinitFlag(false);
    DBG_LOGINFO("Server stopped and resources released");
    return MXM_OK;
}

int32_t UbsmLock::GetClientNode(std::string &clientNodeId)
{
    ZenDiscovery *zenDiscovery = ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_LOCK_NOT_READY;
    }

    if (zenDiscovery->GetVoteOnlyNode(clientNodeId) != UBSM_OK) {
        DBG_LOGERROR("Lock service not ready.");
        return MXM_ERR_LOCK_NOT_READY;
    }
    DBG_LOGINFO("GetClientNode success, clientNodeId=" << clientNodeId);
    return MXM_OK;
}

int32_t UbsmLock::Lock(const std::string &name, bool isExclusive, LockUdsInfo &udsInfo)
{
    std::string clientNodeId;
    auto localNode = rpc::NetRpcConfig::GetInstance().GetLocalNode();
    auto ret = GetClientNode(clientNodeId);
    if (ret != MXM_OK) {
        DBG_LOGERROR("GetClientNode failed, ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("UbsmLcok name=" << name << ", lock client=" << clientNodeId << ", local node=" << localNode.name);
    // 本节点就是client
    if (!localNode.name.empty() && localNode.name == clientNodeId) {
        TP_TRACE_BEGIN(TP_UBSE_LOCK_RPC_HANDLE);
        ret = HandleLock(name, isExclusive, udsInfo);
        TP_TRACE_END(TP_UBSE_LOCK_RPC_HANDLE, ret);
        return ret;
    }

    rpc::RpcNode clientNode = {};
    ret = rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(clientNodeId, clientNode);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Invalid client node Id: " << clientNodeId);
        return ret;
    }

    auto rpcReq = std::make_shared<LockRequest>(name, isExclusive, udsInfo.pid, udsInfo.uid, udsInfo.gid);
    auto rpcRsp = std::make_shared<DLockResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("make_shared failed.");
        return MXM_ERR_MALLOC_FAIL;
    }
    TP_TRACE_BEGIN(TP_UBSM_LOCK_RPC_CONNECT);
    auto retConnect = rpc::service::RpcServer::GetInstance().Connect(clientNode);
    TP_TRACE_END(TP_UBSM_LOCK_RPC_CONNECT, retConnect);
    if (retConnect != 0) {
        DBG_LOGERROR("Connect node: " << clientNode.name << " failed, ret = " << retConnect);
        return retConnect;
    }
    DBG_LOGINFO("SendMsg RPC_LOCK, name=" << name << ", client=" << clientNode.name << ", isExclusive=" << isExclusive);
    TP_TRACE_BEGIN(TP_UBSM_LOCK_RPC_SEND_MSG);
    ret = RpcServer::GetInstance().SendMsg(RPC_LOCK, rpcReq.get(), rpcRsp.get(), clientNode.name);
    TP_TRACE_END(TP_UBSM_LOCK_RPC_SEND_MSG, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Send RPC MemLock failed, ret=" << ret << ", name=" << name << ", isExclusive=" << isExclusive);
        return ret;
    }
    if (rpcRsp->dLockCode_ != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("UbsmLock failed, client node id=" << clientNodeId << ", ret=" << rpcRsp->dLockCode_);
        return rpcRsp->dLockCode_;
    }
    DBG_LOGINFO("UbsmLock success, name=" << name << ", isExclusive=" << isExclusive);
    return rpcRsp->dLockCode_;
}

void UbsmLock::CleanUpExpiredLocksForName(const std::string &name, ClientDesc *clientDesc)
{
    if (clientDesc == nullptr) {
        DBG_LOGERROR("CleanUpExpiredLocksForName failed to get lock: " << name);
        return;
    }
    auto invalidUds = clientDesc->GetInValidUdsInfo(name);
    DBG_LOGINFO("name: " << name << " has num:" << invalidUds.size() << " invalid udsInfo.");
    int number = 0;
    for (const auto &udsInfo : invalidUds) {
        if (Instance().UnlockWithDesc(name, clientDesc, udsInfo) == MXM_OK) {
            ++number;
        }
    }
    DBG_LOGINFO("lock name: " << name << " invalidUds Num: " << invalidUds.size() << ", Unlock number: " << number);

    auto lockIdPair = clientDesc->GetLockId(name);
    if (!lockIdPair.first) {
        DBG_LOGINFO("Remove UdsInfo of expired locks for name: " << name);
        clientDesc->RemoveLockUdsInfo(name);
        return;
    }
}

void UbsmLock::CleanupExpiredLocksThread()
{
    if (!UbsmLock::Instance().IsUbsmLockInit() || UbsmLock::Instance().IsUbsmLockInElection()) {
        DBG_LOGINFO("In CleanupExpiredLocksThread, UbsmLock is not ready.");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }
    auto &instance = UbsmLock::Instance();
    while (instance.isCleanupThreadRunning.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 定期清理

        auto &ctx = DLockContext::Instance();
        auto allName = ctx.GetAllShmLockName();

        // 遍历所有锁 name
        for (const auto &name : allName) {
            // 获取对应的 ClientDesc
            auto clientDesc = ctx.GetClientDesc(name);
            if (!clientDesc) {
                DBG_LOGERROR("ClientDesc not found for name: " << name);
                continue;
            }
            if (!clientDesc->IsLockInValidTime(name)) {
                DBG_LOGINFO("Cleaning up expired locks for name: " << name);
                CleanUpExpiredLocksForName(name, clientDesc);
            }
        }
    }
}

int32_t UbsmLock::HandleLock(const std::string &name, bool isExclusive, LockUdsInfo &udsInfo)
{
    // 封装一层，判断自己是不是client，是的话才处理，不是的话，发送rpc处理
    DBG_LOGINFO("HandleLock name=" << name << ", isExclusive=" << isExclusive);
    if (!UbsmLock::Instance().IsUbsmLockInit() || UbsmLock::Instance().IsUbsmLockInElection()) {
        DBG_LOGERROR("UbsmLock is not ready.");
        return MXM_ERR_LOCK_NOT_READY;
    }

    // 检查是否需要启动后台清理线程
    {
        std::lock_guard<std::mutex> guard(cleanupMutex);
        if (!isCleanupThreadRunning.load()) {
            isCleanupThreadRunning.store(true);
            cleanupThread = std::thread(&UbsmLock::CleanupExpiredLocksThread);
        }
    }

    auto &ctx = DLockContext::Instance();
    DBG_LOGINFO("Get client desc for lock=" << name << ", isExclusive=" << isExclusive);
    TP_TRACE_BEGIN(TP_UBSM_GET_LOCK);
    auto clientDesc = GetLock(name, udsInfo);
    TP_TRACE_END(TP_UBSM_GET_LOCK, clientDesc == nullptr ? 1UL : 0UL);
    if (clientDesc == nullptr) {
        DBG_LOGERROR("UbsmLock failed to get lock=" << name);
        return MXM_ERR_LOCK_NOT_FOUND;
    }

    DBG_LOGINFO("Attempting tryLock (ClientID=" << clientDesc->GetClientId() << "), name=" << name);
    TP_TRACE_BEGIN(TP_UBSM_TRY_LOCK);
    auto ret = TryLock(name, clientDesc, isExclusive, udsInfo);
    TP_TRACE_END(TP_UBSM_TRY_LOCK, ret);
    auto lockIdPair = clientDesc->GetLockId(name);
    if (!lockIdPair.first) {
        DBG_LOGERROR("LockId does not exist, an unexpected error, name=" << name);
        return MXM_ERR_LOCK_NOT_FOUND;
    }

    if (ret != MXM_OK) {
        auto clientId = clientDesc->GetClientId();
        DBG_LOGERROR("Trylock failed, clientId=" << clientId << ", lockId=" << lockIdPair.second << ", name=" << name
                                                 << ", ret=" << ret);
        // 初次获取锁，但try_lock失败
        if (ctx.ReleaseClientDescRef(name) < 0) {
            TryReleaseLock(name, lockIdPair.second);
        }
        return ret;
    }
    DBG_LOGINFO("HandleLock success, name=" << name << ", lockId=" << lockIdPair.second << ", name=" << name);
    return ret;
}

int32_t UbsmLock::TryLock(const std::string &name, ClientDesc *clientDesc, bool isExclusive, LockUdsInfo &udsInfo)
{
    if (isExclusive && DLockContext::Instance().GetClientDescRef(name) > STARTING_REF_COUNT) {
        DBG_LOGINFO("try to acquire write lock again. name=" << name);
        return MXM_ERR_LOCK_ALREADY_LOCKED;
    }

    auto lockIdPair = clientDesc->GetLockId(name);
    if (!lockIdPair.first) {
        DBG_LOGERROR("LockId missing for name=" << name);
        return MXM_ERR_LOCK_NOT_FOUND;
    }
    auto clientId = clientDesc->GetClientId();
    auto lockId = lockIdPair.second;
    auto lockExpireTime = DLockContext::Instance().GetConfig().lockExpireTime;
    int32_t ret = dlock::DLOCK_FAIL;
    struct dlock::lock_request req {
    };
    req.lock_id = lockId;
    req.lock_op = isExclusive ? dlock::LOCK_EXCLUSIVE : dlock::LOCK_SHARED;
    req.expire_time = lockExpireTime;
    dlock::fairlock_state result;
    DBG_LOGINFO("TryLock for name=" << name << ", lockId=" << lockId << ", expireTime=" << lockExpireTime);

    int num = 0;
    do {
        clientDesc->Lock();
        TP_TRACE_BEGIN(TP_UBSM_DLOCK_TRY_LOCK);
        ret = DLockExecutor::GetInstance().DLockTryLockFunc(clientId, &req, &result);
        TP_TRACE_END(TP_UBSM_DLOCK_TRY_LOCK, ret);
        clientDesc->Unlock();
        DBG_LOGINFO("Trylock ret=" << ret << ", clientId=" << clientId << ", lockId=" << lockId << ",name=" << name);
        if (ret == dlock::DLOCK_BAD_RESPONSE) {
            DBG_LOGERROR("DLockTryLockFunc failed(bad response), clientId=" << clientId << ", lockId=" << lockId);
            return MXM_ERR_DLOCK_INNER;
        } else if (ret == dlock::DLOCK_EAGAIN) {
            // 公平锁 返回DLOCK_EAGAIN：加锁排队成功，需要主动调unlock取消排队
            dlock::fairlock_state res;
            DBG_LOGINFO("Dlock Unlock opts, client id: " << clientId << ", lock id: " << lockId);

            clientDesc->Lock();
            DLockExecutor::GetInstance().DLockUnlockFunc(clientId, lockId, &res);
            clientDesc->Unlock();
            DBG_LOGERROR("Dlock client trylock failed, retCode: " << ret << ", client id: " << clientId
                                                                  << ", lock id: " << lockId);
            return MXM_ERR_DLOCK_INNER;
        } else if (ret == dlock::DLOCK_EINVAL || ret == dlock::DLOCK_CLIENTMGR_NOT_INIT ||
                   ret == dlock::DLOCK_CLIENT_NOT_INIT || ret == dlock::DLOCK_LOCK_NOT_GET ||
                   ret == dlock::DLOCK_FAIL) {
            DBG_LOGERROR("Dlock client trylock failed, retCode: " << ret << ", client id: " << clientId
                                                                  << ", lock id: " << lockId);
            return MXM_ERR_DLOCK_INNER;
        } else if (ret == dlock::DLOCK_SUCCESS || ret == dlock::DLOCK_ALREADY_LOCKED) { // 重复加锁，时间延期
            auto validTime = ock::dagger::Monotonic::TimeUs() + (lockExpireTime * 1000000);
            udsInfo.validTime = validTime;
            // 更新udsInfo
            clientDesc->SetLockUdsInfo(name, udsInfo);
            DBG_LOGINFO("Trylock success, name=" << name << ", set validTime=" << validTime << ", clientId=" << clientId
                                                 << ", lockId=" << lockId);
            return MXM_OK;
        }
    } while (num++ < DEFAULT_TRY_LOCK_COUNT && (ret == dlock::DLOCK_NOT_READY || ret == dlock::DLOCK_EASYNC));

    DBG_LOGERROR("TryLock failed, name=" << name << ",clientId=" << clientId << ",lockId=" << lockId << ",ret=" << ret);
    return MXM_ERR_DLOCK_INNER;
}

ClientDesc *UbsmLock::GetLock(const std::string &name, const LockUdsInfo &udsInfo)
{
    int32_t lockId;
    ClientDesc *clientDesc = nullptr;
    auto &ctx = DLockContext::Instance();
    if (ctx.ContainClientDesc(name)) {
        clientDesc = ctx.GetClientDesc(name);
        ctx.AddClientDescRef(name);
        DBG_LOGINFO("ContainClientDesc name " << name);
        return clientDesc;
    }
    clientDesc = ctx.GetDlockClient();
    if (clientDesc == nullptr) {
        DBG_LOGERROR("Failed to GetDlockClient");
        return nullptr;
    }
    auto clientId = clientDesc->GetClientId();

    struct dlock::lock_desc desc {
    };

    desc.p_desc = const_cast<char *>(name.c_str());
    desc.len = name.size();
    desc.lock_type = DLockContext::Instance().GetConfig().lockType;
    desc.lease_time = DLockContext::Instance().GetConfig().leaseTime;

    TP_TRACE_BEGIN(TP_UBSM_DLOCK_GET_LOCK);
    auto ret = DLockExecutor::GetInstance().DLockClientGetLockFunc(clientId, &desc, &lockId);
    TP_TRACE_END(TP_UBSM_DLOCK_GET_LOCK, ret);
    if (ret != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("DLockClientGetLockFunc failed, ret: " << ret);
        if (ret == dlock::DLOCK_SERVER_NO_RESOURCE) {
            DBG_LOGERROR("DLockClientGetLockFunc DLOCK_SERVER_NO_RESOURCE, ret: " << ret);
        }
        return nullptr;
    }
    clientDesc->SetLockId(name, lockId);
    ctx.SetClientDesc(name, clientDesc);
    ctx.AddClientDescRef(name);
    DBG_LOGINFO("Get lock successfully. name: " << name << ", lock id: " << lockId << ", client id: " << clientId);
    return clientDesc;
}

int32_t UbsmLock::Unlock(const std::string &name, const LockUdsInfo &udsInfo)
{
    std::string clientNodeId;
    auto localNode = rpc::NetRpcConfig::GetInstance().GetLocalNode();

    ZenDiscovery *zenDiscovery = ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        DBG_LOGERROR("zenDiscovery is nullptr");
        return MXM_ERR_LOCK_NOT_READY;
    }

    if (zenDiscovery->GetVoteOnlyNode(clientNodeId) != UBSM_OK) {
        DBG_LOGERROR("Lock service not ready.");
        return MXM_ERR_LOCK_NOT_READY;
    }
    // 本节点就是client
    if (!localNode.name.empty() && localNode.name == clientNodeId) {
        TP_TRACE_BEGIN(TP_UBSM_UNLOCK_RPC_HANDLE);
        auto ret = HandleUnlock(name, udsInfo);
        TP_TRACE_END(TP_UBSM_UNLOCK_RPC_HANDLE, ret);
        return ret;
    }

    rpc::RpcNode clientNode = {};
    auto ret = rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(clientNodeId, clientNode);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Invalid client nodeId: " << clientNodeId);
        return ret;
    }

    auto rpcReq = std::make_shared<UnLockRequest>(name, udsInfo.pid, udsInfo.uid, udsInfo.gid);
    if (rpcReq == nullptr) {
        DBG_LOGERROR("rpcReq is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    TP_TRACE_BEGIN(TP_UBSM_LOCK_RPC_CONNECT);
    auto retConnect = rpc::service::RpcServer::GetInstance().Connect(clientNode);
    TP_TRACE_END(TP_UBSM_LOCK_RPC_CONNECT, retConnect);
    if (retConnect != 0) {
        DBG_LOGERROR("Connect node: " << clientNode.name << " failed, ret = " << retConnect);
        return retConnect;
    }
    auto rpcRsp = std::make_shared<DLockResponse>();
    if (rpcRsp == nullptr) {
        DBG_LOGERROR("rpcRsp is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    TP_TRACE_BEGIN(TP_UBSM_UNLOCK_RPC_SEND_MSG);
    ret = RpcServer::GetInstance().SendMsg(RPC_UNLOCK, rpcReq.get(), rpcRsp.get(), clientNode.name);
    TP_TRACE_END(TP_UBSM_UNLOCK_RPC_SEND_MSG, ret);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Send RPC MemUnLock failed, ret: " << ret);
        return ret;
    }
    if (rpcRsp->dLockCode_ != dlock::DLOCK_SUCCESS) {
        DBG_LOGERROR("memory unLock failed, client node id: " << clientNodeId << ", ret: " << rpcRsp->dLockCode_);
    }
    return rpcRsp->dLockCode_;
}

int32_t UbsmLock::HandleUnlock(const std::string &name, const LockUdsInfo &udsInfo)
{
    DBG_LOGINFO("HandleUnlock name=" << name << " pid=" << udsInfo.pid << " uid=" << udsInfo.uid
                                     << " gid=" << udsInfo.gid);
    if (!UbsmLock::Instance().IsUbsmLockInit() || UbsmLock::Instance().IsUbsmLockInElection()) {
        DBG_LOGERROR("UbsmLock is not ready.");
        return MXM_ERR_LOCK_NOT_READY;
    }
    auto &ctx = DLockContext::Instance();
    ClientDesc *clientDesc;
    if (ctx.ContainClientDesc(name)) {
        clientDesc = ctx.GetClientDesc(name);
        if (clientDesc == nullptr) {
            DBG_LOGERROR("Failed to GetClientDesc by name=" << name);
            return MXM_ERR_LOCK_NOT_FOUND;
        }
        if (!clientDesc->IsLockUdsValid(name, udsInfo)) {
            DBG_LOGERROR("UbsmLock name: " << name << " udsInfo is invalid, pid: " << udsInfo.pid
                                           << " uid: " << udsInfo.uid << " gid: " << udsInfo.gid);
            return MXM_ERR_LOCK_NOT_FOUND; // 该进程未给对应name加过锁
        }
        if (!clientDesc->IsLockInValidTime(name, udsInfo)) {
            DBG_LOGWARN("UbsmLock name: " << name << " time is invalid, pid: " << udsInfo.pid << " uid: " << udsInfo.uid
                                          << " gid: " << udsInfo.gid);
            // 解锁
            auto ret = UnlockWithDesc(name, clientDesc, udsInfo);
            DBG_LOGINFO("Unlock invalid name: " << name << " ret: " << ret);
            return MXM_OK;
        }

        // 解锁
        auto ret = UnlockWithDesc(name, clientDesc, udsInfo);
        if (ret != MXM_OK) {
            DBG_LOGERROR("UnlockWithDesc failed, name: " << name << " udsInfo is invalid, pid: " << udsInfo.pid
                                                         << " uid: " << udsInfo.uid << " gid: " << udsInfo.gid
                                                         << ", ret: " << ret);
            return ret;
        }
        return ret;
    } else {
        DBG_LOGERROR("Try to unlock a non-existent lock, name=" << name);
        return MXM_ERR_LOCK_NOT_FOUND;
    }
}

int32_t UbsmLock::UnlockWithDesc(const std::string &name, ClientDesc *clientDesc, const LockUdsInfo &udsInfo)
{
    auto clientId = clientDesc->GetClientId();
    auto lockIdPair = clientDesc->GetLockId(name);
    if (!lockIdPair.first) {
        DBG_LOGERROR("LockId missing for: " << name);
        return MXM_ERR_LOCK_NOT_FOUND;
    }
    int32_t lockId = lockIdPair.second;

    DBG_LOGINFO("Try to do Unlock name: " << name << " lock id: " << lockId << ", client id: " << clientId);
    int32_t ret = dlock::DLOCK_FAIL;
    auto &ctx = DLockContext::Instance();
    int num = 0;
    do {
        clientDesc->Lock();
        TP_TRACE_BEGIN(TP_UBSM_DLOCK_UNLOCK);
        ret = DoUnlock(lockId, clientId);
        TP_TRACE_END(TP_UBSM_DLOCK_UNLOCK, ret);
        clientDesc->Unlock();
        DBG_LOGINFO("Dlock unlock retCode: " << ret << ", client id: " << clientId << ", lock id: " << lockId);
        if (ret == dlock::DLOCK_ALREADY_UNLOCKED || ret == dlock::DLOCK_FAIL) {
            // FAIL时dlock清空client本地锁状态；UNLOCKED时锁被解锁。ctx都需要更新
            ctx.RemoveClientDescRef(name);
            TryReleaseLock(name, lockId);
            DBG_LOGERROR("Unlock failed, retCode: " << ret << ", client id: " << clientId << ", lock id: " << lockId);
            return MXM_ERR_DLOCK_INNER;
        } else if (ret == dlock::DLOCK_ALREADY_LOCKED || ret == dlock::DLOCK_SUCCESS) {
            if (ctx.ReleaseClientDescRef(name) <= 0) {
                TryReleaseLock(name, lockId);
                // 解锁成功后再删除lock name 对应的进程记录
                clientDesc->RemoveLockUdsInfoByName(name, udsInfo);
            }
            DBG_LOGINFO("Unlock successfully. name: " << name);
            return MXM_OK;
        } else if (ret == dlock::DLOCK_EINVAL || ret == dlock::DLOCK_LOCK_NOT_GET ||
                   ret == dlock::DLOCK_CLIENTMGR_NOT_INIT || ret == dlock::DLOCK_BAD_RESPONSE ||
                   ret == dlock::DLOCK_CLIENT_NOT_INIT) {
            DBG_LOGERROR("Unlock failed, retCode: " << ret << ", client id: " << clientId << ", lock id: " << lockId);
            return MXM_ERR_DLOCK_INNER;
        }
    } while (num++ < DEFAULT_TRY_LOCK_COUNT && (ret == dlock::DLOCK_NOT_READY || ret == dlock::DLOCK_EASYNC));
    DBG_LOGERROR("All unlock attempts exhausted. name: " << name << ", final code: " << ret);
    return MXM_ERR_DLOCK_INNER;
}

int32_t UbsmLock::DoUnlock(int32_t lockId, int32_t clientId)
{
    DBG_LOGINFO("Sending unlock request. lock id: " << lockId << ", client id: " << clientId);
    dlock::fairlock_state result;
    return DLockExecutor::GetInstance().DLockUnlockFunc(clientId, lockId, &result);
}

int32_t UbsmLock::TryReleaseLock(const std::string &name, int32_t lockId)
{
    DBG_LOGINFO("Releasing lock resources. name: " << name << ", lock id: " << lockId);
    auto &ctx = DLockContext::Instance();
    auto clientDesc = ctx.GetClientDesc(name);
    if (clientDesc == nullptr) {
        DBG_LOGERROR("Failed to GetClientDesc by name: " << name);
        return MXM_ERR_NULLPTR;
    }
    ctx.RemoveClientDesc(name);
    clientDesc->RemoveLockId(name);
    auto clientId = clientDesc->GetClientId();

    auto ret = DLockExecutor::GetInstance().DLockReleaseLockFunc(clientId, lockId);
    DBG_LOGINFO("Dlock client try release lock, client id: " << clientId << ", lock id: " << lockId
                                                             << ", retCode: " << ret);
    return ret;
}

int32_t UbsmLock::Reinit()
{
    DBG_LOGINFO("UbsmLock Reinit, set lock flag false.");
    UbsmLock::Instance().UbsmLockInitSet(false);
    auto ret = DLockExecutor::GetInstance().InitDLockDlopenLib();
    if (ret != UBSM_OK) {
        DLockExecutor::GetInstance().DestroyDLockDlopenLib();
        DBG_LOGERROR("InitDLockDlopenLib failed, ret: " << ret);
        return ret;
    }

    struct dlock::ssl_cfg sslConfig = {0};
    ret = InitTlsConfig(sslConfig);
    if (ret != MXM_OK) {
        DBG_LOGINFO("Failed to init tls config of ubsmlock, ret: " << ret);
        DeinitTlsConfig();
        return ret;
    }

    auto &ctx = DLockContext::Instance();
    DBG_LOGINFO("UbsmLock reinit, serverIp: " << ctx.GetConfig().serverIp << ", tls flag: " << sslConfig.ssl_enable);

    if (ctx.GetConfig().isDlockServer) {
        DBG_LOGINFO("Server reinit required. Calculating recovery nodes, serverIp: " << ctx.GetConfig().serverIp);
        auto result = DlockServerReinit(ctx.GetConfig().serverIp, sslConfig);
        if (result != UBSM_OK) {
            DBG_LOGERROR("Failed to reinit dlock server, return code is " << result);
            return result;
        }
    }
    if (ctx.GetConfig().isDlockClient) {
        auto clients = ctx.GetDlockClientList();
        DBG_LOGINFO("Processing " << clients.size() << " clients for reinit");
        for (uint32_t i = 0; i < clients.size(); i++) {
            auto clientDesc = clients.at(i);
            if (clientDesc == nullptr) {
                DBG_LOGERROR("Failed to GetDlockClient");
                return MXM_ERR_NULLPTR;
            }
            auto clientId = clientDesc->GetClientId();
            DBG_LOGINFO("Reinitializing client " << clientId << " (" << (i + 1) << "/" << clients.size() << ")");
            auto result = DlockClientReinit(clientId, ctx.GetConfig().serverIp);
            if (result != MXM_OK) {
                DBG_LOGERROR("Failed to reinit client " << clientId << ", return code is " << result);
                return MXM_ERR_DLOCK_INNER;
            }
            DBG_LOGINFO("Client " << clientId << " reinit successfully");
        }
        DBG_LOGINFO("UbsmLock reinit complete");
        UbsmLock::Instance().UbsmLockInitSet(true); // 锁状态成功
    }
    return MXM_OK;
}

int32_t UbsmLock::DlockServerReinit(const std::string &serverIp, struct dlock::ssl_cfg ssl)
{
    if (serverIp.empty()) {
        DBG_LOGERROR("The server ip is empty!");
        return MXM_ERR_PARAM_INVALID;
    }

    auto &ctx = DLockContext::Instance();
    DBG_LOGINFO("Starting server reinit. Server IP: " << serverIp
                                                      << ", Recovery clients: " << ctx.GetConfig().recoveryClientNum);
    dlock::server_cfg conf = {};
    dlock::primary_cfg primCfg = GetPrimCfg(serverIp, ctx);
    auto retCode = GetServerCfg(ssl, primCfg, conf);
    if (retCode != MXM_OK) {
        DBG_LOGERROR("Get server config failed, ret: " << retCode);
        return retCode;
    }

    DBG_LOGINFO("Server bind core " << primCfg.cmd_cpuset << ", sleep" << conf.sleep_mode_enable);

    if (ctx.IsNeedServerDeinit()) {
        DBG_LOGINFO("Attempting to stop server with serverId=" << ctx.GetConfig().serverId);
        auto ret = DLockExecutor::GetInstance().DLockServerStopFunc(ctx.GetConfig().serverId);
        if (ret == dlock::DLOCK_SUCCESS) {
            DBG_LOGINFO("Server stopped success, serverId=" << ctx.GetConfig().serverId);
        } else {
            DBG_LOGERROR("Failed to stop server, return code is " << ret);
        }
    } else {
        DBG_LOGINFO("Initializing server library for server");
        auto ret = DLockExecutor::GetInstance().DLockServerLibInitFunc(ctx.GetConfig().maxServerNum);
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("Failed to initialize server library for server, return code is " << ret);
            return MXM_ERR_DLOCK_INNER;
        }
        DBG_LOGINFO("Server library initialized successfully");
        ctx.SetServerDeinitFlag(true);
    }
    auto ret = DLockExecutor::ServerStartWrapper(conf, ctx.GetConfig().serverId);
    if (ret != dlock::DLOCK_SUCCESS) {
        if (ret == dlock::DLOCK_SERVER_NO_RESOURCE) {
            DBG_LOGERROR("Failed to start server, dlock server has no resource");
        }
        DBG_LOGERROR("Failed to start server, retCode: " << ret);
        return MXM_ERR_DLOCK_INNER;
    }
    DBG_LOGINFO("Server started successfully, serverId=" << ctx.GetConfig().serverId);
    return UBSM_OK;
}

int32_t UbsmLock::GetServerCfg(const dlock::ssl_cfg &ssl, const dlock::primary_cfg &primCfg, dlock::server_cfg &conf)
{
    auto &ctx = DLockContext::Instance();
    conf.type = dlock::SERVER_PRIMARY;
    conf.dev_name = const_cast<char *>(ctx.GetConfig().dlockDevName.c_str());
    conf.log_level = ctx.GetConfig().dlockLogLevel;
    conf.sleep_mode_enable = ctx.GetConfig().sleepMode;
    conf.primary = primCfg;
    conf.ssl = ssl;
    conf.tp_mode = dlock::UNI_CONN;
    conf.ub_token_disable = !ctx.GetConfig().ubToken;
    auto ret = SetEid(&conf.eid, ctx.GetConfig().dlockDevEid);
    if (ret != MXM_OK) {
        DBG_LOGERROR("SetEid failed, ret: " << ret);
        return ret;
    }
    return MXM_OK;
}

dlock::primary_cfg UbsmLock::GetPrimCfg(const std::string &serverIp, DLockContext &ctx)
{
    struct dlock::primary_cfg primCfg = {0};
    primCfg.num_of_replica = 0;
    primCfg.recovery_client_num = ctx.GetConfig().recoveryClientNum * ctx.GetConfig().dlockClientNum;
    primCfg.cmd_cpuset = const_cast<char *>(ctx.GetConfig().cmdCpuSet.c_str());
    primCfg.ctrl_cpuset = nullptr;
    primCfg.server_ip_str = const_cast<char *>(serverIp.c_str());
    primCfg.server_port = ctx.GetConfig().serverPort;
    primCfg.replica_enable = false;
    return primCfg;
}

void UbsmLock::DoClientReInitStagesClientReInit(int32_t &ret, bool &skipUpdate, int32_t clientId, REINIT_STAGES &stages)
{
    ret = ClientReInitStagesClientReInitDone(clientId, stages);
    DBG_LOGDEBUG("client reinit done with return code " << ret);
    if (ret == dlock::DLOCK_BAD_RESPONSE) {
        skipUpdate = true;
    }
}

int32_t UbsmLock::DlockClientReinit(int32_t clientId, const std::string &serverIp)
{
    DBG_LOGINFO("Begin to reinit client " << clientId);
    uint32_t retryCount = 0;
    int32_t ret = dlock::DLOCK_BAD_RESPONSE;
    REINIT_STAGES stages = REINIT_STAGES::CLIENT_REINIT;
    bool skipUpdate = false;
    int32_t updateRetryTimes = 0;
    // 客户端重新初始化、更新锁和客户端重新初始化完成三个阶段. 每个阶段中，根据 stage 的值决定是否进入下一个阶段
    while (ret != -1 && ret != dlock::DLOCK_EINVAL && ret != dlock::DLOCK_CLIENT_NOT_INIT &&
           ret != dlock::DLOCK_CLIENT_REMOVED_BY_SERVER && ret != dlock::DLOCK_SERVER_NO_RESOURCE) {
        switch (stages) {
            case REINIT_STAGES::CLIENT_REINIT:
                ret = ClientReInitStagesClientReInit(clientId, serverIp.c_str(), retryCount);
                if (ret == dlock::DLOCK_SUCCESS) {
                    stages = skipUpdate ? REINIT_STAGES::CLIENT_REINIT_DONE : REINIT_STAGES::UPDATE_LOCK;
                    skipUpdate = false;
                }
                DBG_LOGINFO("client reinit with return code " << ret);
                break;
            case REINIT_STAGES::UPDATE_LOCK:
                ret = ClientReInitStagesUpdateLocks(clientId, updateRetryTimes, stages);
                if (ret == dlock::DLOCK_BAD_RESPONSE) {
                    DBG_LOGERROR("UpdateLocks returns DLOCK_BAD_RESPONSE, reinit process needs to be performed again.");
                    return ret;
                }
                if (ret != dlock::DLOCK_SUCCESS) {
                    DBG_LOGERROR("update locks failed with return code " << ret);
                    break;
                }
                DoClientReInitStagesClientReInit(ret, skipUpdate, clientId, stages);
                break;
            case REINIT_STAGES::CLIENT_REINIT_DONE:
                DoClientReInitStagesClientReInit(ret, skipUpdate, clientId, stages);
                break;
            default:
                break;
        }
        if (ret == dlock::DLOCK_SUCCESS && stages == REINIT_STAGES::CLIENT_REINIT_DONE) {
            break;
        }
    }
    DBG_LOGINFO("End to reinit client " << clientId);
    return ret;
}

int32_t UbsmLock::ClientReInitStagesClientReInit(int32_t clientId, const char *serverIp, uint32_t &retryCount)
{
    auto ret = DLockExecutor::ClientReinitWrapper(clientId, serverIp);
    DBG_LOGINFO("The ClientReinit return code is " << ret);
    if (ret == -1 || ret == dlock::DLOCK_SERVER_NO_RESOURCE || ret == dlock::DLOCK_EINVAL) {
        DBG_LOGERROR("Client reinit failed, return code is " << ret);
        return ret;
    }
    if (ret == dlock::DLOCK_NOT_READY) {
        if (retryCount++ >= DEFAULT_TRY_COUNT) {
            return -1;
        }
        DBG_LOGWARN("dlock server is not ready");
        sleep(DEFAULT_RETRY_INTERVAL);
    }
    return ret;
}

int32_t UbsmLock::ClientReInitStagesUpdateLocks(int32_t clientId, int32_t &updateRetryTimes, REINIT_STAGES &stages)
{
    int32_t ret = DLockExecutor::GetInstance().DLockUpdateAllLocksFunc(clientId);
    DBG_LOGINFO("UpdateAllLocks ret is " << ret << ", Client reinit stage: " << GetReinitStageName(stages));
    if (ret == dlock::DLOCK_BAD_RESPONSE) {
        return ret;
    } else if (ret == dlock::DLOCK_EAGAIN) {
        stages = REINIT_STAGES::UPDATE_LOCK;
    } else if (ret == dlock::DLOCK_ENOMEM) {
        if (updateRetryTimes++ >= DEFAULT_TRY_COUNT) {
            DBG_LOGERROR("updateAllLocks retry times exceed the limit");
            return -1;
        }
        sleep(DEFAULT_RETRY_INTERVAL);
        stages = REINIT_STAGES::UPDATE_LOCK;
    }
    DBG_LOGDEBUG("Client reinit stage: " << GetReinitStageName(stages));
    return ret;
}

int32_t UbsmLock::ClientReInitStagesClientReInitDone(int32_t clientId, REINIT_STAGES &stages)
{
    int32_t ret = DLockExecutor::GetInstance().DLockClientReinitDoneFunc(clientId);
    DBG_LOGINFO("ClientReinitDone ret is " << ret << ", Client reinit stage: " << GetReinitStageName(stages));
    if (ret == dlock::DLOCK_BAD_RESPONSE) {
        stages = REINIT_STAGES::CLIENT_REINIT;
    } else if (ret == dlock::DLOCK_ENOMEM || ret == dlock::DLOCK_SUCCESS) {
        stages = REINIT_STAGES::CLIENT_REINIT_DONE;
    }
    DBG_LOGINFO("Client reinit stage: " << GetReinitStageName(stages));
    return ret;
}

int32_t UbsmLock::Heartbeat()
{
    auto &ctx = DLockContext::Instance();
    // 不需要开启心跳
    if (!ctx.IsHeartBeatActive()) {
        return dlock::DLOCK_SUCCESS;
    }
    auto clients = ctx.GetDlockClientList();
    for (size_t i = 0; i < clients.size(); i++) {
        auto client = clients.at(i);
        if (client == nullptr) {
            DBG_LOGERROR("Failed to GetDlockClient");
            return MXM_ERR_LOCK_NOT_FOUND;
        }
        auto ret =
            DLockExecutor::GetInstance().DLockClientHeartbeatFunc(client->GetClientId(), DEFAULT_HEART_BEAT_TIMEOUT);
        if (ret != dlock::DLOCK_SUCCESS && ret != dlock::DLOCK_NOT_READY) {
            DBG_LOGERROR("Heartbeat failed for client " << client->GetClientId() << "! Code: " << ret);
            return ret;
        }
        DBG_LOGINFO("Heartbeat sent to client " << client->GetClientId());
    }
    DBG_LOGINFO("Heartbeat completed successfully");
    return dlock::DLOCK_SUCCESS;
}

UbsmLock::~UbsmLock()
{
    isCleanupThreadRunning.store(false);
    if (cleanupThread.joinable()) {
        cleanupThread.join();
    }
}

void GetPrivateKeyPwd(char **keyPwd, int *keyPwdLen)
{
    if (keyPwd == nullptr || keyPwdLen == nullptr) {
        DBG_LOGERROR("Invalid input parameters (keyPwd or keyPwdLen is nullptr).");
        return;
    }
    auto path = UbsCommonConfig::GetInstance().GetLockKeyPath();
    DBG_LOGINFO("key.path=" << path);
    std::pair<char *, int> pwPair;
    auto ret = UbsCryptorHandler::GetInstance().Decrypt(0, UbsCommonConfig::GetInstance().GetLockKeypassPath(), pwPair);
    if (ret != 0) {
        DBG_LOGERROR("Decrypt failed, ret: " << ret);
        return;
    }
    *keyPwd = pwPair.first;
    *keyPwdLen = pwPair.second;
    DBG_LOGINFO("TLS get private key pwd finished.");
}

void ErasePrivateKey(void *addr, int len)
{
    if (addr == nullptr) {
        return;
    }
    char *charAddr = static_cast<char *>(addr);
    auto ret = memset_s(charAddr, len, 0, len);
    if (ret != 0) {
        DBG_LOGERROR("memset for ErasePrivateKey failed, ret=" << ret);
    }
    ock::mxmd::SafeDeleteArray(charAddr);
    DBG_LOGINFO("TLS erase key pass finished.");
    return;
}

int32_t UbsmLock::InitializeTlsPaths()
{
    size_t caLen = UbsCommonConfig::GetInstance().GetLockCaPath().size() + 1;
    tlsConfig.ca_path = new (std::nothrow) char[caLen];
    if (!tlsConfig.ca_path) {
        DBG_LOGERROR("Failed to allocate memory for ca_path.");
        return MXM_ERR_MALLOC_FAIL;
    }

    auto ret = strcpy_s(tlsConfig.ca_path, caLen, UbsCommonConfig::GetInstance().GetLockCaPath().c_str());
    if (ret != 0) {
        DBG_LOGERROR("Failed to strcpy_s ca_path.");
        return MXM_ERR_MEMORY;
    }

    size_t crlLen = UbsCommonConfig::GetInstance().GetLockCrlPath().size() + 1;
    tlsConfig.crl_path = new (std::nothrow) char[crlLen];
    if (!tlsConfig.crl_path) {
        DBG_LOGERROR("Failed to allocate memory for crl_path.");
        return MXM_ERR_MALLOC_FAIL;
    }
    ret = strcpy_s(tlsConfig.crl_path, crlLen, UbsCommonConfig::GetInstance().GetLockCrlPath().c_str());
    if (ret != 0) {
        DBG_LOGERROR("Failed to strcpy_s crl_path.");
        return MXM_ERR_MEMORY;
    }

    size_t certLen = UbsCommonConfig::GetInstance().GetLockCertPath().size() + 1;
    tlsConfig.cert_path = new (std::nothrow) char[certLen];
    if (!tlsConfig.cert_path) {
        DBG_LOGERROR("Failed to allocate memory for cert_path.");
        return MXM_ERR_MALLOC_FAIL;
    }
    ret = strcpy_s(tlsConfig.cert_path, certLen, UbsCommonConfig::GetInstance().GetLockCertPath().c_str());
    if (ret != 0) {
        DBG_LOGERROR("Failed to strcpy_s cert_path.");
        return MXM_ERR_MEMORY;
    }

    size_t prkeyLen = UbsCommonConfig::GetInstance().GetLockKeyPath().size() + 1;
    tlsConfig.prkey_path = new (std::nothrow) char[prkeyLen];
    if (!tlsConfig.prkey_path) {
        DBG_LOGERROR("Failed to allocate memory for prkey_path.");
        return MXM_ERR_MALLOC_FAIL;
    }
    ret = strcpy_s(tlsConfig.prkey_path, prkeyLen, UbsCommonConfig::GetInstance().GetLockKeyPath().c_str());
    if (ret != 0) {
        DBG_LOGERROR("Failed to strcpy_s prkey_path.");
        return MXM_ERR_MEMORY;
    }
    return MXM_OK;
}

int32_t UbsmLock::InitTlsConfig(struct dlock::ssl_cfg &conf)
{
    auto &ctx = DLockContext::Instance();
    auto &cfg = ctx.GetConfig();

    if (!cfg.enableTls) {
        DBG_LOGINFO("Tls flag of ubsm lock is not enabled.");
        return MXM_OK;
    }
    if (tlsConfig.ssl_enable) {
        DBG_LOGINFO("Tls config of dlock is inited.");
        conf = tlsConfig;
        return MXM_OK;
    }
    DBG_LOGINFO("Tls flag of ubsm lock is enabled.");

    auto ret = InitializeTlsPaths();
    if (ret != MXM_OK) {
        DBG_LOGERROR("Initialize tls paths failed, ret: ", ret);
        return ret;
    }
    tlsConfig.cert_verify_cb = nullptr;
    tlsConfig.prkey_pwd_cb = &GetPrivateKeyPwd;
    tlsConfig.erase_prkey_cb = &ErasePrivateKey;
    tlsConfig.ssl_enable = true;
    conf = tlsConfig;
    DBG_LOGINFO("Tls config ca_path: " << conf.ca_path);
    DBG_LOGDEBUG("Tls config crl_path: " << conf.crl_path);
    DBG_LOGDEBUG("Tls config cert_path: " << conf.cert_path);
    DBG_LOGDEBUG("Tls config prkey_path: " << conf.prkey_path);
    return MXM_OK;
}

void UbsmLock::DeinitTlsConfig()
{
    DBG_LOGDEBUG("Try to deinit ubsm lock tls config.");
    // 释放所有动态分配的内存
    if (tlsConfig.ca_path) {
        delete[] tlsConfig.ca_path;
        tlsConfig.ca_path = nullptr;
    }

    if (tlsConfig.crl_path) {
        delete[] tlsConfig.crl_path;
        tlsConfig.crl_path = nullptr;
    }

    if (tlsConfig.cert_path) {
        delete[] tlsConfig.cert_path;
        tlsConfig.cert_path = nullptr;
    }

    if (tlsConfig.prkey_path) {
        delete[] tlsConfig.prkey_path;
        tlsConfig.prkey_path = nullptr;
    }
    tlsConfig.ssl_enable = false;
}
