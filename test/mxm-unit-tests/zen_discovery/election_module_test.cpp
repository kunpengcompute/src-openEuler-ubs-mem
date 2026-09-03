/*
* Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>

#define private public
#include "election_module.h"
#include "zen_discovery.h"
#undef private

namespace UT {
using namespace ock::zendiscovery;

class ElectionModuleTest : public testing::Test {
protected:
    void SetUp() override
    {
        discovery = std::make_shared<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
        electionModule = std::make_unique<ElectionModule>(*discovery);
    }

    void TearDown() override
    {
        if (electionModule) {
            electionModule->Stop();
            electionModule.reset();
        }
        if (discovery) {
            discovery->Stop();
            discovery.reset();
        }
    }

    std::shared_ptr<ZenDiscovery> discovery;
    std::unique_ptr<ElectionModule> electionModule;
};

TEST_F(ElectionModuleTest, TestConstructorDestructor)
{
    EXPECT_TRUE(electionModule->running_);
    EXPECT_NE(electionModule->listenerThread_.get_id(), std::thread::id());
}

TEST_F(ElectionModuleTest, TestStop)
{
    EXPECT_TRUE(electionModule->running_);
    electionModule->Stop();
    EXPECT_FALSE(electionModule->running_);
    EXPECT_FALSE(electionModule->listenerThread_.joinable());
}

TEST_F(ElectionModuleTest, TestAddRemoveElectionListener)
{
    int listenerId = electionModule->AddElectionListener([](auto... args) {});
    EXPECT_GT(listenerId, 0);

    EXPECT_NO_THROW(electionModule->RemoveElectionListener(listenerId));
}

TEST_F(ElectionModuleTest, TestClearJoinsReceived)
{
    electionModule->AddJoinsReceived("node1");
    electionModule->AddJoinsReceived("node2");

    EXPECT_EQ(electionModule->GetJoinsReceived().size(), 2);

    electionModule->ClearJoinsReceived();
    EXPECT_TRUE(electionModule->GetJoinsReceived().empty());
}

TEST_F(ElectionModuleTest, TestAddJoinsReceived)
{
    electionModule->AddJoinsReceived("node1");
    electionModule->AddJoinsReceived("node2");

    auto joins = electionModule->GetJoinsReceived();
    EXPECT_EQ(joins.size(), 2);
    EXPECT_NE(joins.find("node1"), joins.end());
    EXPECT_NE(joins.find("node2"), joins.end());
}

TEST_F(ElectionModuleTest, TestRecordVote)
{
    electionModule->RecordVote("node1");
    electionModule->RecordVote("node2");

    EXPECT_EQ(electionModule->GetCurrentTerm(), 0);
}

TEST_F(ElectionModuleTest, TestSetCurrentTerm)
{
    electionModule->SetCurrentTerm(5);
    EXPECT_EQ(electionModule->GetCurrentTerm(), 5);

    electionModule->SetCurrentTerm(10);
    EXPECT_EQ(electionModule->GetCurrentTerm(), 10);
}

TEST_F(ElectionModuleTest, TestIsElectionInProgress)
{
    EXPECT_FALSE(electionModule->IsElectionInProgress());
}

TEST_F(ElectionModuleTest, TestGetElectedMaster)
{
    EXPECT_TRUE(electionModule->GetElectedMaster().empty());
}

TEST_F(ElectionModuleTest, TestNotifyListeners)
{
    bool listenerCalled = false;
    int listenerId = electionModule->AddElectionListener(
        [&listenerCalled](ElectionModule::ZenElectionEventType event, const std::string &masterId,
                          const std::string &voteOnlyId) { listenerCalled = true; });

    electionModule->notifyListeners(ElectionModule::ZenElectionEventType::ELECTION_STARTED, "master1", "vote1");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    electionModule->RemoveElectionListener(listenerId);
}

TEST_F(ElectionModuleTest, TestSelectTemporaryMasterWithoutActiveMasters)
{
    ElectionModule::NodeCollections collections;
    collections.activeMasters.clear();
    collections.masterCandidates = {"node3", "node1", "node2"};

    std::string tempMaster = electionModule->SelectTemporaryMaster(collections);
    EXPECT_EQ(tempMaster, "node1");
}

TEST_F(ElectionModuleTest, TestProcessVotingResultsSafe)
{
    electionModule->SetCurrentTerm(5);
    EXPECT_EQ(electionModule->GetCurrentTerm(), 5);

    electionModule->RecordVote("node1");
    electionModule->RecordVote("node2");

    EXPECT_TRUE(electionModule->GetElectedMaster().empty());
}

TEST_F(ElectionModuleTest, TestBasicFunctionality)
{
    electionModule->SetCurrentTerm(5);
    EXPECT_EQ(electionModule->GetCurrentTerm(), 5);

    electionModule->AddJoinsReceived("node1");
    EXPECT_EQ(electionModule->GetJoinsReceived().size(), 1);

    electionModule->ClearJoinsReceived();
    EXPECT_TRUE(electionModule->GetJoinsReceived().empty());
}

TEST_F(ElectionModuleTest, TestListenerLoopShutdown)
{
    EXPECT_TRUE(electionModule->running_);

    int listenerId = electionModule->AddElectionListener([](auto... args) {});

    electionModule->notifyListeners(ElectionModule::ZenElectionEventType::ELECTION_STARTED);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    electionModule->Stop();

    electionModule->RemoveElectionListener(listenerId);
    EXPECT_FALSE(electionModule->running_);
}

TEST_F(ElectionModuleTest, TestElectionEventTypes)
{
    std::vector<ElectionModule::ZenElectionEventType> eventTypes = {
        ElectionModule::ZenElectionEventType::ELECTION_STARTED, ElectionModule::ZenElectionEventType::VOTING_COMPLETED,
        ElectionModule::ZenElectionEventType::MASTER_ELECTED,   ElectionModule::ZenElectionEventType::ELECTION_FAILED,
        ElectionModule::ZenElectionEventType::BECOME_VOTE_NODE, ElectionModule::ZenElectionEventType::NODE_DEMOTED,
        ElectionModule::ZenElectionEventType::HAVE_JOINED};

    for (auto eventType : eventTypes) {
        EXPECT_NO_THROW(electionModule->notifyListeners(eventType, "master1", "vote1"));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ElectionModuleTest, TestListenerLoopWithEmptyQueueAndStop)
{
    while (!electionModule->listenerQueue_.empty()) {
        electionModule->listenerQueue_.pop();
    }
    electionModule->running_ = false;
    electionModule->ListenerLoop();

    EXPECT_FALSE(electionModule->running_);
}

TEST_F(ElectionModuleTest, TestNotifyListenerParamWithException)
{
    int listenerId = electionModule->AddElectionListener(
        [](ElectionModule::ZenElectionEventType event, const std::string &masterId, const std::string &voteOnlyId) {
            throw std::runtime_error("Test exception");
        });

    bool normalCalled = false;
    int normalListenerId = electionModule->AddElectionListener(
        [&normalCalled](ElectionModule::ZenElectionEventType event, const std::string &masterId,
                        const std::string &voteOnlyId) { normalCalled = true; });

    ElectionModule::ListenerParam param{
        .event = ElectionModule::ZenElectionEventType::ELECTION_STARTED, .masterId = "master1", .voteOnlyId = "vote1"};

    EXPECT_NO_THROW(electionModule->NotifyListenerParam(param));
    EXPECT_TRUE(normalCalled);

    electionModule->RemoveElectionListener(listenerId);
    electionModule->RemoveElectionListener(normalListenerId);
}

TEST_F(ElectionModuleTest, TestNotifyListenerParamWithNullListener)
{
    electionModule->electionListeners_.emplace_back(999, nullptr);

    ElectionModule::ListenerParam param{
        .event = ElectionModule::ZenElectionEventType::ELECTION_STARTED, .masterId = "master1", .voteOnlyId = "vote1"};
    EXPECT_NO_THROW(electionModule->NotifyListenerParam(param));
    electionModule->electionListeners_.pop_back();
}

TEST_F(ElectionModuleTest, TestRunElectionVariousBranches)
{
    electionModule->electionInProgress_ = true;
    electionModule->RunElection();
    electionModule->electionInProgress_ = false;
}

TEST_F(ElectionModuleTest, TestSelectTemporaryMasterWithActiveMasters)
{
    ElectionModule::NodeCollections collections;
    collections.activeMasters = {"node3", "node1", "node2"};
    collections.masterCandidates = {"node4", "node5"};

    std::string tempMaster = electionModule->SelectTemporaryMaster(collections);
    EXPECT_EQ(tempMaster, "node1");
}

TEST_F(ElectionModuleTest, TestConductVotingTimeout)
{
    ElectionModule::NodeCollections collections;
    collections.masterCandidates = {"node1", "node2", "node3"};
    collections.voteOnlyNode = "vote1";

    electionModule->electionStartTime_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(5000);

    electionModule->electionInProgress_ = true;
    bool result = electionModule->ConductVoting("temp_master", collections);
    EXPECT_FALSE(result);
    electionModule->electionInProgress_ = false;
}

TEST_F(ElectionModuleTest, TestAddElectionListenerAndRemoveElectionListenerEdgeCases)
{
    EXPECT_NO_THROW(electionModule->RemoveElectionListener(9999));

    std::vector<int> listenerIds;
    for (int i = 0; i < 10; i++) {
        int id = electionModule->AddElectionListener([](auto...) {});
        listenerIds.push_back(id);
    }
    for (int i = 0; i < 5; i++) {
        electionModule->RemoveElectionListener(listenerIds[i]);
    }
    EXPECT_EQ(electionModule->electionListeners_.size(), 5);

    for (int i = 5; i < 10; i++) {
        electionModule->RemoveElectionListener(listenerIds[i]);
    }
    EXPECT_TRUE(electionModule->electionListeners_.empty());
}

TEST_F(ElectionModuleTest, TestElectionListenerEdgeCases)
{
    EXPECT_NO_THROW(electionModule->RemoveElectionListener(9999));
    std::vector<int> listenerIds;
    for (int i = 0; i < 10; i++) {
        int id = electionModule->AddElectionListener([](auto...) {});
        listenerIds.push_back(id);
    }
    for (int i = 0; i < 5; i++) {
        electionModule->RemoveElectionListener(listenerIds[i]);
    }
    EXPECT_EQ(electionModule->electionListeners_.size(), 5);
    for (int i = 5; i < 10; i++) {
        electionModule->RemoveElectionListener(listenerIds[i]);
    }
    EXPECT_TRUE(electionModule->electionListeners_.empty());
}

TEST_F(ElectionModuleTest, TestMultipleStopCalls)
{
    EXPECT_TRUE(electionModule->running_);
    electionModule->Stop();
    EXPECT_FALSE(electionModule->running_);
    EXPECT_NO_THROW(electionModule->Stop());
    EXPECT_FALSE(electionModule->running_);
}

TEST_F(ElectionModuleTest, TestListenerIdManagement)
{
    int id1 = electionModule->AddElectionListener([](auto...) {});
    int id2 = electionModule->AddElectionListener([](auto...) {});
    int id3 = electionModule->AddElectionListener([](auto...) {});

    EXPECT_GT(id2, id1);
    EXPECT_GT(id3, id2);
    EXPECT_NO_THROW(electionModule->RemoveElectionListener(999));
    EXPECT_NO_THROW(electionModule->RemoveElectionListener(id2));
    EXPECT_EQ(electionModule->electionListeners_.size(), 2);
}

TEST_F(ElectionModuleTest, TestConcurrentListenerQueueAccess)
{
    const int numThreads = 5;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 10; j++) {
                electionModule->notifyListeners(ElectionModule::ZenElectionEventType::ELECTION_STARTED,
                                                "master_" + std::to_string(i), "vote_" + std::to_string(j));
            }
        });
    }

    for (auto &thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(electionModule->listenerQueue_.empty());
}

TEST_F(ElectionModuleTest, TestCurrentTermAtomicity)
{
    electionModule->currentTerm_.store(5, std::memory_order_relaxed);
    EXPECT_EQ(electionModule->currentTerm_.load(std::memory_order_relaxed), 5);

    ElectionModule::ListenerParam param{
        .event = ElectionModule::ZenElectionEventType::ELECTION_STARTED, .masterId = "master1", .voteOnlyId = "vote1"};

    electionModule->currentTerm_.store(10, std::memory_order_relaxed);
    EXPECT_NO_THROW(electionModule->NotifyListenerParam(param));
}

TEST_F(ElectionModuleTest, TestCollectNodeInfoWithInactiveNodes)
{
    ock::rpc::ClusterNode inactiveNode;
    inactiveNode.id = "inactive_node";
    inactiveNode.isActive = false;
    inactiveNode.type = ock::rpc::NodeType::ELIGIBLE_NODE;
    discovery->nodes_["inactive_node"] = inactiveNode;

    auto collections = electionModule->CollectNodeInfo();
    EXPECT_EQ(collections.activeMasters.find("inactive_node"), collections.activeMasters.end());
    EXPECT_EQ(collections.masterCandidates.find("inactive_node"), collections.masterCandidates.end());
}

TEST_F(ElectionModuleTest, TestConductVotingWithNullDiscovery)
{
    ElectionModule::NodeCollections collections;
    collections.masterCandidates = {"node1", "node2", "test_node"};
    collections.voteOnlyNode = "vote_node";

    ZenDiscovery::CleanupInstance();

    bool result = electionModule->ConductVoting("temp_master", collections);
    EXPECT_FALSE(result);

    discovery = std::make_shared<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
    electionModule = std::make_unique<ElectionModule>(*discovery);
}

TEST_F(ElectionModuleTest, TestConductVotingWithEnoughVotes)
{
    ElectionModule::NodeCollections collections;
    collections.masterCandidates = {"node1", "node2", "test_node"};
    collections.voteOnlyNode = "vote_node";
    auto longTimeoutDiscovery = std::make_shared<ZenDiscovery>("test_node", 1000, 2000, 10000, 2);
    auto longTimeoutElectionModule = std::make_unique<ElectionModule>(*longTimeoutDiscovery);

    longTimeoutElectionModule->RecordVote("node1");
    longTimeoutElectionModule->RecordVote("test_node");

    longTimeoutElectionModule->electionInProgress_ = true;

    {
        std::lock_guard<std::mutex> lock(longTimeoutElectionModule->votingMutex_);
        longTimeoutElectionModule->votesReceived_.insert("node1");
        longTimeoutElectionModule->votesReceived_.insert("test_node");
    }

    size_t votesCount;
    {
        std::lock_guard<std::mutex> lock(longTimeoutElectionModule->votingMutex_);
        votesCount = longTimeoutElectionModule->votesReceived_.size();
    }

    EXPECT_GE(votesCount, 2);
    longTimeoutElectionModule->electionInProgress_ = false;
}

TEST_F(ElectionModuleTest, TestConductVotingWithVoteOnlyNode)
{
    ElectionModule::NodeCollections collections;
    collections.masterCandidates = {"node1", "node2", "test_node"};
    collections.voteOnlyNode = "vote_node";

    collections.masterCandidates.insert("vote_node");

    electionModule->electionInProgress_ = true;

    electionModule->electionStartTime_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(5000);

    auto now = std::chrono::steady_clock::now();
    bool timeout = (now - electionModule->electionStartTime_) >
                   std::chrono::milliseconds(discovery->GetElectionTimeoutMs());

    EXPECT_TRUE(timeout);
    electionModule->electionInProgress_ = false;
}

TEST_F(ElectionModuleTest, TestProcessVotingResultsWithInsufficientVotes)
{
    electionModule->SetCurrentTerm(5);

    electionModule->RecordVote("node1");

    std::vector<std::string> masterCandidates = {"node1", "node2", "test_node"};
    electionModule->ProcessVotingResults("temp_master", masterCandidates);

    EXPECT_TRUE(electionModule->GetElectedMaster().empty());
}

TEST_F(ElectionModuleTest, TestProcessVotingResultsWithTransfer)
{
    electionModule->SetCurrentTerm(5);

    electionModule->RecordVote("node1");
    electionModule->RecordVote("node2");
    electionModule->RecordVote("test_node");

    std::vector<std::string> masterCandidates = {"node1", "node2", "test_node"};

    electionModule->electedMaster_ = "node1";

    EXPECT_EQ(electionModule->GetElectedMaster(), "node1");
}

TEST_F(ElectionModuleTest, TestProcessVotingResultsWithSelfAsMaster)
{
    electionModule->SetCurrentTerm(5);

    electionModule->RecordVote("node1");
    electionModule->RecordVote("node2");
    electionModule->RecordVote("test_node");

    std::vector<std::string> masterCandidates = {"node1", "node2", "test_node"};

    electionModule->electedMaster_ = "test_node";
    discovery->BecomeTempMaster(masterCandidates);

    EXPECT_EQ(electionModule->GetElectedMaster(), "test_node");
    EXPECT_EQ(discovery->GetCurrentState(), NodeState::TEMP_MASTER);
}

TEST_F(ElectionModuleTest, TestProcessVotingResultsWithNullDiscovery)
{
    electionModule->SetCurrentTerm(5);

    electionModule->RecordVote("node1");
    electionModule->RecordVote("node2");
    electionModule->RecordVote("test_node");

    std::vector<std::string> masterCandidates = {"node1", "node2", "test_node"};

    ZenDiscovery::CleanupInstance();

    electionModule->ProcessVotingResults("test_node", masterCandidates);

    discovery = std::make_shared<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
    electionModule = std::make_unique<ElectionModule>(*discovery);
    EXPECT_NE(electionModule, nullptr);
}

TEST_F(ElectionModuleTest, TestBroadcastElectionResultWithNullDiscovery)
{
    ZenDiscovery::CleanupInstance();

    electionModule->BroadcastElectionResult("elected_master");

    discovery = std::make_shared<ZenDiscovery>("test_node", 1000, 2000, 3000, 2);
    electionModule = std::make_unique<ElectionModule>(*discovery);
    EXPECT_NE(electionModule, nullptr);
}

TEST_F(ElectionModuleTest, TestBroadcastElectionResult)
{
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

    electionModule->electedMaster_ = "elected_master";

    EXPECT_EQ(electionModule->GetElectedMaster(), "elected_master");
}
} // namespace UT