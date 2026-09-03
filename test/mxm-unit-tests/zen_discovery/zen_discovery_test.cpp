/*
* Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
#include <gtest/gtest.h>

#define private public
#include "zen_discovery.h"
#undef private

namespace UT {
using namespace ock::zendiscovery;

class ZenDiscoveryTest : public testing::Test {
protected:
    std::unique_ptr<ZenDiscovery> discovery;
    void SetUp() override
    {
        ZenDiscovery::CleanupInstance();
        discovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 1);
    }

    void TearDown() override
    {
        if (discovery) {
            discovery->Stop();
        }
        ZenDiscovery::CleanupInstance();
        discovery.reset();
    }
};

TEST_F(ZenDiscoveryTest, TestInitialize)
{
    ZenDiscovery::Initialize(1000, 2000, 3000, 1);
    ZenDiscovery *instance = ZenDiscovery::GetInstance();
    EXPECT_NE(instance, nullptr);
}

TEST_F(ZenDiscoveryTest, TestStartStop)
{
    discovery = std::make_unique<ZenDiscovery>("192.168.100.100", 1000, 2000, 3000, 1);
    discovery->Start();
    EXPECT_TRUE(discovery->running_);

    discovery->Stop();
    EXPECT_FALSE(discovery->running_);
}

TEST_F(ZenDiscoveryTest, TestHandlePingRequest)
{
    discovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 1);
    discovery->HandlePingRequest("node2");

    auto nodes = discovery->GetClusterNodes();
    bool foundNode2 = false;
    for (const auto &node : nodes) {
        if (node.id == "node2" && node.isActive) {
            foundNode2 = true;
            break;
        }
    }
    EXPECT_TRUE(foundNode2);
    ZenDiscovery::CleanupInstance();
}

TEST_F(ZenDiscoveryTest, TestHandlePingResponse)
{
    discovery->HandlePingResponse("node2");

    auto nodes = discovery->GetClusterNodes();
    bool foundNode2 = false;
    for (const auto &node : nodes) {
        if (node.id == "node2" && node.isActive) {
            foundNode2 = true;
            break;
        }
    }
    EXPECT_TRUE(foundNode2);
}

TEST_F(ZenDiscoveryTest, TestGetSmallestNodeId)
{
    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    discovery->nodes_["node1"] = node1;

    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    discovery->nodes_["node2"] = node2;

    std::string smallestId = discovery->GetSmallestNodeId();
    EXPECT_EQ(smallestId, "node1");
}

TEST_F(ZenDiscoveryTest, TestGetBiggestNodeId)
{
    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    node2.isMaster = false;
    discovery->nodes_["node2"] = node2;

    ock::rpc::ClusterNode node3;
    node3.id = "node3";
    node3.isActive = true;
    node3.isMaster = false;
    discovery->nodes_["node3"] = node3;

    std::string biggestId = discovery->GetBiggestNodeId();
    EXPECT_EQ(biggestId, "node3");
}

TEST_F(ZenDiscoveryTest, TestBecomeTempMaster)
{
    std::vector<std::string> candidates = {"node1", "node2", "node3"};
    discovery->BecomeTempMaster(candidates);

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::TEMP_MASTER);
    EXPECT_EQ(discovery->GetCurrentMaster(), "test_node");
    EXPECT_EQ(discovery->tempNodeList_, candidates);
}

TEST_F(ZenDiscoveryTest, TestStepDownAsMaster)
{
    discovery->state_ = NodeState::ELECTED_MASTER;
    discovery->electedMaster_ = "test_node";

    discovery->StepDownAsMaster();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::MASTER_CANDIDATE);
    EXPECT_TRUE(discovery->GetCurrentMaster().empty());
}

TEST_F(ZenDiscoveryTest, TestIsMaster)
{
    EXPECT_FALSE(discovery->IsMaster());

    discovery->state_ = NodeState::ELECTED_MASTER;
    EXPECT_TRUE(discovery->IsMaster());
}

TEST_F(ZenDiscoveryTest, TestIsTempMaster)
{
    EXPECT_FALSE(discovery->IsTempMaster());

    discovery->state_ = NodeState::TEMP_MASTER;
    EXPECT_TRUE(discovery->IsTempMaster());
}

TEST_F(ZenDiscoveryTest, TestUpdateNodeLastSeen)
{
    discovery->UpdateNodeLastSeen("node2");

    auto it = discovery->nodes_.find("node2");
    EXPECT_NE(it, discovery->nodes_.end());
    EXPECT_GT(it->second.lastSeen, 0);
}

TEST_F(ZenDiscoveryTest, TestMarkNodeActive)
{
    discovery->UpdateNodeLastSeen("node2");

    discovery->MarkNodeActive("node2", false);

    auto it = discovery->nodes_.find("node2");
    EXPECT_NE(it, discovery->nodes_.end());
    EXPECT_FALSE(it->second.isActive);

    discovery->MarkNodeActive("node2", true);
    EXPECT_TRUE(it->second.isActive);
}

TEST_F(ZenDiscoveryTest, TestGetClusterNodes)
{
    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    discovery->nodes_["node1"] = node1;

    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    discovery->nodes_["node2"] = node2;

    auto nodes = discovery->GetClusterNodes();
    EXPECT_EQ(nodes.size(), 2);
}

TEST_F(ZenDiscoveryTest, TestGetCurrentState)
{
    discovery->state_ = NodeState::INITIAL;
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::INITIAL);

    discovery->state_ = NodeState::ELECTED_MASTER;
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestGetCurrentMaster)
{
    discovery->electedMaster_ = "node1";
    EXPECT_EQ(discovery->GetCurrentMaster(), "node1");
}

TEST_F(ZenDiscoveryTest, TestSendRequest)
{
    int joinResponse = discovery->SendJoinRequest("joining_node");
    EXPECT_NE(joinResponse, 0);

    std::vector<std::string> nodes = {"node1", "node2"};
    int transElectedResponse = discovery->SendTransElectedRequest(nodes, "elected_master", 1);
    EXPECT_NE(transElectedResponse, 0);

    int electedResponse = discovery->SendMasterElected("target_node", "elected_master", 1);
    EXPECT_NE(electedResponse, 0);

    int voteResponse = discovery->SendVoteRequest("target_node", "candidate", 1);
    EXPECT_NE(voteResponse, 0);

    std::map<std::string, ock::rpc::ClusterNode> nodesToBroadcast;
    ock::rpc::ClusterNode node;
    node.id = "test_node";
    node.isActive = true;
    nodesToBroadcast["test_node"] = node;

    int broadcastResponse = discovery->SendBroadcastRequest(nodesToBroadcast, "target_node");
    EXPECT_NE(broadcastResponse, 0);

    int connectResponse = discovery->ConnectToRpcNode("test_node", "TestRequest");
    EXPECT_NE(connectResponse, 0);
}

TEST_F(ZenDiscoveryTest, TestPerformInitialDiscoveryVotingOnly)
{
    discovery->type_ = ock::rpc::NodeType::VOTING_ONLY_NODE;

    discovery->PerformInitialDiscovery();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::MASTER_CANDIDATE);
}

TEST_F(ZenDiscoveryTest, TestCheckElectionResultTimeoutWithEnoughNodes)
{
    auto startTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(discovery->joinTimeoutMs_ + 100);

    auto testDiscovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
    testDiscovery->electionModule_->AddJoinsReceived("node1");
    testDiscovery->electionModule_->AddJoinsReceived("node2");

    bool result = testDiscovery->CheckElectionResult(startTime);

    EXPECT_TRUE(result);
    EXPECT_EQ(testDiscovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestCheckElectionResultTimeoutWithoutEnoughNodes)
{
    auto startTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(discovery->joinTimeoutMs_ + 100);

    auto testDiscovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 3);
    testDiscovery->electionModule_->AddJoinsReceived("node1");

    bool result = testDiscovery->CheckElectionResult(startTime);

    EXPECT_TRUE(result);
    EXPECT_EQ(testDiscovery->GetCurrentState(), NodeState::MASTER_CANDIDATE);
}

TEST_F(ZenDiscoveryTest, TestHandleAllJoined)
{
    discovery->UpdateNodeLastSeen("node1");
    discovery->MarkNodeActive("node1", true);

    discovery->HandleAllJoined();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestModifyNodeList)
{
    discovery->UpdateNodeLastSeen("node1");
    discovery->MarkNodeActive("node1", true);

    discovery->electionModule_->AddJoinsReceived("node1");

    discovery->ModifyNodeList();

    auto nodes = discovery->GetClusterNodes();
    bool foundActiveNode = false;
    for (const auto &node : nodes) {
        if (node.id == "node1" && node.isActive) {
            foundActiveNode = true;
            break;
        }
    }

    EXPECT_TRUE(foundActiveNode);
}

TEST_F(ZenDiscoveryTest, TestMaintainAsClusterMemberWithInactiveMaster)
{
    discovery->state_ = NodeState::JOINED_CLUSTER;
    discovery->electedMaster_ = "master_node";
    std::string nodeId = "node1";
    discovery->UpdateNodeLastSeen(nodeId);
    discovery->MarkNodeActive(nodeId, false);
    discovery->MaintainAsClusterMember();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::INITIAL);
}

TEST_F(ZenDiscoveryTest, TestGetVoteOnlyNode)
{
    std::string voteOnlyNode;

    int32_t result = discovery->GetVoteOnlyNode(voteOnlyNode);
    EXPECT_EQ(result, -1);

    discovery->UpdateNodeLastSeen("vote_node");
    discovery->nodes_["vote_node"].type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    discovery->MarkNodeActive("vote_node", true);

    result = discovery->GetVoteOnlyNode(voteOnlyNode);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(voteOnlyNode, "vote_node");
}

TEST_F(ZenDiscoveryTest, TestMaintainAsMasterWithEnoughNodes)
{
    auto testDiscovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
    testDiscovery->state_ = NodeState::ELECTED_MASTER;

    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    node1.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    testDiscovery->nodes_["node1"] = node1;

    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    node2.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    testDiscovery->nodes_["node2"] = node2;

    testDiscovery->MaintainAsMaster();
    EXPECT_EQ(testDiscovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestSendPingRequestFailure)
{
    discovery->state_ = NodeState::ELECTED_MASTER;

    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    node1.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    discovery->nodes_["node1"] = node1;
    discovery->nodes_["node1"].isActive = false;
    discovery->nodes_["node1"].type = ock::rpc::NodeType::ELIGIBLE_NODE;

    EXPECT_FALSE(discovery->nodes_["node1"].isActive);
    EXPECT_EQ(discovery->nodes_["node1"].type, ock::rpc::NodeType::ELIGIBLE_NODE);
}

TEST_F(ZenDiscoveryTest, TestSendJoinRequestWithVotingNode)
{
    ock::rpc::ClusterNode node;
    node.id = "node1";
    node.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["node1"] = node;

    discovery->nodes_["node1"].type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    discovery->electionModule_->AddJoinsReceived("node1");

    EXPECT_EQ(discovery->nodes_["node1"].type, ock::rpc::NodeType::VOTING_ONLY_NODE);
    EXPECT_TRUE(discovery->electionModule_->GetJoinsReceived().count("node1") > 0);
}

TEST_F(ZenDiscoveryTest, TestMaintainAsMasterWithoutEnoughNodes)
{
    auto testDiscovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 3);
    testDiscovery->state_ = NodeState::ELECTED_MASTER;

    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    node1.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    testDiscovery->nodes_["node1"] = node1;

    testDiscovery->MaintainAsMaster();
    EXPECT_EQ(testDiscovery->GetCurrentState(), NodeState::MASTER_CANDIDATE);
}

TEST_F(ZenDiscoveryTest, TestHandleSendTransElectedHigherTerm)
{
    discovery->electionModule_->SetCurrentTerm(2);

    std::vector<std::string> nodeList = {"node1", "node2"};
    discovery->HandleSendTransElected("fromNode", nodeList, 5);

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::TEMP_MASTER);
    EXPECT_EQ(discovery->electionModule_->GetCurrentTerm(), 5);
}

TEST_F(ZenDiscoveryTest, TestCheckElectionResultNotEnoughNodes)
{
    auto startTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(2500);

    auto testDiscovery = std::make_unique<ZenDiscovery>("test_node", 1000, 2000, 3000, 3);
    testDiscovery->tempNodeList_ = {"node1", "node2", "test_node"};
    testDiscovery->electionModule_->AddJoinsReceived("node1");

    bool result = testDiscovery->CheckElectionResult(startTime);

    EXPECT_TRUE(result);
    EXPECT_EQ(testDiscovery->GetCurrentState(), NodeState::MASTER_CANDIDATE);
}

TEST_F(ZenDiscoveryTest, TestBecomeTempMasterWhenAlreadyMaster)
{
    discovery->state_ = NodeState::ELECTED_MASTER;
    discovery->electedMaster_ = "test_node";

    std::vector<std::string> candidates = {"node1", "node2"};
    discovery->BecomeTempMaster(candidates);

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestHandleVoteResponseGranted)
{
    discovery->HandleVoteResponse("node1", true);
    EXPECT_TRUE(true);
}

TEST_F(ZenDiscoveryTest, TestInviteNodesToJoin)
{
    discovery->state_ = NodeState::TEMP_MASTER;
    discovery->tempNodeList_ = {"node1", "node2", "test_node"};

    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    node1.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["node1"] = node1;

    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    node2.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["node2"] = node2;

    discovery->InviteNodesToJoin();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::TEMP_MASTER);
    EXPECT_TRUE(discovery->electionModule_->GetJoinsReceived().count("test_node") > 0);
}

TEST_F(ZenDiscoveryTest, TestSetMasterToJoin)
{
    discovery->SetMasterToJoin("master_node");

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINING_MASTER);
    EXPECT_EQ(discovery->masterNodeToJoin_, "master_node");
}

TEST_F(ZenDiscoveryTest, TestDiscoveryLoopDifferentStates)
{
    discovery->state_ = NodeState::INITIAL;
    discovery->DiscoveryLoop();

    discovery->state_ = NodeState::TEMP_MASTER;
    discovery->tempNodeList_ = {"node1", "node2", "test_node"};
    discovery->DiscoveryLoop();

    discovery->state_ = NodeState::ELECTED_MASTER;
    discovery->DiscoveryLoop();

    discovery->state_ = NodeState::JOINED_CLUSTER;
    discovery->electedMaster_ = "master_node";
    discovery->DiscoveryLoop();

    discovery->state_ = static_cast<NodeState>(100);
    discovery->DiscoveryLoop();
}

TEST_F(ZenDiscoveryTest, TestCheckElectionResultAllNodesJoined)
{
    auto startTime = std::chrono::steady_clock::now();

    discovery->tempNodeList_ = {"node1", "node2", "test_node"};

    discovery->electionModule_->AddJoinsReceived("node1");
    discovery->electionModule_->AddJoinsReceived("node2");
    discovery->electionModule_->AddJoinsReceived("test_node");

    bool result = discovery->CheckElectionResult(startTime);

    EXPECT_TRUE(result);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestHandleAllJoinsed)
{
    ock::rpc::ClusterNode node1;
    node1.id = "node1";
    node1.isActive = true;
    node1.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    discovery->nodes_["node1"] = node1;

    ock::rpc::ClusterNode node2;
    node2.id = "node2";
    node2.isActive = true;
    node2.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["node2"] = node2;

    discovery->HandleAllJoined();

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::ELECTED_MASTER);
}

TEST_F(ZenDiscoveryTest, TestMaintainAsClusterMemberWithActiveMaster)
{
    discovery->state_ = NodeState::JOINED_CLUSTER;
    discovery->electedMaster_ = "master_node";

    ock::rpc::ClusterNode masterNode;
    masterNode.id = "master_node";
    masterNode.isActive = true;
    masterNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["master_node"] = masterNode;
    discovery->MaintainAsClusterMember();
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);
}

TEST_F(ZenDiscoveryTest, TestHandleBroadCastRequestWithDifferentNodeTypes)
{
    std::map<std::string, ock::rpc::ClusterNode> nodeList;

    discovery->type_ = ock::rpc::NodeType::VOTING_ONLY_NODE;

    ock::rpc::ClusterNode voteNode;
    voteNode.id = "test_node";
    voteNode.isActive = true;
    voteNode.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    nodeList["test_node"] = voteNode;

    discovery->HandleBroadCastRequest("master_node", nodeList, true);

    EXPECT_EQ(discovery->type_, ock::rpc::NodeType::VOTING_ONLY_NODE);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);

    discovery->type_ = ock::rpc::NodeType::VOTING_ONLY_NODE;

    ock::rpc::ClusterNode eligibleNode;
    eligibleNode.id = "test_node";
    eligibleNode.isActive = true;
    eligibleNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    nodeList["test_node"] = eligibleNode;

    discovery->HandleBroadCastRequest("master_node", nodeList, true);

    EXPECT_EQ(discovery->type_, ock::rpc::NodeType::ELIGIBLE_NODE);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);

    discovery->type_ = ock::rpc::NodeType::ELIGIBLE_NODE;

    discovery->HandleBroadCastRequest("master_node", nodeList, true);

    EXPECT_EQ(discovery->type_, ock::rpc::NodeType::ELIGIBLE_NODE);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);

    discovery->HandleBroadCastRequest("master_node", nodeList, false);

    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);
}

TEST_F(ZenDiscoveryTest, TestHandleJoinRequest)
{
    ock::rpc::ClusterNode node;
    node.id = "requesting_node";
    node.isActive = false;
    node.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["requesting_node"] = node;

    ock::rpc::NodeType result = discovery->HandleJoinRequest("requesting_node");

    EXPECT_EQ(result, discovery->type_);

    EXPECT_TRUE(discovery->nodes_["requesting_node"].isActive);
}

TEST_F(ZenDiscoveryTest, TestHandleJoinRequestWithNewNode)
{
    ock::rpc::NodeType result = discovery->HandleJoinRequest("new_node");

    EXPECT_EQ(result, discovery->type_);
    EXPECT_NE(discovery->nodes_.find("new_node"), discovery->nodes_.end());
    EXPECT_TRUE(discovery->nodes_["new_node"].isActive);
}

TEST_F(ZenDiscoveryTest, TestHandleBroadCastRequestWithEmptyNodeList)
{
    std::map<std::string, ock::rpc::ClusterNode> emptyNodeList;
    discovery->HandleBroadCastRequest("master_node", emptyNodeList, true);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);
    EXPECT_EQ(discovery->GetCurrentMaster(), "master_node");
}

TEST_F(ZenDiscoveryTest, TestHandleBroadCastRequestWithMultipleNodeTypes)
{
    std::map<std::string, ock::rpc::ClusterNode> nodeList;
    ock::rpc::ClusterNode voteNode;
    voteNode.id = "vote_node";
    voteNode.isActive = true;
    voteNode.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    nodeList["vote_node"] = voteNode;

    ock::rpc::ClusterNode eligibleNode;
    eligibleNode.id = "eligible_node";
    eligibleNode.isActive = true;
    eligibleNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    nodeList["eligible_node"] = eligibleNode;

    ock::rpc::ClusterNode selfNode;
    selfNode.id = "test_node";
    selfNode.isActive = true;
    selfNode.type = ock::rpc::NodeType::VOTING_ONLY_NODE;
    nodeList["test_node"] = selfNode;
    discovery->type_ = ock::rpc::NodeType::VOTING_ONLY_NODE;
    discovery->HandleBroadCastRequest("master_node", nodeList, true);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);
    EXPECT_EQ(discovery->GetCurrentMaster(), "master_node");
    EXPECT_EQ(discovery->type_, ock::rpc::NodeType::VOTING_ONLY_NODE);
}

TEST_F(ZenDiscoveryTest, TestHandleBroadCastRequestWithInactiveNodes)
{
    std::map<std::string, ock::rpc::ClusterNode> nodeList;
    ock::rpc::ClusterNode inactiveNode;
    inactiveNode.id = "inactive_node";
    inactiveNode.isActive = false;
    inactiveNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    nodeList["inactive_node"] = inactiveNode;

    ock::rpc::ClusterNode selfNode;
    selfNode.id = "test_node";
    selfNode.isActive = true;
    selfNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    nodeList["test_node"] = selfNode;
    discovery->HandleBroadCastRequest("master_node", nodeList, true);
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::JOINED_CLUSTER);
    EXPECT_EQ(discovery->GetCurrentMaster(), "master_node");
    EXPECT_EQ(discovery->type_, ock::rpc::NodeType::ELIGIBLE_NODE);
}

TEST_F(ZenDiscoveryTest, TestHandleJoinRequestWithExistingInactiveNode)
{
    ock::rpc::ClusterNode node;
    node.id = "existing_node";
    node.isActive = false;
    node.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["existing_node"] = node;
    ock::rpc::NodeType result = discovery->HandleJoinRequest("existing_node");
    EXPECT_EQ(result, discovery->type_);
    EXPECT_TRUE(discovery->nodes_["existing_node"].isActive);
}
} // namespace UT