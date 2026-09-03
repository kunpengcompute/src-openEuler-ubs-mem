/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "rack_mem_lib_common.h"
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "ubs_mem.h"
#include "RackMemShm.h"
#include "app_ipc_stub.h"
#include "ipc_command.h"
#include "shm_ipc_command.h"

using namespace ock::mxmd;
using testing::Test;
#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))

namespace UT {
constexpr uint64_t MOCK_MEMORY_LEN = 4096;
class RackMemLibCommonTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(RackMemLibCommonTestSuite, TestCheckRackMemSizeWhichSizeLessThanZero)
{
    size_t shmSize = 0;
    auto ret = ock::mxmd::CheckRackMemSize(shmSize);
    ASSERT_FALSE(ret);
}

TEST_F(RackMemLibCommonTestSuite, TestCheckRackMemSizeWhichSizeIsNotIntegerIn4MB)
{
    size_t shmSize = 1.5 * 4 * 1024 * 1024;
    auto ret = ock::mxmd::CheckRackMemSize(shmSize);
    ASSERT_FALSE(ret);
}

TEST_F(RackMemLibCommonTestSuite, TestCheckRackMemSizeWhichSizeValid)
{
    size_t shmSize = 4 * 1024 * 1024;
    auto ret = ock::mxmd::CheckRackMemSize(shmSize);
    ASSERT_TRUE(ret);
}

TEST_F(RackMemLibCommonTestSuite, TestObmmOpenInternalFdGetterThanZero)
{
    mem_id id = 0;
    int flags = 0;
    int oflags = 0;
    ASSERT_NE(ock::mxmd::ObmmOpenInternal(id, flags, oflags), 0);
}

TEST_F(RackMemLibCommonTestSuite, TestObmmOpenInternalOpenFlagChanged)
{
    mem_id id = 0;
    int flags = 2;
    int oflags = 0;
    ASSERT_NE(ock::mxmd::ObmmOpenInternal(id, flags, oflags), 0);
}

TEST_F(RackMemLibCommonTestSuite, ipc_command_cpp)
{
    size_t size{};
    PerfLevel level{};
    std::vector<uint64_t> memIds;
    size_t uintSize{};
    int64_t numaId{};
    bool isNuma{false};
    std::string name{};
    auto request = std::make_shared<AppMallocMemoryRequest>(MOCK_MEMORY_LEN, PerfLevel::L0, isNuma, "default");
    auto response = std::make_shared<AppMallocMemoryResponse>();
    QueryLocalBorrowMemInfo borrowInfo;
    std::vector<std::string> recordInf;
    ubsmem_cluster_info_t clusterInfo{};

    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmComIpcClientSendStub));

    ASSERT_EQ(IpcCommand::AppMallocMemory(request, response), 0);
    memIds = response->memIds_;
    name = response->name_;
    ASSERT_EQ(IpcCommand::AppFreeMemory("", name, memIds), 0);
    ASSERT_EQ(IpcCommand::IpcRackMemLookupClusterStatistic(&clusterInfo), 0);
    ASSERT_EQ(IpcCommand::AppQueryLeaseRecord(recordInf), 0);
    ASSERT_EQ(IpcCommand::AppForceFreeCachedMemory(), 0);
    ASSERT_EQ(IpcCommand::AppCheckLeaseMemoryResource(), 0);
}

TEST_F(RackMemLibCommonTestSuite, RackMemShmCpp)
{
    SHMRegions list{};
    SHMRegionDesc region{};
    ubsmem_shmem_info_t shm_info;
    std::vector<ubsmem_shmem_desc_t> shm_list;
    std::string name;
    bool isNodeReady;
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmUnmmap(nullptr, 0), MXM_ERR_PARAM_INVALID);
    RackMemShm::GetInstance().UbsMemShmDelete("name");
    ASSERT_NE(RackMemShm::GetInstance().CheckRegionPar("name", {}), 0);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemLookupResourceRegions("name", ShmRegionType::ALL2ALL_SHARE, list),
              MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemCreateResourceRegion("name", {}), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemLookupResourceRegion("name", region), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemDestroyResourceRegion("name"), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmWriteLock("name"), MXM_ERR_SHM_NOT_FOUND);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmReadLock("name"), MXM_ERR_SHM_NOT_FOUND);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmUnLock("name"), MXM_ERR_SHM_NOT_FOUND);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmAttach("name"), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmDetach("name"), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmListLookup("name", shm_list), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmLookup("name", shm_info), MXM_ERR_IPC_HCOM_INNER_SYNC_CALL);
    ASSERT_EQ(RackMemShm::GetInstance().RpcQueryInfo(name), MXM_ERR_RPC_QUERY_ERROR);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemQueryNode(name, isNodeReady, false), MXM_ERR_MEMLIB);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemQueryDlockStatus(isNodeReady), MXM_ERR_MEMLIB);
}

TEST_F(RackMemLibCommonTestSuite, RackMemShmCppStub)
{
    SHMRegions list{};
    std::string name;
    bool isNodeReady;
    SHMRegionDesc region{};
    ubsmem_shmem_info_t shm_info;
    std::vector<ubsmem_shmem_desc_t> shm_list;

    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmShmIpcClientSendStub));

    ASSERT_EQ(RackMemShm::GetInstance().RackMemLookupResourceRegions("name", ShmRegionType::ALL2ALL_SHARE, list), 0);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemCreateResourceRegion("name", {}), 0);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemLookupResourceRegion("name", region), 0);
    ASSERT_EQ(RackMemShm::GetInstance().RackMemDestroyResourceRegion("name"), 0);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmAttach("name"), MXM_ERR_SHM_NOT_EXIST);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmDetach("name"), 0);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmListLookup("name", shm_list), 0);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemShmLookup("name", shm_info), MXM_ERR_SHM_NOT_FOUND);
    ASSERT_EQ(RackMemShm::GetInstance().RpcQueryInfo(name), 0);
    ASSERT_EQ(RackMemShm::GetInstance().UbsMemQueryNode(name, isNodeReady, false), 0);
    ASSERT_EQ(isNodeReady, false);
}

TEST_F(RackMemLibCommonTestSuite, mxmem_shmem_cpp)
{
    SHMRegions list{};
    SHMRegionDesc region{};
    ubsmem_shmem_info_t shm_info;
    std::vector<ubsmem_shmem_desc_t> shm_list;
    std::string name;
    bool isNodeReady;
    ubsmem_shmem_lookup("", nullptr);
    ASSERT_EQ(ubsmem_shmem_list_lookup("", nullptr, nullptr), UBSM_ERR_PARAM_INVALID);
}

TEST_F(RackMemLibCommonTestSuite, shm_ipc_command_cpp)
{
    std::string name{"name"};
    uint64_t mapSize{};
    std::vector<uint64_t> memIds;
    size_t shmSize{};
    size_t unitSize{};
    uint64_t flag{};
    int oflag{};
    mode_t mode{};
    int prot{};
    std::string regionName{"name"};
    uint64_t size{};
    std::string nid{};
    SHMRegionDesc shmRegionDesc{};
    uint64_t flags{};
    std::string baseNid = "name";
    ShmRegionType type{};
    SHMRegions list{};
    SHMRegionDesc desc{};
    SHMRegionInfo shmRegionInfo{};
    QueryLocalBorrowMemInfo borrowInfo{};
    SHMRegionDesc region{};
    std::vector<ubsmem_shmem_desc_t> shm_list;
    ubsmem_shmem_info_t shm_info{};
    bool isNodeReady{false};
    ShmIpcCommand::IpcCallShmMap(name, mapSize, memIds, shmSize, unitSize, flag, oflag, mode, prot);
    ShmIpcCommand::IpcCallShmCreate(regionName, name, size, nid, shmRegionDesc, flags, mode);
    ShmIpcCommand::IpcCallShmDelete(name);
    ShmIpcCommand::IpcCallShmLookRegionList(baseNid, type, list);
    ShmIpcCommand::IpcCallShmUnMap(name);
    ShmIpcCommand::IpcShmemReadLock(name);
    ShmIpcCommand::IpcShmemUnLock(name);
    ShmIpcCommand::IpcCallRpcQuery(name);
    ShmIpcCommand::IpcCallLookupRegionList(baseNid, type, list);
    ShmIpcCommand::IpcCallCreateRegion(name, region);
    ShmIpcCommand::IpcCallLookupRegion(name, region);
    ShmIpcCommand::IpcCallDestroyRegion(name);
    ShmIpcCommand::IpcShmemWriteLock(name);
    ShmIpcCommand::IpcCallShmAttach(name);
    ShmIpcCommand::IpcCallShmDetach(name);
    ShmIpcCommand::IpcCallShmListLookup(name, shm_list);
    ShmIpcCommand::IpcCallShmLookup(name, shm_info);
    ShmIpcCommand::IpcQueryNode(name, isNodeReady, false);

    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmComIpcClientSendStub));
    ASSERT_EQ(ShmIpcCommand::IpcQueryDlockStatus(isNodeReady), 0);
}

TEST_F(RackMemLibCommonTestSuite, ShmMetaDataMgrCppInvalid)
{
    uint64_t usedFirst = 1u;
    std::vector<int> indices{1u};
    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Init(), 0);
    ShmAppMetaData meta("name", {1u}, reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, ShmOwnStatus::UNACCESS, 0);
    ShmMetaDataMgr::GetInstance().UpdateMetaData("name", meta);
    ShmMetaDataMgr::GetInstance().GetMetaIsLockAddress("name");
    ShmMetaDataMgr::GetInstance().GetMetaHasUnmapped("name");
    ShmMetaDataMgr::GetInstance().SetMetaHasUnmapped("name", true);
    ShmMetaDataMgr::GetInstance().SetMetaIsLockAddress("name", true);
    ShmMetaDataMgr::GetInstance().GetShmMetaFromName("name", meta);
    ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, meta, usedFirst,
                                            indices);
    auto ret = ShmMetaDataMgr::GetInstance().CheckNameExistAndGet(nullptr, 0, meta);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    ret = ShmMetaDataMgr::GetInstance().CheckNameExistAndGet(reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, meta);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);

    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(nullptr, "name");
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);
    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(reinterpret_cast<void *>(1u), "");
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(reinterpret_cast<void *>(1u), "name");
    ASSERT_EQ(ret, MXM_ERR_MEMLIB);

    std::vector<std::string> names;
    ret = ShmMetaDataMgr::GetInstance().GetAllUsedMemoryNames(names);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().AddMetaData("name", nullptr, meta);
    ASSERT_EQ(ret, MXM_ERR_NULLPTR);

    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Destroy(), 0);
}

TEST_F(RackMemLibCommonTestSuite, ShmMetaDataMgrCppOk)
{
    uint64_t usedFirst = 1u;
    std::vector<int> indices{1u};
    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Init(), 0);
    ShmAppMetaData meta("name", {1u}, reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, ShmOwnStatus::UNACCESS, 0);
    meta.addrs.push_back(std::make_pair(reinterpret_cast<void *>(1u), 1u));
    meta.addrs.push_back(std::make_pair(reinterpret_cast<void *>(2u), 2u));

    meta.unitSize = 1024u;
    auto ret = ShmMetaDataMgr::GetInstance().AddMetaData("name", reinterpret_cast<void *>(1u), meta);
    ASSERT_EQ(ret, 0);
    /* 重复添加 */
    ret = ShmMetaDataMgr::GetInstance().AddMetaData("name", reinterpret_cast<void *>(1u), meta);
    ASSERT_EQ(ret, 0);

    void *addr = ShmMetaDataMgr::GetInstance().GetAddr("name");
    ASSERT_EQ(addr, reinterpret_cast<void *>(1u));

    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(1u), 1, meta, usedFirst, indices);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().CheckNameExistAndGet(reinterpret_cast<void *>(1u), 1u, meta);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);

    ret = ShmMetaDataMgr::GetInstance().CheckNameExistAndGet(reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, meta);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().GetShmMetaFromName("name", meta);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().SetMetaHasUnmapped("name", true);
    ASSERT_EQ(ret, 0);

    auto hasUnmapped = ShmMetaDataMgr::GetInstance().GetMetaHasUnmapped("name");
    ASSERT_EQ(hasUnmapped, true);

    ret = ShmMetaDataMgr::GetInstance().SetMetaIsLockAddress("name", true);
    ASSERT_EQ(ret, 0);

    auto isLock = ShmMetaDataMgr::GetInstance().GetMetaIsLockAddress("name");
    ASSERT_EQ(isLock, true);

    std::vector<std::string> names;
    ret = ShmMetaDataMgr::GetInstance().GetAllUsedMemoryNames(names);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().UpdateMetaData("name", meta);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(reinterpret_cast<void *>(1u), "name");
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Destroy(), 0);
}

TEST_F(RackMemLibCommonTestSuite, ShmMetaDataMgrCheckAddrUnok)
{
    uint64_t usedFirst = 1u;
    std::vector<int> indices{1u};
    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Init(), 0);
    ShmAppMetaData meta("name", {1u}, reinterpret_cast<void *>(1u), 128ULL * 1024 * 1024, ShmOwnStatus::UNACCESS, 0);
    meta.addrs.push_back(std::make_pair(reinterpret_cast<void *>(1u), 1u));
    meta.addrs.push_back(std::make_pair(reinterpret_cast<void *>(2u), 2u));

    meta.unitSize = 1024u;
    auto ret = ShmMetaDataMgr::GetInstance().AddMetaData("name", reinterpret_cast<void *>(1u), meta);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", nullptr, 1, meta, usedFirst, indices);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(2u), 1, meta, usedFirst, indices);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    meta.addr = reinterpret_cast<void *>(2u);
    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(2u), 1, meta, usedFirst, indices);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    meta.addr = reinterpret_cast<void *>(1u);
    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(2u), 256ULL * 1024 * 1024, meta,
                                                  usedFirst, indices);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);

    meta.unitSize = 0;
    ret = ShmMetaDataMgr::GetInstance().AddMetaData("name", reinterpret_cast<void *>(1u), meta);
    ASSERT_EQ(ret, 0);

    ret = ShmMetaDataMgr::GetInstance().CheckAddr("name", reinterpret_cast<void *>(1u), 1, meta, usedFirst, indices);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);

    ret = ShmMetaDataMgr::GetInstance().RemoveMetaData(reinterpret_cast<void *>(1u), "name");
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(ShmMetaDataMgr::GetInstance().Destroy(), 0);
}

} // namespace UT
