/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include <gtest/gtest.h>
#include <numaif.h>
#include <mockcpp/mockcpp.hpp>

#include "RackMem.h"
#include "ipc_command.h"
#include "rack_mem_fd_map.h"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
namespace UT {
using namespace ock::mxmd;
constexpr uint64_t MOCK_FILE_LEN = 4096;

class RackMemTestSuite : public testing::Test {
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(RackMemTestSuite, TestMemoryIDUsedByNumaWithNumaIDLessThanZero)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    int64_t numaId = -1;
    size_t unitSize = 8;
    std::string cachedName = "mockName";

    auto ret = RackMem::GetInstance().MemoryIDUsedByNuma(desc, memIds, numaId, unitSize, cachedName);
    ASSERT_EQ(ret, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByNumaMmapFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    int64_t numaId = 0;
    size_t unitSize = 8;
    std::string cachedName = "mockName";

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue((void *)-1));

    auto ret = RackMem::GetInstance().MemoryIDUsedByNuma(desc, memIds, numaId, unitSize, cachedName);
    ASSERT_EQ(ret, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByNumaMbindFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    int64_t numaId = 0;
    size_t unitSize = 8;
    std::string cachedName = "mockName";

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue((void *)0x12345678));
    MOCKER(SystemAdapter::MemoryBind).stubs().will(returnValue(-1));
    MOCKER(SystemAdapter::MemoryUnMap).stubs().with(any(), any()).will(returnValue(0));

    auto ret = RackMem::GetInstance().MemoryIDUsedByNuma(desc, memIds, numaId, unitSize, cachedName);
    ASSERT_EQ(ret, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByNumaStorageMetaFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    int64_t numaId = 0;
    size_t unitSize = 8;
    std::string cachedName = "mockName";

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue((void *)0x12345678));
    MOCKER(SystemAdapter::MemoryBind).stubs().will(returnValue(0));
    MOCKER_CPP(&BorrowAppMetaMgr::AddMeta, uint32_t(*)(const void *addr, const AppBorrowMetaDesc &desc))
        .stubs()
        .will(returnValue(1));

    auto ret = RackMem::GetInstance().MemoryIDUsedByNuma(desc, memIds, numaId, unitSize, cachedName);
    ASSERT_EQ(ret, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByNumaAllSuccess)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    int64_t numaId = 0;
    size_t unitSize = 8;
    std::string cachedName = "mockName";

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue((void *)0x12345678));
    MOCKER(SystemAdapter::MemoryBind).stubs().will(returnValue(0));
    MOCKER_CPP(&BorrowAppMetaMgr::AddMeta, uint32_t(*)(const void *addr, const AppBorrowMetaDesc &desc))
        .stubs()
        .will(returnValue(0));

    auto ret = RackMem::GetInstance().MemoryIDUsedByNuma(desc, memIds, numaId, unitSize, cachedName);
    ASSERT_EQ(ret, (void *)0x12345678);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByFdFileMapByTotalSizeFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    size_t unitSize = 8;
    std::string name = "test";

    MOCKER_CPP(&RackMemFdMap::FileMapByTotalSize, uint32_t(*)(uint64_t, void *&, int))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(1));

    auto result = RackMem::GetInstance().MemoryIDUsedByFd(desc, memIds, unitSize, name, 0);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByFdObmmOpenFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    size_t unitSize = 8;
    std::string name = "test";

    // Mock FileMapByTotalSize to succeed
    MOCKER_CPP(&RackMemFdMap::FileMapByTotalSize, uint32_t(*)(uint64_t, void *&, int))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(0));

    MOCKER(ObmmOpenInternal).stubs().with(any(), any()).will(returnValue(-1));

    MOCKER(SystemAdapter::MemoryUnMap).stubs().with(any(), any()).will(returnValue(0));

    auto result = RackMem::GetInstance().MemoryIDUsedByFd(desc, memIds, unitSize, name, 0);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByFdMapForEachFdFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    size_t unitSize = 8;
    std::string name = "test";

    MOCKER_CPP(&RackMemFdMap::FileMapByTotalSize, uint32_t(*)(uint64_t, void *&, int))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(0));

    MOCKER(ObmmOpenInternal).stubs().with(any(), any()).will(returnValue(0x123));

    MOCKER_CPP(&RackMemFdMap::MapForEachFd, uint32_t(*)(void *, size_t, int, int, int, off_t))
        .stubs()
        .will(returnValue(1));

    MOCKER(SystemAdapter::MemoryUnMap).stubs().with(any(), any()).will(returnValue(0));

    auto result = RackMem::GetInstance().MemoryIDUsedByFd(desc, memIds, unitSize, name, 0);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByFdAddMetaFailed)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    size_t unitSize = 8;
    std::string name = "test";

    MOCKER_CPP(&RackMemFdMap::FileMapByTotalSize, uint32_t(*)(uint64_t, void *&, int))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(0));

    MOCKER(ObmmOpenInternal).stubs().with(any(), any()).will(returnValue(0x123));
    MOCKER_CPP(&RackMemFdMap::MapForEachFd, uint32_t(*)(void *, size_t, int, int, int, off_t))
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&BorrowAppMetaMgr::AddMeta, uint32_t(*)(const void *, const AppBorrowMetaDesc &))
        .stubs()
        .will(returnValue(1));

    MOCKER(SystemAdapter::MemoryUnMap).stubs().with(any(), any()).will(returnValue(0));

    auto result = RackMem::GetInstance().MemoryIDUsedByFd(desc, memIds, unitSize, name, 0);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestMemoryIDUsedByFdSuccess)
{
    AppBorrowMetaDesc desc{4};
    std::vector<uint64_t> memIds = {1, 2, 3};
    size_t unitSize = 8;
    std::string name = "test";
    void *expectedAddr = reinterpret_cast<void *>(0x12345678);

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue((void *)0x12345678));
    MOCKER(ObmmOpenInternal).stubs().with(any(), any()).will(returnValue(0x123));
    MOCKER_CPP(&RackMemFdMap::MapForEachFd, uint32_t(*)(void *, size_t, int, int, int, off_t))
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&BorrowAppMetaMgr::AddMeta, uint32_t(*)(const void *, const AppBorrowMetaDesc &))
        .stubs()
        .will(returnValue(0));

    auto result = RackMem::GetInstance().MemoryIDUsedByFd(desc, memIds, unitSize, name, 0);
    ASSERT_EQ(result, expectedAddr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocAppMallocMemoryFailed)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = 0;

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(returnValue(1));

    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocNonNumaMemIdsEmpty)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = 0;

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(invoke(+[](std::shared_ptr<AppMallocMemoryRequest> &req,
                         std::shared_ptr<AppMallocMemoryResponse> &resp) -> uint32_t {
            resp->memIds_ = {};
            resp->unitSize_ = 8; // 8 为模拟的 uintSize
            resp->name_ = "test_name";
            return 0;
        }));

    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocNumaPathFailed)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = UBSM_FLAG_MALLOC_WITH_NUMA;

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(invoke(+[](const std::shared_ptr<AppMallocMemoryRequest> &req,
                         std::shared_ptr<AppMallocMemoryResponse> &resp) -> uint32_t {
            resp->memIds_ = {1, 2, 3};
            resp->unitSize_ = 8; // 8 为模拟的 uintSize
            resp->numaId_ = 0;
            resp->name_ = "test_name";
            return 0;
        }));

    MOCKER_CPP(&RackMem::MemoryIDUsedByNuma, void *(*)(AppBorrowMetaDesc & desc, const std::vector<uint64_t> &memIds,
                                                       int64_t numaId, size_t unitSize, const std::string &name))
        .stubs()
        .will(returnValue((void *)(nullptr)));

    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocNonNumaPathFailed)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = 0;

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(invoke(+[](std::shared_ptr<AppMallocMemoryRequest> &req,
                         std::shared_ptr<AppMallocMemoryResponse> &resp) -> uint32_t {
            resp->memIds_ = {1, 2, 3};
            resp->unitSize_ = 8; // 8 为模拟的 uintSize
            resp->name_ = "test_name";
            return 0;
        }));

    MOCKER_CPP(&RackMem::MemoryIDUsedByFd, void *(*)(AppBorrowMetaDesc & desc, const std::vector<uint64_t> &memIds,
                                                     size_t unitSize, const std::string &name, uint64_t))
        .stubs()
        .will(returnValue((void *)(nullptr)));

    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, nullptr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocNumaPathSuccess)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = UBSM_FLAG_MALLOC_WITH_NUMA;
    void *mockAddr = reinterpret_cast<void *>(0x12345678);

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(invoke(+[](std::shared_ptr<AppMallocMemoryRequest> &req,
                         std::shared_ptr<AppMallocMemoryResponse> &resp) -> uint32_t {
            resp->memIds_ = {1, 2, 3};
            resp->unitSize_ = 8; // 8 为模拟的 uintSize
            resp->numaId_ = 0;
            resp->name_ = "test_name";
            return 0;
        }));

    MOCKER_CPP(&RackMem::MemoryIDUsedByNuma, void *(*)(AppBorrowMetaDesc & desc, const std::vector<uint64_t> &memIds,
                                                       int64_t numaId, size_t unitSize, const std::string &name))
        .stubs()
        .will(returnValue(mockAddr));

    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, mockAddr);
}

TEST_F(RackMemTestSuite, TestUbsMemMallocNonNumaPathSuccess)
{
    std::string region = "test_region";
    size_t size = MOCK_FILE_LEN;
    PerfLevel level = PerfLevel::L0;
    uint64_t flags = 0;
    void *mockAddr = reinterpret_cast<void *>(0x12345678);

    MOCKER(static_cast<uint32_t (*)(std::shared_ptr<AppMallocMemoryRequest> & request,
                                    std::shared_ptr<AppMallocMemoryResponse> & response)>(IpcCommand::AppMallocMemory))
        .stubs()
        .will(invoke(+[](std::shared_ptr<AppMallocMemoryRequest> &req,
                         std::shared_ptr<AppMallocMemoryResponse> &resp) -> uint32_t {
            resp->memIds_ = {1, 2, 3};
            resp->unitSize_ = 8; // 8 为模拟的 uintSize
            resp->name_ = "test_name";
            return 0;
        }));

    MOCKER_CPP(&RackMem::MemoryIDUsedByFd,
               void *(*)(AppBorrowMetaDesc & desc, const std::vector<uint64_t> &memIds, int64_t numaId, size_t unitSize,
                         const std::string &name, uint64_t))
        .stubs()
        .will(returnValue(mockAddr));
    void *result = nullptr;
    auto ret = RackMem::GetInstance().UbsMemMalloc(region, size, level, flags, &result);
    ASSERT_EQ(result, mockAddr);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeNullPtr)
{
    void *ptr = nullptr;
    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeGetMetaFailed)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);

    MOCKER_CPP(&BorrowAppMetaMgr::GetMeta, uint32_t(*)(const void *addr, AppBorrowMetaDesc &desc))
        .stubs()
        .will(returnValue(1));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeNonNumaNotUnmapped)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);

    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, MOCK_FILE_LEN, false, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER_CPP(&BorrowAppMetaMgr::GetMetaHasUnmapped, bool (*)(const void *addr)).stubs().will(returnValue(false));

    MOCKER(SystemAdapter::MemoryUnMap).stubs().will(returnValue(0));

    MOCKER(SystemAdapter::MemoryMap).stubs().will(returnValue(ptr));

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(0));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 0);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeNonNumaMunmapFailed)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);

    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, MOCK_FILE_LEN, false, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER_CPP(&BorrowAppMetaMgr::GetMetaHasUnmapped, bool (*)(const void *addr)).stubs().will(returnValue(false));

    MOCKER(SystemAdapter::MemoryUnMap).stubs().will(returnValue(-1));

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(0));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 0);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeNumaSuccess)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);
    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, MOCK_FILE_LEN, true, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(0));

    MOCKER_CPP(&BorrowAppMetaMgr::RemoveMeta, uint32_t(*)(const void *addr)).stubs().will(returnValue(0));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 0);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeAppFreeFailed)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);
    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, 0, true, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(1));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 1);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeRemoveMetaFailed)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);
    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, 0, true, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(0));

    MOCKER_CPP(&BorrowAppMetaMgr::RemoveMeta, uint32_t(*)(const void *addr)).expects(once()).will(returnValue(1));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 0);
}

TEST_F(RackMemTestSuite, TestUbsMemFreeNonNumaLockAddressUnmap)
{
    void *ptr = reinterpret_cast<void *>(0x12345678);
    AppBorrowMetaDesc desc(0);
    desc.UpdateBorrowMetaDesc(std::vector<uint64_t>{}, 0, true, "testName");

    BorrowAppMetaMgr::GetInstance().AddMeta(ptr, desc);

    MOCKER_CPP(&BorrowAppMetaMgr::GetMetaIsLockAddress, bool (*)(const void *addr)).stubs().will(returnValue(true));

    MOCKER(SystemAdapter::MemoryUnMap).expects(never()).with(eq(ptr), eq(MOCK_FILE_LEN));

    MOCKER(IpcCommand::AppFreeMemory).stubs().will(returnValue(0));

    uint32_t result = RackMem::GetInstance().UbsMemFree(ptr);
    ASSERT_EQ(result, 0);
}

// RackMemLookupClusterStatistic测试
TEST_F(RackMemTestSuite, TestRackMemLookupClusterStatisticSuccess)
{
    ubsmem_cluster_info_t mockInfo{};

    MOCKER(IpcCommand::IpcRackMemLookupClusterStatistic)
        .expects(once())
        .with(any())
        .will(returnValue(static_cast<int>(UBSM_OK)));

    uint32_t result = RackMem::GetInstance().RackMemLookupClusterStatistic(&mockInfo);
    ASSERT_EQ(result, UBSM_OK);
}

TEST_F(RackMemTestSuite, TestRackMemLookupClusterStatisticFailure)
{
    const uint32_t expectedErr = MXM_ERR_MALLOC_FAIL;

    MOCKER(IpcCommand::IpcRackMemLookupClusterStatistic).expects(once()).will(returnValue(expectedErr));

    ubsmem_cluster_info_t info{};
    uint32_t result = RackMem::GetInstance().RackMemLookupClusterStatistic(&info);
    ASSERT_EQ(result, expectedErr);
}

TEST_F(RackMemTestSuite, TestQueryLeaseRecordSuccess)
{
    std::vector<std::string> mockRecords = {"record1", "record2"};

    MOCKER(IpcCommand::AppQueryLeaseRecord)
        .expects(once())
        .with(outBound(mockRecords, 0))
        .will(returnValue(static_cast<int>(UBSM_OK)));

    auto result = RackMem::GetInstance().QueryLeaseRecord();
    ASSERT_EQ(result.first, static_cast<int>(UBSM_OK));
    ASSERT_EQ(result.second.size(), 2); // 2 为模拟的记录数量
}

TEST_F(RackMemTestSuite, TestQueryLeaseRecordFailure)
{
    const uint32_t expectedErr = MXM_ERR_MALLOC_FAIL;

    MOCKER(IpcCommand::AppQueryLeaseRecord).expects(once()).will(returnValue(expectedErr));

    auto result = RackMem::GetInstance().QueryLeaseRecord();
    ASSERT_EQ(result.first, expectedErr);
    ASSERT_TRUE(result.second.empty());
}

TEST_F(RackMemTestSuite, TestForceFreeCachedMemorySuccess)
{
    MOCKER(IpcCommand::AppForceFreeCachedMemory).expects(once()).will(returnValue(static_cast<int>(UBSM_OK)));

    uint32_t result = RackMem::GetInstance().ForceFreeCachedMemory();
    ASSERT_EQ(result, UBSM_OK);
}

TEST_F(RackMemTestSuite, TestForceFreeCachedMemoryFailure)
{
    const uint32_t expectedErr = MXM_ERR_MALLOC_FAIL;

    MOCKER(IpcCommand::AppForceFreeCachedMemory).expects(once()).will(returnValue(expectedErr));

    uint32_t result = RackMem::GetInstance().ForceFreeCachedMemory();
    ASSERT_EQ(result, expectedErr);
}
} // namespace UT
