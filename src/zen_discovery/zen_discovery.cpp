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

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <string>

#include "election_module.h"
#include "log.h"
#include "mxm_msg.h"
#include "rpc_config.h"
#include "rpc_server.h"
#include "ubsm_lock.h"
#include "zen_discovery.h"

namespace ock::zendiscovery {
constexpr auto HEART_BEAT_RETRY_COUNT = 3;
constexpr auto HEART_BEAT_RETRY_INTERVAL = 1;
constexpr auto LOG_PRINT_INTERVAL = 100;

std::mutex ZenDiscovery::mutex_;
ZenDiscovery *ZenDiscovery::instance_;

ZenDiscovery::ZenDiscovery(const std::string &nodeId, int pingTimeoutMs, int joinTimeoutMs, int electionTimeoutMs,
                           int minimumMasterNodes)
    : nodeId_(nodeId),
      pingTimeoutMs_(pingTimeoutMs),
      joinTimeoutMs_(joinTimeoutMs),
      electionTimeoutMs_(electionTimeoutMs),
      minimumMasterNodes_(minimumMasterNodes),
      state_(NodeState::UNINITIALIZED),
      running_(false)
{
    electionModule_ = std::make_unique<ElectionModule>(*this);
}

ZenDiscovery::~ZenDiscovery()
{
    try {
        Stop();
    } catch (const std::exception &e) {
        DBG_LOGERROR("Exception in ZenDiscovery destructor: " << e.what());
    }
}

ZenDiscovery *ZenDiscovery::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        DBG_LOGERROR("ZenDiscovery must be initialized first. Call Initialize() before GetInstance().");
        return nullptr;
    }
    return instance_;
}

void ZenDiscovery::Initialize(int pingTimeoutMs, int joinTimeoutMs, int electionTimeoutMs, int minimumMasterNodes)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        DBG_LOGWARN("ZenDiscovery has been initialized.");
        return;
    }
    std::string localNodeId = ock::rpc::NetRpcConfig::GetInstance().GetLocalNode().name;
    instance_ = new ZenDiscovery(localNodeId, pingTimeoutMs, joinTimeoutMs, electionTimeoutMs, minimumMasterNodes);
}

void ZenDiscovery::CleanupInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ != nullptr) {
        DBG_LOGINFO("Cleaning up ZenDiscovery instance");
        try {
            instance_->Stop(); // 确保先停止所有线程
            delete instance_;
            instance_ = nullptr;
        } catch (const std::exception &e) {
            DBG_LOGERROR("Exception during CleanupInstance: " << e.what());
            // 紧急清理，即使有异常
            delete instance_;
            instance_ = nullptr;
        }
    }
}

void ZenDiscovery::Start()
{
    // 若运行中，直接退出
    DBG_LOGINFO("start Zen-Discovery module.");
    if (running_) {
        DBG_LOGERROR("Zen-Discovery module is already running.");
        return;
    }

    running_ = true;
    state_ = NodeState::INITIAL;
    std::this_thread::sleep_for(std::chrono::milliseconds(pingTimeoutMs_));
    firstPingThread_ = std::thread([this]() {
        try {
            PerformInitialDiscovery();
            PerformPingCycle(true);
            DBG_LOGINFO("First Ping ended successful.");
            discoveryThread_ = std::thread(&ZenDiscovery::DiscoveryLoop, this);
            pingThread_ = std::thread(&ZenDiscovery::PingLoop, this);
            electionThread_ = std::thread(&ZenDiscovery::ElectionLoop, this);
        } catch (...) {
            DBG_LOGERROR("First Ping ended failed.");
        }
    });
}

void ZenDiscovery::Stop()
{
    DBG_LOGINFO("Stopping ZenDiscovery...");
    if (!running_.load()) {
        DBG_LOGINFO("ZenDiscovery has been stopped");
        return;
    }
    running_.store(false);
    // 然后停止 ElectionModule（这会停止 listenerThread_）
    if (electionModule_) {
        electionModule_->Stop();
    }

    if (firstPingThread_.joinable()) {
        firstPingThread_.join();
    }
    if (discoveryThread_.joinable()) {
        discoveryThread_.join();
    }
    if (pingThread_.joinable()) {
        pingThread_.join();
    }
    if (electionThread_.joinable()) {
        electionThread_.join();
    }
}

void ZenDiscovery::HandlePingRequest(const std::string &fromNodeId)
{
    UpdateNodeLastSeen(fromNodeId);
    MarkNodeActive(fromNodeId, true);
    DBG_LOGDEBUG("Get ping request from " << fromNodeId << ".");
}

void ZenDiscovery::HandlePingResponse(const std::string &fromNodeId)
{
    UpdateNodeLastSeen(fromNodeId);
    MarkNodeActive(fromNodeId, true);
}

ock::rpc::NodeType ZenDiscovery::HandleJoinRequest(const std::string &fromNodeId)
{
    UpdateNodeLastSeen(fromNodeId);
    MarkNodeActive(fromNodeId, true);
    DBG_LOGDEBUG("Get join request from " << fromNodeId << ".");
    return type_;
}

bool ZenDiscovery::HandleVoteRequest(const std::string &fromNodeId, const std::string &candidate, int term)
{
    bool granted = false;

    std::string smallestId = GetSmallestNodeId();

    if (term > electionModule_->GetCurrentTerm()) {
        electionModule_->SetCurrentTerm(term);
    }

    if (smallestId == candidate) {
        granted = true;
    } else {
        granted = false;
    }
    DBG_LOGDEBUG("Get vote request from " << fromNodeId << ", chosen grant result=" << granted << ", candidate is "
                                          << candidate);
    return granted;
}

void ZenDiscovery::HandleVoteResponse(const std::string &fromNodeId, bool granted)
{
    DBG_LOGDEBUG("get vote response from " << fromNodeId << ", chosen grant is " << granted);
    if (granted) {
        electionModule_->RecordVote(fromNodeId);
    }
}

void ZenDiscovery::HandleSendTransElected(const std::string &fromNodeId, const std::vector<std::string> &nodeList,
                                          int term)
{
    if (term < electionModule_->GetCurrentTerm()) {
        return;
    }

    if (term > electionModule_->GetCurrentTerm()) {
        electionModule_->SetCurrentTerm(term);
    }
    DBG_LOGDEBUG("Get master election request from " << fromNodeId << ", masterNode is " << nodeId_);
    // 收到后成为临时主节点
    BecomeTempMaster(nodeList);
}

void ZenDiscovery::HandleBroadCastRequest(const std::string &fromNodeId,
                                          const std::map<std::string, ock::rpc::ClusterNode> &nodeList,
                                          bool isSeverInited)
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    nodes_ = nodeList;
    electedMaster_ = fromNodeId;
    state_ = NodeState::JOINED_CLUSTER;

    for (const auto &pair : nodes_) {
        const auto &node = pair.second;
        if (node.id == nodeId_ && node.type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            type_ = ock::rpc::NodeType::VOTING_ONLY_NODE;
            if (isSeverInited) {
                electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::BECOME_VOTE_NODE, electedMaster_,
                                                 nodeId_);
            }
            continue;
        }
        if (node.id == nodeId_ && node.type == ock::rpc::NodeType::ELIGIBLE_NODE &&
            type_ == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            type_ = ock::rpc::NodeType::ELIGIBLE_NODE;
            electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::NODE_DEMOTED, electedMaster_,
                                             nodeId_);
        }
    }
}

std::string ZenDiscovery::GetSmallestNodeId()
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    std::string minId = nodeId_;
    for (const auto &entry : nodes_) {
        const std::string &id = entry.first;
        const ock::rpc::ClusterNode &node = entry.second;
        if (id < minId && node.isActive) {
            minId = id;
        }
    }
    return minId;
}

std::string ZenDiscovery::GetBiggestNodeId()
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    std::string maxId = "";
    for (const auto &entry : nodes_) {
        const std::string &id = entry.first;
        const ock::rpc::ClusterNode &node = entry.second;
        if (id > maxId && node.isActive && !node.isMaster) {
            maxId = id;
        }
    }
    return maxId;
}

// 状态查询
std::vector<ock::rpc::ClusterNode> ZenDiscovery::GetClusterNodes() const
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    std::vector<ock::rpc::ClusterNode> nodes;
    for (const auto &entry : nodes_) {
        nodes.push_back(entry.second);
    }
    return nodes;
}

NodeState ZenDiscovery::GetCurrentState() const
{
    return state_;
}

std::string ZenDiscovery::GetCurrentMaster() const
{
    return electedMaster_;
}

bool ZenDiscovery::IsMaster() const
{
    return state_ == NodeState::ELECTED_MASTER;
}

bool ZenDiscovery::IsTempMaster() const
{
    return state_ == NodeState::TEMP_MASTER;
}

// 状态设置
void ZenDiscovery::BecomeTempMaster(const std::vector<std::string> &candidates)
{
    if (IsTempMaster() || IsMaster()) {
        return;
    }
    state_ = NodeState::TEMP_MASTER;
    electedMaster_ = nodeId_;
    tempNodeList_ = candidates;
    DBG_LOGDEBUG("Node " << nodeId_ << " becomes temporary master");
}

void ZenDiscovery::SetMasterToJoin(const std::string &master)
{
    state_ = NodeState::JOINING_MASTER;
    masterNodeToJoin_ = master;
}

void ZenDiscovery::StepDownAsMaster()
{
    state_ = NodeState::MASTER_CANDIDATE;
    electedMaster_.clear();

    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (auto &entry : nodes_) {
        entry.second.isMaster = false;
    }
    electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::NODE_DEMOTED, "", "");
}

// 根据Node的状态切换不同的方法
void ZenDiscovery::DiscoveryLoop()
{
    while (running_) {
        switch (state_) {
            case NodeState::INITIAL:
                PerformInitialDiscovery();
                break;
            case NodeState::TEMP_MASTER:
                InviteNodesToJoin();
                break;
            case NodeState::ELECTED_MASTER:
                MaintainAsMaster();
                break;
            case NodeState::JOINED_CLUSTER:
                MaintainAsClusterMember();
                break;
            default:
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ZenDiscovery::PingLoop()
{
    while (running_) {
        PerformPingCycle(false);
    }
}

void ZenDiscovery::PerformPingCycle(const bool needRetry)
{
    std::vector<std::string> activeNodes;
    std::unique_lock<std::mutex> lock(nodesMutex_);
    for (const auto &pair : nodes_) {
        const std::string &nodeId = pair.first;
        const ock::rpc::ClusterNode &node = pair.second;
        if (nodeId != nodeId_ && node.isActive) {
            activeNodes.push_back(nodeId);
        }
    }
    lock.unlock();
    for (const auto &nodeId : activeNodes) {
        DBG_LOGDEBUG("Send PingRequest to " << nodeId);
        auto ret = SendPingRequest(nodeId);
        for (uint32_t i = 0; i < 3u && ret != 0 && needRetry; ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(500u));
            ret = SendPingRequest(nodeId);
            DBG_LOGINFO("Send PingRequest to " << nodeId << " failed.");
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(pingTimeoutMs_));
}

void ZenDiscovery::ElectionLoop()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, electionTimeoutMs_);

    while (running_) {
        if (state_ == NodeState::MASTER_CANDIDATE) {
            // 随机延迟策略
            int delay = distr(gen);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));

            if (state_ == NodeState::MASTER_CANDIDATE) {
                DBG_LOGDEBUG("Start run election from current node.");
                electionModule_->RunElection();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void ZenDiscovery::PerformInitialDiscovery()
{
    DBG_LOGDEBUG("Start init node status.");
    if (type_ == ock::rpc::NodeType::VOTING_ONLY_NODE) {
        state_ = NodeState::MASTER_CANDIDATE;
        return;
    }
    // 初始化获取多有节点Id
    std::vector<ock::rpc::RpcNode> ipAndPortList;
    ock::rpc::RpcNode ipAndPort;
    ock::rpc::NetRpcConfig::GetInstance().GetNodes(ipAndPortList, ipAndPort);
    std::vector<ock::rpc::RpcNode> wholeNodes;
    for (const auto &node : ipAndPortList) {
        wholeNodes.push_back(node);
    }
    wholeNodes.push_back(ipAndPort);
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (const auto &rpcNode : wholeNodes) {
            ock::rpc::ClusterNode node;
            node.id = rpcNode.name;
            node.isMaster = false;
            node.lastSeen = std::chrono::system_clock::now().time_since_epoch().count();
            node.isActive = true;
            node.ip = rpcNode.ip;
            node.port = rpcNode.port;
            node.type = ock::rpc::NodeType::ELIGIBLE_NODE;
            nodes_[rpcNode.name] = node;
        }
    }
    state_ = NodeState::MASTER_CANDIDATE;
}

void ZenDiscovery::InviteNodesToJoin()
{
    electionModule_->ClearJoinsReceived();

    std::vector<std::future<void>> futures;

    for (const auto &nodeId : tempNodeList_) {
        if (nodeId != nodeId_) {
            futures.push_back(std::async(std::launch::async, [this, nodeId]() { SendJoinRequest(nodeId); }));
        }
    }
    // 加入者先加入自己
    electionModule_->AddJoinsReceived(nodeId_);
    // 等待加入
    auto startInviteJoinTime_ = std::chrono::steady_clock::now();
    while (running_) {
        if (CheckElectionResult(startInviteJoinTime_)) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

bool ZenDiscovery::CheckElectionResult(const std::chrono::steady_clock::time_point &startTime)
{
    // 检查超时
    auto now = std::chrono::steady_clock::now();
    if (now - startTime > std::chrono::milliseconds(joinTimeoutMs_)) {
        if (electionModule_->GetJoinsReceived().size() >= minimumMasterNodes_) {
            DBG_LOGINFO("electionModule_->GetJoinsReceived().size() >= minimumMasterNodes_");
            HandleAllJoined();
        } else {
            electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::NODE_DEMOTED, nodeId_, "");
            state_ = NodeState::MASTER_CANDIDATE;
        }
        return true;
    }
    // 检查是否获得全票
    if (tempNodeList_.size() <= electionModule_->GetJoinsReceived().size()) {
        DBG_LOGINFO("tempNodeList_.size(" << tempNodeList_.size() << ") <= electionModule_->GetJoinsReceived().size("
                                          << electionModule_->GetJoinsReceived().size() << ")");
        HandleAllJoined();
        return true;
    }
    return false;
}

void ZenDiscovery::HandleAllJoined()
{
    electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::MASTER_ELECTED, nodeId_, "");
    ModifyNodeList();
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (const auto &pair : nodes_) {
            const std::string &nodeId = pair.first;
            const ock::rpc::ClusterNode &node = pair.second;
            if (node.isActive && node.type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
                DBG_LOGINFO("nodeId: " << nodeId << " is NodeType::VOTING_ONLY_NODE, notify HAVE_JOINED.");
                electionModule_->notifyListeners(ElectionModule::ZenElectionEventType::HAVE_JOINED, nodeId_, nodeId);
            }
        }
    }
    state_ = NodeState::ELECTED_MASTER;
}

void ZenDiscovery::ModifyNodeList()
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (auto &nodePair : nodes_) {
        auto &nodeId = nodePair.first;
        auto &node = nodePair.second;

        if (nodeId == nodeId_) {
            node.isActive = true;
            node.isMaster = true;
            continue;
        }
        node.isActive = (electionModule_->GetJoinsReceived().find(nodeId) != electionModule_->GetJoinsReceived().end());
        node.isMaster = false;
    }
}

void ZenDiscovery::LogMaintainAsMaster()
{
    static int printMasterCnt = 0;
    printMasterCnt++;
    if (printMasterCnt >= LOG_PRINT_INTERVAL) {
        DBG_LOGINFO("Current node maintain as master, nodeId = " << nodeId_);
        printMasterCnt = 0;
    }
}

// 主节点定时检测从节点状态,更新集群信息
void ZenDiscovery::MaintainAsMaster()
{
    LogMaintainAsMaster();
    // 检查是否满足最小主节点数
    int activeCount = 0;
    std::string voteOnlyNode;
    bool hasVoteOnlyNode = false;

    std::unique_lock<std::mutex> lock(nodesMutex_);
    for (const auto &entry : nodes_) {
        if (entry.second.isActive) {
            activeCount++;
        }
        if (entry.second.type == ock::rpc::NodeType::VOTING_ONLY_NODE && entry.second.isActive) {
            voteOnlyNode = entry.second.id;
            hasVoteOnlyNode = true;
        }
    }
    activeCount++; // 包括自己

    lock.unlock();
    if (activeCount < minimumMasterNodes_) {
        StepDownAsMaster();
        return;
    }

    if (!hasVoteOnlyNode) {
        voteOnlyNode = GetBiggestNodeId();
        lock.lock();
        auto it = nodes_.find(voteOnlyNode);
        if (it != nodes_.end()) {
            it->second.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
        }
        lock.unlock();
    }
    lock.lock();
    for (const auto &entry : nodes_) {
        if (entry.second.isActive) {
            activeCount++;
        }
        if (entry.second.type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            voteOnlyNode = entry.second.id;
            hasVoteOnlyNode = true;
        }
    }
    for (const auto &pair : nodes_) {
        const std::string &nodeId = pair.first;
        const ock::rpc::ClusterNode &node = pair.second;
        if (nodeId != nodeId_ && node.isActive) {
            lock.unlock();
            SendBroadcastRequest(nodes_, nodeId);
            lock.lock();
        }
    }
}

// 从节点定时检查主节点是否活跃，若主节点不活跃发起选举
void ZenDiscovery::MaintainAsClusterMember()
{
    static int printClusterCnt = 0;
    printClusterCnt++;
    if (printClusterCnt >= LOG_PRINT_INTERVAL) {
        DBG_LOGINFO("Current node maintain as ClusterMember, nodeId = " << nodeId_);
        printClusterCnt = 0;
    }
    if (electedMaster_.empty()) {
        DBG_LOGDEBUG("MaintainAsClusterMember electedMaster_ is empty");
        state_ = NodeState::INITIAL;
        return;
    }
    bool masterActive = false;
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        auto it = nodes_.find(electedMaster_);
        if (it != nodes_.end() && it->second.isActive) {
            masterActive = true;
        }
    }

    // master不活跃时，发起选举
    if (!masterActive) {
        // 当节点状态为MASTER_CANDIDATE时，electionLoop()线程会发起选举
        state_ = NodeState::INITIAL;
        electedMaster_.clear();
    }
}

// 更新节点状态
void ZenDiscovery::UpdateNodeLastSeen(const std::string &nodeId)
{
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.lastSeen = now;
    } else {
        ock::rpc::ClusterNode node;
        node.id = nodeId;
        node.isMaster = false;
        node.lastSeen = now;
        node.isActive = true;
        nodes_[nodeId] = node;
    }
}

void ZenDiscovery::MarkNodeActive(const std::string &nodeId, bool active)
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.isActive = active;
    }
}

int ZenDiscovery::SendPingRequest(const std::string &nodeId)
{
    auto rpcReq = std::make_shared<PingRequest>(nodeId_);
    auto rpcRsp = std::make_shared<RpcQueryInfoResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(nodeId, "PingRequest");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << nodeId << ", request type=PingRequest");
        UpdateNodeLastSeen(nodeId);
        MarkNodeActive(nodeId, false);
        std::lock_guard<std::mutex> lock(nodesMutex_);
        if (state_ == NodeState::ELECTED_MASTER && nodes_[nodeId].type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            nodes_[nodeId].type = ock::rpc::NodeType::ELIGIBLE_NODE;
        }
        return retConnect;
    }
    auto ret =
        ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_PING_NODE_INFO, rpcReq.get(), rpcRsp.get(), nodeId);
    if (ret != 0) {
        DBG_LOGERROR("Local nodeId=" << nodeId_ << ", Send Ping Request to node(" << nodeId << ") failed, ret= " << ret
                                     << ", rsp code=" << rpcRsp->errCode_);
        UpdateNodeLastSeen(nodeId);
        MarkNodeActive(nodeId, false);
        std::lock_guard<std::mutex> lock(nodesMutex_);
        if (state_ == NodeState::ELECTED_MASTER && nodes_[nodeId].type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            nodes_[nodeId].type = ock::rpc::NodeType::ELIGIBLE_NODE;
        }
        return ret;
    }

    UpdateNodeLastSeen(nodeId);
    MarkNodeActive(nodeId, true);
    return 0;
}

int ZenDiscovery::PingDLock(const std::string &nodeId)
{
    if (nodeId != electedMaster_) {
        return 0;
    }
    int32_t ret = dlock::DLOCK_SUCCESS;
    size_t retryCount = 0;
    size_t retryInterval = HEART_BEAT_RETRY_INTERVAL;
    do {
        ret = dlock_utils::UbsmLock::Instance().Heartbeat();
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("heart beat failed, ret=" << ret);
            sleep(retryInterval);
            retryInterval <<= 1;
        }
    } while (ret != dlock::DLOCK_SUCCESS && retryCount++ < HEART_BEAT_RETRY_COUNT);

    if (ret != dlock::DLOCK_SUCCESS) {
        UpdateNodeLastSeen(nodeId);
        MarkNodeActive(nodeId, false);
    }
    return 0;
}

int ZenDiscovery::SendJoinRequest(const std::string &nodeId)
{
    DBG_LOGDEBUG("Send Join Request to nodeId=", nodeId);
    auto rpcReq = std::make_shared<PingRequest>(nodeId);
    auto rpcRsp = std::make_shared<RpcJoinInfoResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(nodeId, "JoinRequest");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << nodeId << ", request type=JoinRequest");
        return retConnect;
    }
    auto ret =
        ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_JOIN_NODE_INFO, rpcReq.get(), rpcRsp.get(), nodeId);
    if (ret != 0) {
        DBG_LOGERROR("Local nodeId=" << nodeId_ << ", Send Join Request to node(" << nodeId << ") failed, ret= " << ret
                                     << ", rsp code=" << rpcRsp->errCode_);
        return ret;
    }
    // 遍历添加nodeType
    if (rpcRsp->nodetype_ == 1) {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        nodes_[nodeId].type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    }
    electionModule_->AddJoinsReceived(nodeId);
    return 0;
}

int ZenDiscovery::SendTransElectedRequest(const std::vector<std::string> &nodes, const std::string &electedMaster,
                                          int term)
{
    auto rpcReq = std::make_shared<TransElectedRequest>(electedMaster, nodes, term);
    auto rpcRsp = std::make_shared<RpcQueryInfoResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(electedMaster, "TransferElected");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << electedMaster << ", request type=TransferElected");
        return retConnect;
    }
    auto ret = ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_SEND_ELECTED_MASTER_INFO, rpcReq.get(),
                                                                   rpcRsp.get(), electedMaster);
    if (ret != 0) {
        DBG_LOGERROR("nodeId=" << electedMaster << ", Send Trans Elected Request failed, ret= " << ret
                               << ", rsp code=" << rpcRsp->errCode_);
        return ret;
    }
    return 0;
}

int ZenDiscovery::SendMasterElected(const std::string &nodeId, const std::string &masterNode, int term)
{
    auto rpcReq = std::make_shared<VoteRequest>(nodeId_, masterNode, term);
    auto rpcRsp = std::make_shared<RpcQueryInfoResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(nodeId, "MasterElected");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << nodeId << ", request type=MasterElected");
        return retConnect;
    }
    auto ret = ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_MASTER_ELECTED_NODE_INFO, rpcReq.get(),
                                                                   rpcRsp.get(), nodeId);
    if (ret != 0) {
        DBG_LOGERROR("Local nodeId=" << nodeId_ << ", Send Master Elected Request to node(" << nodeId
                                     << ") failed, ret= " << ret << ", rsp code=" << rpcRsp->errCode_);
        return ret;
    }
    return 0;
}

int ZenDiscovery::SendVoteRequest(const std::string &nodeId, const std::string &masterNode, int term)
{
    auto rpcReq = std::make_shared<VoteRequest>(nodeId_, masterNode, term);
    auto rpcRsp = std::make_shared<RpcVoteInfoResponse>();
    if (rpcReq == nullptr || rpcRsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(nodeId, "VoteRequest");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << nodeId << ", request type=VoteRequest");
        return retConnect;
    }
    auto ret =
        ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_VOTE_NODE_INFO, rpcReq.get(), rpcRsp.get(), nodeId);
    if (ret != 0) {
        DBG_LOGERROR("Local nodeId=" << nodeId_ << ", Send Vote Request to node(" << nodeId << ") failed, ret= " << ret
                                     << ", rsp code=" << rpcRsp->errCode_);
        return ret;
    }
    HandleVoteResponse(nodeId, rpcRsp->isGranted_);
    return 0;
}

int ZenDiscovery::SendBroadcastRequest(const std::map<std::string, ock::rpc::ClusterNode> &nodes,
                                       const std::string &nodeId)
{
    std::shared_ptr<BroadcastRequest> rpcReq;
    std::shared_ptr<RpcQueryInfoResponse> rpcRsp;
    try {
        rpcReq = std::make_shared<BroadcastRequest>(electedMaster_, nodes,
                                                    dlock_utils::UbsmLock::Instance().IsUbsmLockInit());
        rpcRsp = std::make_shared<RpcQueryInfoResponse>();
    } catch (...) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_MALLOC_FAIL;
    }
    int retConnect = ConnectToRpcNode(nodeId, "BroadcastRequest");
    if (retConnect != 0) {
        DBG_LOGERROR("ConnectToRpcNode failed, nodeId=" << nodeId << ", request type=BroadcastRequest");
        return retConnect;
    }
    auto ret = ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_BROADCAST_NODE_INFO, rpcReq.get(), rpcRsp.get(),
                                                                   nodeId);
    if (ret != 0) {
        DBG_LOGERROR("nodeId=" << nodeId << ", Send Broadcast Request failed, ret = " << ret
                               << ", rsp code=" << rpcRsp->errCode_);
        return ret;
    }
    return 0;
}

int ZenDiscovery::ConnectToRpcNode(const std::string &nodeId, const std::string &reqType)
{
    rpc::RpcNode rpcNode = {};
    auto ret = rpc::NetRpcConfig::GetInstance().ParseRpcNodeFromId(nodeId, rpcNode);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Invalid nodeId=" << nodeId << ", request type=" << reqType);
        return ret;
    }
    auto retConnect = ock::rpc::service::RpcServer::GetInstance().Connect(rpcNode);
    if (retConnect != 0) {
        DBG_LOGERROR("node=" << nodeId << " Send (" << reqType << ") Connect failed, ret=" << retConnect);
        UpdateNodeLastSeen(nodeId);
        MarkNodeActive(nodeId, false);
        return static_cast<int>(retConnect);
    }
    return 0;
}

int32_t ZenDiscovery::GetVoteOnlyNode(std::string &voteOnlyNode) const
{
    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (const auto &entry : nodes_) {
        if (entry.second.isActive && entry.second.type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            voteOnlyNode = entry.second.id;
            return 0;
        }
    }
    return -1;
}
} // namespace ock::zendiscovery