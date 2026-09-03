/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <common_def.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "mxm_msg.h"

namespace UT {
using namespace ock::mxmd;

constexpr auto TEST_DEFAULT_REGION_NAME = "default";
constexpr auto TEST_SHM_NAME = "TEST_SHM_NAME";
constexpr auto TEST_LEASE_NAME = "TEST_LEASE_NAME";
constexpr size_t TEST_SHM_SIZE = 1 << 30;
constexpr auto TEST_SHM_MODE = 0600;
constexpr auto TEST_SHM_FLAG = 0;

using TEST_MsgBaseFunc = std::function<MsgBase *()>;
static std::unordered_map<int16_t, TEST_MsgBaseFunc> gTestRequestMap = {
    {MXM_MSG_SHM_ALLOCATE,
     []() {
         return new (std::nothrow) ShmemAllocateRequest();
     }},
    {MXM_MSG_SHM_DEALLOCATE,
     []() {
         return new (std::nothrow) ShmemDeallocateRequest();
     }},
    {IPC_MALLOC_MEMORY,
     []() {
         return new (std::nothrow) AppMallocMemoryRequest();
     }},
    {IPC_FREE_RACKMEM,
     []() {
         return new (std::nothrow) AppFreeMemoryRequest();
     }},
    {IPC_QUERY_CLUSTERINFO,
     []() {
         return new (std::nothrow) CommonRequest();
     }},
    {IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS,
     []() {
         return new (std::nothrow) ShmLookRegionListRequest();
     }},
    {IPC_RACKMEMSHM_CREATE,
     []() {
         return new (std::nothrow) ShmCreateRequest();
     }},
    {IPC_RACKMEMSHM_DELETE,
     []() {
         return new (std::nothrow) ShmDeleteRequest();
     }},
    {IPC_RACKMEMSHM_MMAP,
     []() {
         return new (std::nothrow) ShmMapRequest();
     }},
    {IPC_RACKMEMSHM_UNMMAP,
     []() {
         return new (std::nothrow) ShmUnmapRequest();
     }},
    {IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS,
     []() {
         return new (std::nothrow) ShmQueryMemFaultStatusRequest();
     }},
    {IPC_RACKMEMSHM_WRITELOCK,
     []() {
         return new (std::nothrow) ShmWriteLockRequest();
     }},
    {IPC_RACKMEMSHM_READLOCK,
     []() {
         return new (std::nothrow) ShmReadLockRequest();
     }},
    {IPC_RACKMEMSHM_UNLOCK,
     []() {
         return new (std::nothrow) ShmUnLockRequest();
     }},
    {IPC_RACKMEMSHM_ATTACH,
     []() {
         return new (std::nothrow) ShmAttachRequest();
     }},
    {IPC_RACKMEMSHM_DETACH,
     []() {
         return new (std::nothrow) ShmDetachRequest();
     }},
    {IPC_RACKMEMSHM_LOOKUP_LIST,
     []() {
         return new (std::nothrow) ShmListLookupRequest();
     }},
    {IPC_RACKMEMSHM_LOOKUP,
     []() {
         return new (std::nothrow) ShmLookupRequest();
     }},
    {RPC_AGENT_QUERY_NODE_INFO,
     []() {
         return new (std::nothrow) CommonRequest();
     }},
    {RPC_DLOCK_CLIENT_REINIT,
     []() {
         return new (std::nothrow) DLockClientReinitRequest();
     }},
    {IPC_RACKMEMSHM_QUERY_NODE,
     []() {
         return new (std::nothrow) QueryNodeRequest();
     }},
    {IPC_RACKMEMSHM_QUERY_DLOCK_STATUS,
     []() {
         return new (std::nothrow) CommonRequest();
     }},
    {IPC_CHECK_MEMORY_LEASE,
     []() {
         return new (std::nothrow) CheckMemoryLeaseRequest();
     }},
    {IPC_CHECK_SHARE_MEMORY,
     []() {
         return new (std::nothrow) CheckShareMemoryMapRequest();
     }},
    {IPC_RACKMEMSHM_QUERY_SLOT_ID, []() {
         return new (std::nothrow) CommonRequest();
     }}};
static std::unordered_map<int16_t, TEST_MsgBaseFunc> gTestResponseMap = {
    {MXM_MSG_SHM_ALLOCATE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {MXM_MSG_SHM_DEALLOCATE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_MALLOC_MEMORY,
     []() {
         return new (std::nothrow) AppMallocMemoryResponse();
     }},
    {IPC_FREE_RACKMEM,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_QUERY_CLUSTERINFO,
     []() {
         return new (std::nothrow) AppQueryClusterInfoResponse();
     }},
    {IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS,
     []() {
         return new (std::nothrow) ShmLookRegionListResponse();
     }},
    {IPC_RACKMEMSHM_CREATE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_DELETE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_MMAP,
     []() {
         return new (std::nothrow) ShmMapResponse();
     }},
    {IPC_RACKMEMSHM_UNMMAP,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS,
     []() {
         return new (std::nothrow) ShmQueryMemFaultStatusResponse();
     }},
    {IPC_RACKMEMSHM_WRITELOCK,
     []() {
         return new (std::nothrow) ShmWriteLockResponse();
     }},
    {IPC_RACKMEMSHM_READLOCK,
     []() {
         return new (std::nothrow) ShmReadLockResponse();
     }},
    {IPC_RACKMEMSHM_UNLOCK,
     []() {
         return new (std::nothrow) ShmUnLockResponse();
     }},
    {IPC_RACKMEMSHM_ATTACH,
     []() {
         return new (std::nothrow) ShmAttachResponse();
     }},
    {IPC_RACKMEMSHM_DETACH,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_LOOKUP_LIST,
     []() {
         return new (std::nothrow) ShmListLookupResponse();
     }},
    {IPC_RACKMEMSHM_LOOKUP,
     []() {
         return new (std::nothrow) ShmLookupResponse();
     }},
    {RPC_AGENT_QUERY_NODE_INFO,
     []() {
         return new (std::nothrow) RpcQueryInfoResponse();
     }},
    {RPC_DLOCK_CLIENT_REINIT,
     []() {
         return new (std::nothrow) DLockClientReinitResponse();
     }},
    {IPC_RACKMEMSHM_QUERY_NODE,
     []() {
         return new (std::nothrow) QueryNodeResponse();
     }},
    {IPC_RACKMEMSHM_QUERY_DLOCK_STATUS,
     []() {
         return new (std::nothrow) QueryDlockStatusResponse();
     }},
    {IPC_CHECK_MEMORY_LEASE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_CHECK_SHARE_MEMORY,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_QUERY_SLOT_ID, []() {
         return new (std::nothrow) LookupSlotIdResponse();
     }}};

class MxmMessageTest : public testing::Test {
public:
    static void TearDownTestCase()
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void SetUp() override {}

    void TearDown() override {}
};

template <typename TMsg>
bool TestSerializeDeserialize()
{
    TMsg message{};
    NetMsgPacker packer;
    auto ret = message.Serialize(packer);
    if (ret != UBSM_OK) {
        return false;
    }
    NetMsgUnpacker unpacker(packer.String());

    TMsg messageNew;
    ret = messageNew.Deserialize(unpacker);
    if (ret != UBSM_OK) {
        return false;
    }
    return true; // 检查是否一致
}

TEST_F(MxmMessageTest, TestSerialize_ShmemAllocateRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmemAllocateRequest>();
    EXPECT_EQ(ret, true);
    ShmemAllocateRequest req("region1", "shm1", 4096, 0600, 0);
    EXPECT_EQ(req.regionName_, "region1");
    EXPECT_EQ(req.shmName_, "shm1");
    EXPECT_EQ(req.size_, 4096);
}

TEST_F(MxmMessageTest, TestSerialize_AppMallocMemoryWithLocRequest_Success)
{
    auto ret = TestSerializeDeserialize<AppMallocMemoryWithLocRequest>();
    EXPECT_EQ(ret, true);
    AppMallocMemoryWithLocRequest req(1024, true, 0, 1, 2, 3);
    EXPECT_EQ(req.size_, 1024);
    EXPECT_EQ(req.isNuma_, 1);
    EXPECT_EQ(req.slotId_, 0);
    EXPECT_EQ(req.socketId_, 1);
    EXPECT_EQ(req.numaId_, 2);
    EXPECT_EQ(req.portId_, 3);
}

TEST_F(MxmMessageTest, TestSerialize_ShmCreateWithProviderRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmCreateWithProviderRequest>();
    EXPECT_EQ(ret, true);
    ShmCreateWithProviderRequest req("host1", 0, 1, 2, "shm1", 4096, 0, 0600);
    EXPECT_EQ(req.hostName_, "host1");
    EXPECT_EQ(req.socketId_, 0);
    EXPECT_EQ(req.numaId_, 1);
    EXPECT_EQ(req.portId_, 2);
    EXPECT_EQ(req.name_, "shm1");
    EXPECT_EQ(req.size_, 4096);
}

TEST_F(MxmMessageTest, TestSerialize_CheckMemoryLeaseRequest_Success)
{
    auto ret = TestSerializeDeserialize<CheckMemoryLeaseRequest>();
    EXPECT_EQ(ret, true);
    std::vector<std::string> names{"lease1", "lease2"};
    CheckMemoryLeaseRequest req(names);
    EXPECT_EQ(req.names_.size(), 2);
}

TEST_F(MxmMessageTest, TestSerialize_CheckShareMemoryMapRequest_Success)
{
    auto ret = TestSerializeDeserialize<CheckShareMemoryMapRequest>();
    EXPECT_EQ(ret, true);
    std::vector<std::string> names{"shm1", "shm2"};
    CheckShareMemoryMapRequest req(names);
    EXPECT_EQ(req.names_.size(), 2);
}

TEST_F(MxmMessageTest, TestSerialize_LookupSlotIdResponse_Success)
{
    auto ret = TestSerializeDeserialize<LookupSlotIdResponse>();
    EXPECT_EQ(ret, true);
    LookupSlotIdResponse rsp(UBSM_OK, 42);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.slotId_, 42);
}

TEST_F(MxmMessageTest, TestSerialize_ShmemDeallocateRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmemDeallocateRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_AppMallocMemoryRequest_Success)
{
    auto ret = TestSerializeDeserialize<AppMallocMemoryRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_AppFreeMemoryRequest_Success)
{
    auto ret = TestSerializeDeserialize<AppFreeMemoryRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_AppMallocMemoryResponse_Success)
{
    auto ret = TestSerializeDeserialize<AppMallocMemoryResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_AppQueryClusterInfoResponse_Success)
{
    auto ret = TestSerializeDeserialize<AppQueryClusterInfoResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_AppQueryCachedMemoryResponse_Success)
{
    auto ret = TestSerializeDeserialize<AppQueryCachedMemoryResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookRegionListRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookRegionListRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmCreateRegionRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmCreateRegionRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookupRegionRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookupRegionRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmDestroyRegionRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmDestroyRegionRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmCreateRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmCreateRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmDeleteRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmDeleteRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmMapRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmMapRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmUnmapRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmUnmapRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmQueryMemFaultStatusRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmQueryMemFaultStatusRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_PingRequest_Success)
{
    auto ret = TestSerializeDeserialize<PingRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_VoteRequest_Success)
{
    auto ret = TestSerializeDeserialize<VoteRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_BroadcastRequest_Success)
{
    auto ret = TestSerializeDeserialize<BroadcastRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_TransElectedRequest_Success)
{
    auto ret = TestSerializeDeserialize<TransElectedRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookRegionListResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookRegionListResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmCreateRegionResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmCreateRegionResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookupRegionResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookupRegionResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmDestroyRegionResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmDestroyRegionResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmMapResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmMapResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmQueryMemFaultStatusResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmQueryMemFaultStatusResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmWriteLockRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmWriteLockRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmWriteLockResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmWriteLockResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmReadLockRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmReadLockRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmReadLockResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmReadLockResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmUnLockRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmUnLockRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmUnLockResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmUnLockResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_RpcQueryInfoResponse_Success)
{
    auto ret = TestSerializeDeserialize<RpcQueryInfoResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_DLockClientReinitRequest_Success)
{
    auto ret = TestSerializeDeserialize<DLockClientReinitRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_DLockClientReinitResponse_Success)
{
    auto ret = TestSerializeDeserialize<DLockClientReinitResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_LockRequest_Success)
{
    auto ret = TestSerializeDeserialize<LockRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_UnLockRequest_Success)
{
    auto ret = TestSerializeDeserialize<UnLockRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_DLockResponse_Success)
{
    auto ret = TestSerializeDeserialize<DLockResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_RpcJoinInfoResponse_Success)
{
    auto ret = TestSerializeDeserialize<RpcJoinInfoResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_RpcVoteInfoResponse_Success)
{
    auto ret = TestSerializeDeserialize<RpcVoteInfoResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmAttachRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmAttachRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmAttachResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmAttachResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmDetachRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmDetachRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookupRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookupRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmLookupResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmLookupResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_QueryNodeRequest_Success)
{
    auto ret = TestSerializeDeserialize<QueryNodeRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_QueryNodeResponse_Success)
{
    auto ret = TestSerializeDeserialize<QueryNodeResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_QueryDlockStatusResponse_Success)
{
    auto ret = TestSerializeDeserialize<QueryDlockStatusResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmListLookupRequest_Success)
{
    auto ret = TestSerializeDeserialize<ShmListLookupRequest>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestSerialize_ShmListLookupResponse_Success)
{
    auto ret = TestSerializeDeserialize<ShmListLookupResponse>();
    EXPECT_EQ(ret, true);
}

TEST_F(MxmMessageTest, TestCreateRequestByOpCode_Success)
{
    for (const auto &it : gTestRequestMap) {
        auto ret = CreateRequestByOpCode(it.first);
        EXPECT_NE(ret, nullptr);
        SafeDelete(ret);
    }
}

TEST_F(MxmMessageTest, CreateResponseByOpCode_Success)
{
    for (const auto &it : gTestResponseMap) {
        auto ret = CreateResponseByOpCode(it.first);
        EXPECT_NE(ret, nullptr);
        SafeDelete(ret);
    }
}

TEST_F(MxmMessageTest, TestCreateRequestByOpCodeInner_Success)
{
    std::vector<int16_t> ops{IPC_REGION_LOOKUP_REGION_LIST,
                             IPC_REGION_CREATE_REGION,
                             IPC_REGION_LOOKUP_REGION,
                             IPC_REGION_DESTROY_REGION,
                             IPC_FORCE_FREE_CACHED_MEMORY,
                             IPC_QUERY_CACHED_MEMORY,
                             RPC_PING_NODE_INFO,
                             RPC_JOIN_NODE_INFO,
                             RPC_MASTER_ELECTED_NODE_INFO,
                             RPC_SEND_ELECTED_MASTER_INFO,
                             RPC_VOTE_NODE_INFO,
                             RPC_BROADCAST_NODE_INFO,
                             RPC_LOCK,
                             RPC_UNLOCK};
    for (const auto &it : ops) {
        auto ret = CreateRequestByOpCodeInner(it);
        EXPECT_NE(ret, nullptr);
        SafeDelete(ret);
    }
    auto ret = CreateRequestByOpCodeInner(-1);
    EXPECT_EQ(ret, nullptr);
}

TEST_F(MxmMessageTest, CreateResponseByOpCodeInner_Success)
{
    std::vector<int16_t> ops{IPC_REGION_LOOKUP_REGION_LIST,
                             IPC_REGION_CREATE_REGION,
                             IPC_REGION_LOOKUP_REGION,
                             IPC_REGION_DESTROY_REGION,
                             IPC_FORCE_FREE_CACHED_MEMORY,
                             IPC_QUERY_CACHED_MEMORY,
                             RPC_PING_NODE_INFO,
                             RPC_JOIN_NODE_INFO,
                             RPC_MASTER_ELECTED_NODE_INFO,
                             RPC_SEND_ELECTED_MASTER_INFO,
                             RPC_VOTE_NODE_INFO,
                             RPC_BROADCAST_NODE_INFO,
                             RPC_LOCK,
                             RPC_UNLOCK};
    for (const auto &it : ops) {
        auto ret = CreateResponseByOpCodeInner(it);
        EXPECT_NE(ret, nullptr);
        SafeDelete(ret);
    }
    auto ret = CreateResponseByOpCodeInner(-1);
    EXPECT_EQ(ret, nullptr);
}

TEST_F(MxmMessageTest, TestCommonRequestWithInput)
{
    CommonRequest req(42);
    EXPECT_EQ(req.input_, 42);
    NetMsgPacker packer;
    EXPECT_EQ(req.Serialize(packer), UBSM_OK);
    NetMsgUnpacker unpacker(packer.String());
    CommonRequest req2;
    EXPECT_EQ(req2.Deserialize(unpacker), UBSM_OK);
    EXPECT_EQ(req2.input_, 42);
}

TEST_F(MxmMessageTest, TestCommonResponseWithInput)
{
    CommonResponse rsp(UBSM_ERR_MEMLIB);
    EXPECT_EQ(rsp.errCode_, UBSM_ERR_MEMLIB);
}

TEST_F(MxmMessageTest, TestBroadcastRequestWithValues)
{
    std::map<std::string, ock::rpc::ClusterNode> nodes;
    ock::rpc::ClusterNode node;
    node.id = "node1";
    nodes["node1"] = node;
    BroadcastRequest req("node1", nodes, true);
    EXPECT_EQ(req.nodeId_, "node1");
    EXPECT_EQ(req.isSeverInited_, true);
    EXPECT_EQ(req.nodes_.size(), 1);
}

TEST_F(MxmMessageTest, TestQueryNodeResponseWithValues)
{
    QueryNodeResponse rsp(UBSM_OK, "node1", true);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.nodeId_, "node1");
    EXPECT_EQ(rsp.nodeIsReady_, true);
}

TEST_F(MxmMessageTest, TestShmMapResponseWithValues)
{
    std::vector<uint64_t> ids{100, 200};
    ShmMapResponse rsp(UBSM_OK, ids, 4096, 512, 0, 0);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.memIds_.size(), 2);
    EXPECT_EQ(rsp.shmSize_, 4096);
    EXPECT_EQ(rsp.unitSize_, 512);
}

TEST_F(MxmMessageTest, TestShmCreateRequestWithValues)
{
    SHMRegionDesc desc;
    ShmCreateRequest req("region1", "shm1", 4096, "nid1", desc, 0, 0600);
    EXPECT_EQ(req.regionName_, "region1");
    EXPECT_EQ(req.name_, "shm1");
    EXPECT_EQ(req.size_, 4096);
    EXPECT_EQ(req.baseNid_, "nid1");
}

TEST_F(MxmMessageTest, TestVoteRequestWithValues)
{
    VoteRequest req("node1", "master1", 3);
    EXPECT_EQ(req.nodeId_, "node1");
    EXPECT_EQ(req.masterNode_, "master1");
    EXPECT_EQ(req.term_, 3);
}

TEST_F(MxmMessageTest, TestLockRequestWithValues)
{
    LockRequest req("mem1", true, 100, 200, 300);
    EXPECT_EQ(req.memName_, "mem1");
    EXPECT_EQ(req.isExclusive_, true);
    EXPECT_EQ(req.pid_, 100);
    EXPECT_EQ(req.uid_, 200);
    EXPECT_EQ(req.gid_, 300);
}

TEST_F(MxmMessageTest, TestUnLockRequestWithValues)
{
    UnLockRequest req("mem1", 100, 200, 300);
    EXPECT_EQ(req.memName_, "mem1");
    EXPECT_EQ(req.pid_, 100);
    EXPECT_EQ(req.uid_, 200);
    EXPECT_EQ(req.gid_, 300);
}

TEST_F(MxmMessageTest, TestDLockResponseWithValues)
{
    DLockResponse rsp(UBSM_OK, 0);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.dLockCode_, 0);
}

TEST_F(MxmMessageTest, TestRpcJoinInfoResponseWithValues)
{
    RpcJoinInfoResponse rsp(UBSM_OK, 1);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.nodetype_, 1);
}

TEST_F(MxmMessageTest, TestRpcVoteInfoResponseWithValues)
{
    RpcVoteInfoResponse rsp(UBSM_OK, "node1", true);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.name_, "node1");
    EXPECT_EQ(rsp.isGranted_, true);
}

TEST_F(MxmMessageTest, TestShmQueryMemFaultStatusResponseWithValues)
{
    ShmQueryMemFaultStatusResponse rsp(UBSM_OK, true);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.isMemFault_, true);
}

TEST_F(MxmMessageTest, TestTransElectedRequestWithValues)
{
    std::vector<std::string> nodes{"node1", "node2"};
    TransElectedRequest req("master", nodes, 5);
    EXPECT_EQ(req.nodeId_, "master");
    EXPECT_EQ(req.nodes_.size(), 2);
    EXPECT_EQ(req.term_, 5);
}

TEST_F(MxmMessageTest, TestDLockClientReinitResponseWithValues)
{
    DLockClientReinitResponse rsp(UBSM_OK, 0);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.dLockCode_, 0);
}

TEST_F(MxmMessageTest, TestShmLookupResponseWithValues)
{
    ubsmem_shmem_info_t info;
    ShmLookupResponse rsp(UBSM_OK, info);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
}

TEST_F(MxmMessageTest, TestShmCreateRegionRequestWithValues)
{
    SHMRegionDesc desc;
    ShmCreateRegionRequest req("region1", desc);
    EXPECT_EQ(req.regionName_, "region1");
}

TEST_F(MxmMessageTest, TestQueryDlockStatusResponseWithValues)
{
    QueryDlockStatusResponse rsp(UBSM_OK, true);
    EXPECT_EQ(rsp.errCode_, UBSM_OK);
    EXPECT_EQ(rsp.isReady_, true);
}
} // namespace UT