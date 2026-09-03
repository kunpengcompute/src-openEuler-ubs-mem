/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "shm_ipc_handler_test.h"
#include <fcntl.h>
#include "mxm_shm/ipc_handler.h"
#include "record_store.h"
#include "region_manager.h"
#include "region_repository.h"
#include "rpc_server.h"
#include "shm_manager.h"
#include "ubse_mem_adapter_stub.h"
#include "ubsm_lock.h"
#include "zen_discovery.h"

using namespace UT;
using namespace ock::share::service;

TEST_F(ShmIpcHandlerTestSuite, TestShmemNull)
{
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmMap(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmUnmap(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmDelete(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::RegionLookupRegionList(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::RegionCreateRegion(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::RegionLookupRegion(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::RegionDestroyRegion(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmWriteLock(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmReadLock(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmUnLock(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmQueryNode(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmQueryDlockStatus(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmAttach(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmDetach(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmListLookup(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::ShmLookup(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = MxmServerMsgHandle::RpcQueryNodeInfo(nullptr, nullptr, info);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmMapFail)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<ShmMapRequest>("default", "", 1, 0, 1);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmMapResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmMap(request.get(), response.get(), info);
    ASSERT_NE(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmMapFail01)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(-1));

    auto request = std::make_shared<ShmMapRequest>("default", "", 1, 0, 1);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmMapResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmMap(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmMapHandleExistingShm)
{
    SHMManager::GetInstance().PrepareAddShareMemoryInfo("xxx", 134217728, 111);
    SHMManager::GetInstance().AddFullShareMemoryInfo("xxx", 111,
                                                     {.memIds = {1},
                                                      .unitSize = 134217728,
                                                      .createFlags = 0,
                                                      .openFlag = O_RDWR,
                                                      .ownerUserId = 0,
                                                      .ownerGroupId = 0});
    MOCKER_CPP(&SHMManager::AddMemoryUserInfo, int32_t(*)(const std::string &name, pid_t)).stubs().will(returnValue(0));

    auto request = std::make_shared<ShmMapRequest>("default", "xxx", 134217728, 0, 1);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmMapResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmMap(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
    SHMManager::GetInstance().RemoveMemoryInfo("xxx");
}

TEST_F(ShmIpcHandlerTestSuite, TestShmUnmap)
{
    MOCKER_CPP(&SHMManager::GetShareMemoryInfo, int32_t(*)(const std::string &name, AttachedShareMemory &memory))
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&SHMManager::GetMemoryUsersCountByName, int32_t(*)(const std::string &name, size_t &count))
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&SHMManager::GetShareMemoryInfo, int32_t(*)(const std::string &name, AttachedShareMemory &memory))
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&SHMManager::RemoveMemoryUserInfo, int32_t(*)(const std::string &name, pid_t))
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&SHMManager::RemoveMemoryInfo, int32_t(*)(const std::string &name)).stubs().will(returnValue(0));

    MOCKER_CPP(&SHMManager::UpdateShareMemoryRecordState,
               int32_t(*)(const std::string &name, ock::ubsm::RecordState state))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<ShmUnmapRequest>("default", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmUnmap(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmUnmapFail)
{
    auto request = std::make_shared<ShmUnmapRequest>("default", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmUnmap(request.get(), response.get(), info);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmQueryMemFaultStatus)
{
    auto request = std::make_shared<ShmQueryMemFaultStatusRequest>("default");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmQueryMemFaultStatusResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
}

TEST_F(ShmIpcHandlerTestSuite, TestShmQueryMemFaultStatusFail)
{
    auto request = std::make_shared<ShmQueryMemFaultStatusRequest>("default");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmQueryMemFaultStatusResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionLookupRegionList)
{
    MOCKER(ock::mxm::UbseMemAdapter::LookupRegionList).stubs().will(returnValue(0));
    auto request = std::make_shared<ShmLookRegionListRequest>("default", 0);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookRegionListResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionLookupRegionList(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionLookupRegionListFail)
{
    MOCKER(ock::mxm::UbseMemAdapter::LookupRegionList).stubs().will(returnValue(-1));
    auto request = std::make_shared<ShmLookRegionListRequest>("default", 0);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookRegionListResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionLookupRegionList(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionCreateRegion)
{
    SHMRegionDesc region = {};
    MOCKER(ock::mxm::UbseMemAdapter::LookupRegionList).stubs().will(returnValue(0));
    auto request = std::make_shared<ShmCreateRegionRequest>("default", region);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmCreateRegionResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionCreateRegion(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionLookupRegionFail)
{
    auto request = std::make_shared<ShmLookupRegionRequest>("name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookupRegionResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionLookupRegion(request.get(), response.get(), info);
    ASSERT_EQ(ret, MXM_ERR_REGION_NOT_EXIST);
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionLookupRegionSuccess)
{
    SHMRegionDesc region = {};
    RegionInfo regionInfo{"name", 0, region};
    RegionManager::GetInstance().CreateRegionInfo(regionInfo);
    auto request = std::make_shared<ShmLookupRegionRequest>("name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookupRegionResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionLookupRegion(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestRegionDestroyRegion)
{
    auto request = std::make_shared<ShmDestroyRegionRequest>("name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmDestroyRegionResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RegionDestroyRegion(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmWriteLock)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Lock,
               int32_t(*)(const std::string &name, bool isExclusive, ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<ShmWriteLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmWriteLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmWriteLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmWriteLockFail)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Lock,
               int32_t(*)(const std::string &name, bool isExclusive, ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<ShmWriteLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmWriteLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmWriteLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmReadLock)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Lock,
               int32_t(*)(const std::string &name, bool isExclusive, ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<ShmReadLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmReadLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmReadLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmReadLockFail)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Lock,
               int32_t(*)(const std::string &name, bool isExclusive, ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(-1));

    auto request = std::make_shared<ShmReadLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmReadLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmReadLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmUnLock)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Unlock,
               int32_t(*)(const std::string &name, const ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(0));

    auto request = std::make_shared<ShmUnLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmUnLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmUnLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmUnLockFail)
{
    MOCKER_CPP(&ock::dlock_utils::UbsmLock::Unlock,
               int32_t(*)(const std::string &name, const ock::dlock_utils::LockUdsInfo &udsInfo))
        .stubs()
        .will(returnValue(-1));

    auto request = std::make_shared<ShmUnLockRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmUnLockResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmUnLock(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmQueryNodeNull)
{
    auto request = std::make_shared<QueryNodeRequest>(true);
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<QueryNodeResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmQueryNode(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(response->errCode_, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmAttachFail)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<ShmAttachRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmAttachResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmAttach(request.get(), response.get(), info);
    ASSERT_EQ(ret, MXM_ERR_SHM_PERMISSION_DENIED);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmAttachFail02)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<ShmAttachRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmAttachResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmAttach(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmDetachFail)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<ShmDetachRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmDetach(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmDetachFail01)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData,
               int (*)(const std::string &name, ock::mxm::ubse_user_info_t &ubsUserInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<ShmDetachRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmDetach(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmListLookup)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmListLookup,
               int (*)(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &nameList))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<ShmListLookupRequest>("regionName", "name");
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmListLookupResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmListLookup(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmLookup)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmLookup, int (*)(const std::string &name, ubsmem_shmem_info_t &shmInfo))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<ShmLookupRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookupResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmLookup(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmLookupFail)
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmLookup, int (*)(const std::string &name, ubsmem_shmem_info_t &shmInfo))
        .stubs()
        .will(returnValue(-1));
    auto request = std::make_shared<ShmLookupRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<ShmLookupResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmLookup(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}

TEST_F(ShmIpcHandlerTestSuite, TestRpcQueryNodeInfo)
{
    MOCKER_CPP(&ock::rpc::service::RpcServer::SendMsg,
               int32_t(*)(uint16_t opCode, MsgBase * req, MsgBase * rsp, const std::string &nodeId))
        .stubs()
        .will(returnValue(0));
    auto request = std::make_shared<CommonRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<RpcQueryInfoResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::RpcQueryNodeInfo(request.get(), response.get(), info);
    ASSERT_EQ(ret, 0);
}

TEST_F(ShmIpcHandlerTestSuite, TestShmDelete)
{
    MOCKER_CPP(&SHMManager::GetMemoryUsersCountByName, int32_t(*)(const std::string &name, size_t &count))
        .stubs()
        .will(returnValue(-1));
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmGetUserData, int (*)(const std::string &name, ubs_mem_shm_desc_t &shmDes))
        .stubs()
        .will(invoke(ShmGetUserDataStub));
    MOCKER(memcpy_s).stubs().will(returnValue(0));
    auto request = std::make_shared<ShmDeleteRequest>();
    if (request == nullptr) {
        ASSERT_EQ(0, -1);
    }
    auto response = std::make_shared<CommonResponse>();
    if (response == nullptr) {
        ASSERT_EQ(0, -1);
    }
    MxmComUdsInfo info = {};
    auto ret = MxmServerMsgHandle::ShmDelete(request.get(), response.get(), info);
    ASSERT_EQ(ret, -1);
    ASSERT_EQ(response->errCode_, -1);
}