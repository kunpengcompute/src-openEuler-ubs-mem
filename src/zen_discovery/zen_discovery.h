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
#ifndef ZEN_DISCOVERY_H
#define ZEN_DISCOVERY_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "election_module.h"
#include "rpc_config.h"

namespace ock::mxmd {
struct MsgBase;
}

namespace ock::zendiscovery {
enum NodeState : int {
    UNINITIALIZED = 0,
    INITIAL = 1,
    JOINING_MASTER = 2,
    JOINED_CLUSTER = 3,
    MASTER_CANDIDATE = 4,
    TEMP_MASTER = 5,
    ELECTED_MASTER = 6
};

class ZenDiscovery {
public:
    static ZenDiscovery *GetInstance();

    static void Initialize(int pingTimeoutMs, int joinTimeoutMs, int electionTimeoutMs, int minimumMasterNodes);
    static void CleanupInstance();
    friend class ElectionModule; // 允许选举模块访问私有成员

    explicit ZenDiscovery(const std::string &nodeId, int pingTimeoutMs = 3000, int joinTimeoutMs = 10000,
                          int electionTimeoutMs = 5000, int minimumMasterNodes = 1);
    ~ZenDiscovery();

    // 生命周期管理
    void Start();
    void Stop();

    // 消息处理接口
    void HandlePingRequest(const std::string &fromNodeId);
    void HandlePingResponse(const std::string &fromNodeId);
    ock::rpc::NodeType HandleJoinRequest(const std::string &fromNodeId);
    bool HandleVoteRequest(const std::string &fromNodeId, const std::string &candidate, int term);
    void HandleVoteResponse(const std::string &fromNodeId, bool granted);
    void HandleBroadCastRequest(const std::string &fromNodeId,
                                const std::map<std::string, ock::rpc::ClusterNode> &nodeList, bool isSeverInited);
    void HandleSendTransElected(const std::string &fromNodeId, const std::vector<std::string> &nodeList, int term);

    // 状态查询
    std::vector<ock::rpc::ClusterNode> GetClusterNodes() const;
    NodeState GetCurrentState() const;
    ock::rpc::NodeType type_ = ock::rpc::NodeType::ELIGIBLE_NODE;
    std::string GetCurrentMaster() const;
    bool IsMaster() const;
    bool IsTempMaster() const;

    // 供ElectionModule访问的接口
    const std::string &GetNodeId() const
    {
        return nodeId_;
    }
    int GetMinimumMasterNodes() const
    {
        return minimumMasterNodes_;
    }
    int GetElectionTimeoutMs() const
    {
        return electionTimeoutMs_;
    }
    std::mutex &GetNodesMutex()
    {
        return nodesMutex_;
    }

    // 状态设置
    void BecomeTempMaster(const std::vector<std::string> &nodesId);
    void SetMasterToJoin(const std::string &master);
    void StepDownAsMaster();

    using ElectionListener = ElectionModule::ZenEventListener;

    // 添加/移除选举监听器
    int AddElectionListener(ElectionListener listener)
    {
        return electionModule_->AddElectionListener(std::move(listener));
    }

    void RemoveElectionListener(int listenerId)
    {
        electionModule_->RemoveElectionListener(listenerId);
    }

    bool GetElectionInProgress() const
    {
        if (electionModule_ == nullptr) {
            DBG_LOGERROR("electionModule_ is nullptr, init zenDiscovery first.");
            return false;
        }
        return electionModule_->IsElectionInProgress();
    }

    int32_t GetVoteOnlyNode(std::string &voteOnlyNode) const;

private:
    static std::mutex mutex_;
    static ZenDiscovery *instance_;
    // 持续运行的几个线程
    void DiscoveryLoop();
    void PingLoop();
    void ElectionLoop();

    // 内部方法
    void PerformInitialDiscovery();
    void InviteNodesToJoin();
    void PerformPingCycle(bool needRetry);
    void MaintainAsMaster();
    void MaintainAsClusterMember();
    void UpdateNodeLastSeen(const std::string &nodeId);
    void MarkNodeActive(const std::string &nodeId, bool active);
    std::string GetSmallestNodeId();
    std::string GetBiggestNodeId();
    void ModifyNodeList();
    void LogMaintainAsMaster();

    int PingDLock(const std::string &nodeId);
    int SendPingRequest(const std::string &nodeId);
    int SendJoinRequest(const std::string &nodeId);
    int SendMasterElected(const std::string &nodeId, const std::string &masterNode, int term);
    int SendVoteRequest(const std::string &nodeId, const std::string &masterNode, int term);
    int SendBroadcastRequest(const std::map<std::string, ock::rpc::ClusterNode> &nodes, const std::string &nodeId);
    int SendTransElectedRequest(const std::vector<std::string> &nodes, const std::string &electedMaster, int term);
    int ConnectToRpcNode(const std::string &nodeId, const std::string &reqType);
    bool CheckElectionResult(const std::chrono::steady_clock::time_point &startTime);
    void HandleAllJoined();

    // 成员变量
    const std::string nodeId_;
    const int pingTimeoutMs_;
    const int joinTimeoutMs_;
    const int electionTimeoutMs_;
    const int minimumMasterNodes_;

    NodeState state_;
    std::atomic<bool> running_;
    std::string masterNodeToJoin_;
    std::string electedMaster_;
    std::vector<std::string> tempNodeList_;

    mutable std::mutex nodesMutex_;
    std::map<std::string, ock::rpc::ClusterNode> nodes_;

    std::unique_ptr<ElectionModule> electionModule_;

    std::thread firstPingThread_;
    std::thread discoveryThread_;
    std::thread pingThread_;
    std::thread electionThread_;
};
} // namespace ock::zendiscovery
#endif // ZEN_DISCOVERY_H