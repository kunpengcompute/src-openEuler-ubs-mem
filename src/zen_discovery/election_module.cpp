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
#include <string>
#include <thread>

#include "election_module.h"
#include "rpc_config.h"
#include "zen_discovery.h"

#include <log.h>
#include <cstring>
namespace ock::zendiscovery {
ElectionModule::ElectionModule(ZenDiscovery &discovery) : discovery_(discovery), running_(true)
{
    listenerThread_ = std::thread(&ElectionModule::ListenerLoop, this);
}

ElectionModule::~ElectionModule()
{
    DBG_LOGDEBUG("ElectionModule destructor called");
    try {
        Stop();
    } catch (const std::exception &e) {
        DBG_LOGERROR("Exception in ElectionModule destructor: ", e.what());
    }
}

void ElectionModule::Stop()
{
    DBG_LOGDEBUG("Stopping ElectionModule...");

    // 设置运行标志为 false
    running_ = false;

    // 通知监听器线程退出
    cond.notify_all(); // 使用 notify_all 而不是 notify_one

    // 等待监听器线程结束
    if (listenerThread_.joinable()) {
        listenerThread_.join();
    }
}

void ElectionModule::ListenerLoop()
{
    while (running_) {
        std::unique_lock<std::mutex> lock(listenerQueueMutex_);
        if (listenerQueue_.empty()) {
            cond.wait_for(lock, std::chrono::milliseconds(500));
            continue;
        }

        auto listenerParam = listenerQueue_.front();
        listenerQueue_.pop();
        lock.unlock();

        // 提取监听器复制和通知逻辑到单独代码块
        NotifyListenerParam(listenerParam);
    }
}

void ElectionModule::NotifyListenerParam(const ListenerParam &listenerParam)
{
    std::list<std::pair<int, ZenEventListener>> listenersCopy;
    {
        std::lock_guard<std::mutex> lock1(listenersMutex_);
        listenersCopy = electionListeners_;
    }

    currentTerm_.load(std::memory_order_relaxed);

    for (const auto &pair : listenersCopy) {
        const auto &listener = pair.second;
        if (!listener) {
            continue;
        }

        try {
            listener(listenerParam.event, listenerParam.masterId, listenerParam.voteOnlyId);
        } catch (const std::exception &e) {
            DBG_LOGERROR("Exception in election listener: " << e.what());
        }
    }
}

// 选举流程
void ElectionModule::RunElection()
{
    // 是否选举流程，单次仅一个进行选举
    if (electionInProgress_) {
        DBG_LOGINFO("Election is already in progress...");
        return;
    }

    electionInProgress_ = true;
    currentTerm_++;
    votesReceived_.clear();

    // 通知选举开始
    notifyListeners(ZenElectionEventType::ELECTION_STARTED);

    NodeCollections collections = CollectNodeInfo();

    if (ZenDiscovery::GetInstance() == nullptr) {
        DBG_LOGERROR("ZenDiscovery::GetInstance() is nullptr.");
        return;
    }

    // 没有达到最低选举条件，退出
    if (collections.masterCandidates.size() < ZenDiscovery::GetInstance()->GetMinimumMasterNodes()) {
        DBG_LOGWARN("Candidate size is " << collections.masterCandidates.size()
                                         << " and is smaller than minimum master node size "
                                         << ZenDiscovery::GetInstance()->GetMinimumMasterNodes() << ".");
        electionInProgress_ = false;
        notifyListeners(ZenElectionEventType::ELECTION_FAILED);
        return;
    }
    // 选举出临时的master节点
    std::string tempMaster = SelectTemporaryMaster(collections);

    bool isSuccess = ConductVoting(tempMaster, collections);
    if (isSuccess) {
        std::vector<std::string> masterList(collections.masterCandidates.begin(), collections.masterCandidates.end());
        ProcessVotingResults(tempMaster, masterList);
    }
    electionInProgress_ = false;
}

bool ElectionModule::IsElectionInProgress() const // 选主过程中，不应该提供锁服务
{
    return electionInProgress_;
}

void ElectionModule::ClearJoinsReceived()
{
    joinsReceived_.clear();
}

void ElectionModule::AddJoinsReceived(const std::string &nodeId)
{
    joinsReceived_.insert(nodeId);
}

std::set<std::string> ElectionModule::GetJoinsReceived() const
{
    return joinsReceived_;
}

int ElectionModule::GetCurrentTerm() const
{
    return currentTerm_;
}

const std::string &ElectionModule::GetElectedMaster() const
{
    return electedMaster_;
}

// 投票处理，收到回调时调用，票数++
void ElectionModule::RecordVote(const std::string &nodeId)
{
    std::lock_guard<std::mutex> lock(votingMutex_);
    votesReceived_.insert(nodeId);
}

void ElectionModule::SetCurrentTerm(int term)
{
    currentTerm_ = term;
}

// 收集各个节点当前认为的主节点activeMasters、活跃节点masterCandidates
ElectionModule::NodeCollections ElectionModule::CollectNodeInfo()
{
    NodeCollections collections;

    if (ZenDiscovery::GetInstance() == nullptr) {
        DBG_LOGERROR("ZenDiscovery::GetInstance() is nullptr.");
        return collections;
    }

    // 获取所有节点
    auto clusterNodes = ZenDiscovery::GetInstance()->GetClusterNodes();
    std::lock_guard<std::mutex> lock(ZenDiscovery::GetInstance()->GetNodesMutex());

    for (const auto &node : clusterNodes) {
        if (node.type == ock::rpc::NodeType::VOTING_ONLY_NODE) {
            collections.voteOnlyNode = node.id;
            continue;
        }
        if (node.isActive && node.type == ock::rpc::NodeType::ELIGIBLE_NODE) {
            collections.masterCandidates.insert(node.id);

            // 当前主节点
            if (node.isMaster) {
                collections.activeMasters.insert(node.id);
            }
        }
    }

    // 包括自己
    collections.masterCandidates.insert(ZenDiscovery::GetInstance()->GetNodeId());

    return collections;
}

// 选举临时主节点
std::string ElectionModule::SelectTemporaryMaster(const NodeCollections &collections)
{
    // 优先选择从所有节点视角看到的主节点
    if (!collections.activeMasters.empty()) {
        return *std::min_element(collections.activeMasters.begin(), collections.activeMasters.end());
    }

    // 否则在所有参选节点选择主节点
    return *std::min_element(collections.masterCandidates.begin(), collections.masterCandidates.end());
}

// 执行投票流程
bool ElectionModule::ConductVoting(const std::string &tempMaster, const NodeCollections &collections)
{
    electionStartTime_ = std::chrono::steady_clock::now();

    if (ZenDiscovery::GetInstance() == nullptr) {
        DBG_LOGERROR("ZenDiscovery::GetInstance() is nullptr.");
        return false;
    }

    // 自己先投票
    {
        std::lock_guard<std::mutex> lock(votingMutex_);
        votesReceived_.insert(ZenDiscovery::GetInstance()->GetNodeId());
    }
    std::set<std::string> candidates = collections.masterCandidates;
    if (!collections.voteOnlyNode.empty()) {
        candidates.insert(collections.voteOnlyNode);
    }
    // 向所有节点发送投票请求
    for (const auto &nodeId : candidates) {
        if (nodeId != ZenDiscovery::GetInstance()->GetNodeId()) {
            ZenDiscovery::GetInstance()->SendVoteRequest(nodeId, tempMaster, currentTerm_);
        }
    }

    // 等待投票结果
    const int electionTimeout = ZenDiscovery::GetInstance()->GetElectionTimeoutMs();
    while (electionInProgress_) {
        // 检查超时
        auto now = std::chrono::steady_clock::now();
        if (now - electionStartTime_ > std::chrono::milliseconds(electionTimeout)) {
            DBG_LOGERROR("Vote has exceeded time limit=" << electionTimeout << ", vote size=" << votesReceived_.size());
            return false;
        }

        // 检查是否获得多数票
        {
            std::lock_guard<std::mutex> lock(votingMutex_);
            if (votesReceived_.size() >= ZenDiscovery::GetInstance()->GetMinimumMasterNodes()) {
                return true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return true;
}

// 处理投票结果
void ElectionModule::ProcessVotingResults(const std::string &tempMaster,
                                          const std::vector<std::string> &masterCandidates)
{
    size_t votesCount;
    {
        std::lock_guard<std::mutex> lock(votingMutex_);
        votesCount = votesReceived_.size();
    }
    notifyListeners(ZenElectionEventType::VOTING_COMPLETED, "", "");

    if (ZenDiscovery::GetInstance() == nullptr) {
        DBG_LOGERROR("ZenDiscovery::GetInstance() is nullptr.");
        return;
    }

    if (votesCount < ZenDiscovery::GetInstance()->GetMinimumMasterNodes()) {
        DBG_LOGINFO("votesCount " << votesCount << " is smaller than minimum master node size "
                                  << ZenDiscovery::GetInstance()->GetMinimumMasterNodes() << ".");
        notifyListeners(ZenElectionEventType::ELECTION_FAILED, "", "");
        return;
    }
    // 若临时选举人不是自己，则需要将选举结果transfer给临时选举人，让临时选举人操作
    if (tempMaster != ZenDiscovery::GetInstance()->GetNodeId()) {
        ZenDiscovery::GetInstance()->SendTransElectedRequest(masterCandidates, tempMaster, currentTerm_);
        electedMaster_ = tempMaster;
    }
    // 更新自身状态
    if (tempMaster == ZenDiscovery::GetInstance()->GetNodeId()) {
        ZenDiscovery::GetInstance()->BecomeTempMaster(masterCandidates);
    }
}

void ElectionModule::BroadcastElectionResult(const std::string &electedMaster)
{
    if (ZenDiscovery::GetInstance() == nullptr) {
        DBG_LOGERROR("ZenDiscovery::GetInstance() is nullptr.");
        return;
    }
    auto clusterNodes = ZenDiscovery::GetInstance()->GetClusterNodes();
    for (const auto &node : clusterNodes) {
        if (node.id != ZenDiscovery::GetInstance()->GetNodeId()) {
            DBG_LOGINFO("ElectionResult has been broadcast, the master is " << electedMaster << ", send result to "
                                                                            << node.id);
            ZenDiscovery::GetInstance()->SendMasterElected(node.id, electedMaster, currentTerm_);
        }
    }
    electedMaster_ = electedMaster;
}

// 事件监听管理
int ElectionModule::AddElectionListener(ZenEventListener listener)
{
    std::lock_guard<std::mutex> lock(listenersMutex_);
    int id = ++nextListenerId_; // 生成唯一ID
    electionListeners_.emplace_back(id, std::move(listener));
    return id; // 返回ID，供后续删除使用
}

void ElectionModule::RemoveElectionListener(int listenerId)
{
    std::lock_guard<std::mutex> lock(listenersMutex_);
    electionListeners_.remove_if([listenerId](const auto &item) {
        return item.first == listenerId; // 按ID删除
    });
}

// 触发事件通知
void ElectionModule::notifyListeners(ZenElectionEventType event, const std::string &masterId,
                                     const std::string &voteOnlyId)
{
    std::lock_guard<std::mutex> lock(listenerQueueMutex_);
    listenerQueue_.push({.event = event, .masterId = masterId, .voteOnlyId = voteOnlyId});
    cond.notify_one();
}
} // namespace ock::zendiscovery