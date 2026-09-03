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
#include "mxm_msg.h"
#include <functional>
#include "message_op.h"

namespace ock::mxmd {
using MsgBaseFunc = std::function<MsgBase *()>;

static std::unordered_map<int16_t, MsgBaseFunc> requestMap = {
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
    {IPC_MALLOC_MEMORY_LOC,
     []() {
         return new (std::nothrow) AppMallocMemoryWithLocRequest();
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
    {IPC_RACKMEMSHM_CREATE_WITH_PROVIDER,
     []() {
         return new (std::nothrow) ShmCreateWithProviderRequest();
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
    {IPC_CHECK_SHARE_MEMORY,
     []() {
         return new (std::nothrow) CheckShareMemoryMapRequest();
     }},
    {IPC_CHECK_MEMORY_LEASE,
     []() {
         return new (std::nothrow) CheckMemoryLeaseRequest();
     }},
    {IPC_RACKMEMSHM_QUERY_SLOT_ID,
     []() {
         return new (std::nothrow) CommonRequest();
     }},
};
static std::unordered_map<int16_t, MsgBaseFunc> responseMap = {
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
    {IPC_MALLOC_MEMORY_LOC,
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
    {IPC_RACKMEMSHM_CREATE_WITH_PROVIDER,
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
    {IPC_CHECK_SHARE_MEMORY,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_CHECK_MEMORY_LEASE,
     []() {
         return new (std::nothrow) CommonResponse();
     }},
    {IPC_RACKMEMSHM_QUERY_SLOT_ID,
     []() {
         return new (std::nothrow) LookupSlotIdResponse();
     }},
};

MsgBase *CreateRequestByOpCode(int16_t op)
{
    auto it = requestMap.find(op);
    if (it != requestMap.end()) {
        return it->second();
    }
    return CreateRequestByOpCodeInner(op);
}

MsgBase *CreateResponseByOpCode(int16_t op)
{
    auto it = responseMap.find(op);
    if (it != responseMap.end()) {
        return it->second();
    }
    return CreateResponseByOpCodeInner(op);
}

MsgBase *CreateRequestByOpCodeInner(int16_t op)
{
    switch (op) {
        case IPC_REGION_LOOKUP_REGION_LIST:
            return new (std::nothrow) ShmLookRegionListRequest();
        case IPC_REGION_CREATE_REGION:
            return new (std::nothrow) ShmCreateRegionRequest();
        case IPC_REGION_LOOKUP_REGION:
            return new (std::nothrow) ShmLookupRegionRequest();
        case IPC_REGION_DESTROY_REGION:
            return new (std::nothrow) ShmDestroyRegionRequest();
        case IPC_FORCE_FREE_CACHED_MEMORY:
            return new (std::nothrow) CommonRequest();
        case IPC_QUERY_CACHED_MEMORY:
            return new (std::nothrow) CommonRequest();
        case RPC_PING_NODE_INFO:
            return new (std::nothrow) PingRequest();
        case RPC_JOIN_NODE_INFO:
            return new (std::nothrow) PingRequest();
        case RPC_MASTER_ELECTED_NODE_INFO:
            return new (std::nothrow) BroadcastRequest();
        case RPC_SEND_ELECTED_MASTER_INFO:
            return new (std::nothrow) TransElectedRequest();
        case RPC_VOTE_NODE_INFO:
            return new (std::nothrow) VoteRequest();
        case RPC_BROADCAST_NODE_INFO:
            return new (std::nothrow) BroadcastRequest();
        case RPC_LOCK:
            return new (std::nothrow) LockRequest();
        case RPC_UNLOCK:
            return new (std::nothrow) UnLockRequest();
        default:
            break;
    }
    return nullptr;
}

MsgBase *CreateResponseByOpCodeInner(int16_t op)
{
    switch (op) {
        case IPC_REGION_LOOKUP_REGION_LIST:
            return new (std::nothrow) ShmLookRegionListResponse();
        case IPC_REGION_CREATE_REGION:
            return new (std::nothrow) ShmCreateRegionResponse();
        case IPC_REGION_LOOKUP_REGION:
            return new (std::nothrow) ShmLookupRegionResponse();
        case IPC_REGION_DESTROY_REGION:
            return new (std::nothrow) ShmDestroyRegionResponse();
        case IPC_FORCE_FREE_CACHED_MEMORY:
            return new (std::nothrow) CommonResponse();
        case IPC_QUERY_CACHED_MEMORY:
            return new (std::nothrow) AppQueryCachedMemoryResponse();
        case RPC_PING_NODE_INFO:
            return new (std::nothrow) RpcQueryInfoResponse();
        case RPC_JOIN_NODE_INFO:
            return new (std::nothrow) RpcJoinInfoResponse();
        case RPC_MASTER_ELECTED_NODE_INFO:
            return new (std::nothrow) RpcQueryInfoResponse();
        case RPC_SEND_ELECTED_MASTER_INFO:
            return new (std::nothrow) RpcQueryInfoResponse();
        case RPC_VOTE_NODE_INFO:
            return new (std::nothrow) RpcVoteInfoResponse();
        case RPC_BROADCAST_NODE_INFO:
            return new (std::nothrow) RpcQueryInfoResponse();
        case RPC_LOCK:
            return new (std::nothrow) DLockResponse();
        case RPC_UNLOCK:
            return new (std::nothrow) DLockResponse();
        default:
            break;
    }
    return nullptr;
}
} // namespace ock::mxmd