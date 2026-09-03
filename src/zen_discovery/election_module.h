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

#ifndef ELECTION_MODULE_H
#define ELECTION_MODULE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

namespace ock::zendiscovery {
class ZenDiscovery;
class ElectionModule {
public:
    ~ElectionModule();
    enum class ZenElectionEventType {
        ELECTION_STARTED,
        VOTING_COMPLETED,
        MASTER_ELECTED,
        ELECTION_FAILED,
        NODE_DEMOTED,
        BECOME_VOTE_NODE,
        HAVE_JOINED,
        BROADCAST
    };
    // 选举
    void RunElection();

    bool IsElectionInProgress() const;
    int GetCurrentTerm() const;
    const std::string &GetElectedMaster() const;
    void AddJoinsReceived(const std::string &nodeId);
    void RecordVote(const std::string &nodeId);
    void SetCurrentTerm(int term);
    void ClearJoinsReceived();
    std::set<std::string> GetJoinsReceived() const;

    using ZenEventListener = std::function<void(ZenElectionEventType, const std::string &, const std::string &)>;

    explicit ElectionModule(ZenDiscovery &discovery);

    // 事件监听管理
    int AddElectionListener(ZenEventListener listener);
    void RemoveElectionListener(int listenerId);
    void notifyListeners(ZenElectionEventType event, const std::string &masterId = "",
                         const std::string &voteOnlyId = "");
    void Stop();

private:
    struct NodeCollections {
        std::set<std::string> activeMasters;    // 当前活跃的主节点
        std::set<std::string> masterCandidates; // 候选主节点
        std::string voteOnlyNode;               // 只负责投票的节点
    };
    struct ListenerParam {
        ZenElectionEventType event;
        std::string masterId;
        std::string voteOnlyId;
    };
    std::queue<ListenerParam> listenerQueue_;
    mutable std::mutex listenerQueueMutex_;
    std::thread listenerThread_;
    std::condition_variable cond;

    NodeCollections CollectNodeInfo();
    std::string SelectTemporaryMaster(const NodeCollections &collections);
    bool ConductVoting(const std::string &tempMaster, const NodeCollections &collections);
    void ProcessVotingResults(const std::string &tempMaster, const std::vector<std::string> &candidates);
    void BroadcastElectionResult(const std::string &electedMaster);
    void ListenerLoop();
    void NotifyListenerParam(const ListenerParam &listenerParam);
    ZenDiscovery &discovery_; // 关联的ZenDiscovery实例

    // 选举状态
    std::atomic<bool> electionInProgress_{false};
    std::atomic<int> currentTerm_{0};
    std::string electedMaster_;

    // 投票状态
    mutable std::mutex votingMutex_;
    std::set<std::string> votesReceived_;
    std::set<std::string> joinsReceived_;

    // 时间管理
    std::chrono::steady_clock::time_point electionStartTime_;

    std::mutex listenersMutex_;
    std::list<std::pair<int, ZenEventListener>> electionListeners_;
    int nextListenerId_ = 0;
    std::atomic<bool> running_{true};
};
} // namespace ock::zendiscovery

#endif