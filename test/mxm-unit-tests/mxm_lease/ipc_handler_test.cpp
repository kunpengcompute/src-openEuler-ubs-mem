/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "ipc_handler.h"
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "mls_manager.h"
#include "ubse_mem_adapter_stub.h"

namespace UT {
using namespace ock::lease::service;
using namespace ock::mxm;
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
#endif

class MLSIpcHandlerTest : public testing::Test {
public:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(MLSIpcHandlerTest, TestAppMallocMemoryReuseBufferedMemReturnZero)
{
    auto request = std::make_shared<AppMallocMemoryRequest>();
    auto response = std::make_shared<AppMallocMemoryResponse>();
    MxmComUdsInfo udsInfo{};
    MOCKER_CPP(&MLSManager::ReuseBufferedMem, int32_t(*)(uint64_t size, uint16_t isNuma, const std::string &regionName,
                                                         const AppContext &context, MLSMemInfo &info))
        .stubs()
        .will(returnValue(0));

    auto result = MxmServerMsgHandle::AppMallocMemory(request.get(), response.get(), udsInfo);

    ASSERT_EQ(UBSM_OK, result);
}

TEST_F(MLSIpcHandlerTest, TestAppMallocMemoryReuseBufferedMemReturnNonZero)
{
    auto request = std::make_shared<AppMallocMemoryRequest>();
    auto response = std::make_shared<AppMallocMemoryResponse>();
    MxmComUdsInfo udsInfo{};
    MOCKER_CPP(&MLSManager::ReuseBufferedMem, int32_t(*)(uint64_t size, uint16_t isNuma, const std::string &regionName,
                                                         const AppContext &context, MLSMemInfo &info))
        .stubs()
        .will(returnValue(-1));

    ock::ubsm::LeaseMallocResult res{.memIds = {1, 2, 3}, .numaId = 1, .unitSize = 8};
    MOCKER(ock::mxm::UbseMemAdapter::LeaseMalloc).stubs().with(any(), outBound(res)).will(returnValue(0));
    auto result = MxmServerMsgHandle::AppMallocMemory(request.get(), response.get(), udsInfo);

    ASSERT_EQ(UBSM_OK, result);
}

TEST_F(MLSIpcHandlerTest, TestAppMallocMemory_FailWhenParamNullptr)
{
    auto result = MxmServerMsgHandle::AppMallocMemory(nullptr, nullptr, {});

    ASSERT_NE(result, 0);
}

TEST_F(MLSIpcHandlerTest, Test_AppFreeMemory_SuccessWhenFdMalloc)
{
    std::vector<uint64_t> memIds{1, 2, 3, 4, 5, 6, 7, 8};
    auto name = "N111223344";
    MLSManager::GetInstance().PreAddUsedMem(name, 1 << 30, {}, false, 0);

    MLSManager::GetInstance().FinishAddUsedMem(name, -1, 1 << 27, 0, memIds);
    std::shared_ptr<AppFreeMemoryRequest> request = std::make_shared<AppFreeMemoryRequest>(memIds, "default", name);

    std::shared_ptr<CommonResponse> response = std::make_shared<CommonResponse>();

    MOCKER_CPP(&UbseMemAdapter::FdPermissionChange,
               int (*)(const std::string &name, const AppContext &appContext, mode_t mode))
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&UbseMemAdapter::LeaseFree, int (*)(const std::string &name, bool isNuma)).stubs().will(returnValue(0));

    auto ret = MxmServerMsgHandle::AppFreeMemory(request.get(), response.get(), {});

    EXPECT_EQ(ret, 0);
}

TEST_F(MLSIpcHandlerTest, Test_AppFreeMemory_SuccessWhenNumaMalloc)
{
    std::vector<uint64_t> memIds{9, 10, 11, 12, 13, 14, 15, 16};
    auto name = "N443322111";

    MLSManager::GetInstance().PreAddUsedMem(name, 1 << 30, {}, false, 0);

    MLSManager::GetInstance().FinishAddUsedMem(name, -1, 1 << 27, 0, memIds);
    std::shared_ptr<AppFreeMemoryRequest> request = std::make_shared<AppFreeMemoryRequest>(memIds, "default", name);

    std::shared_ptr<CommonResponse> response = std::make_shared<CommonResponse>();

    MOCKER_CPP(&UbseMemAdapter::FdPermissionChange,
               int (*)(const std::string &name, const AppContext &appContext, mode_t mode))
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&UbseMemAdapter::LeaseFree, int (*)(const std::string &name, bool isNuma)).stubs().will(returnValue(0));

    auto ret = MxmServerMsgHandle::AppFreeMemory(request.get(), response.get(), {});

    EXPECT_EQ(ret, 0);
}

TEST_F(MLSIpcHandlerTest, TestAppQueryClusterInfo_Success)
{
    std::shared_ptr<CommonRequest> request = std::make_shared<CommonRequest>();
    std::shared_ptr<AppQueryClusterInfoResponse> response = std::make_shared<AppQueryClusterInfoResponse>();
    MOCKER_CPP(&UbseMemAdapter::LookUpClusterStatistic, int (*)(ubsmemClusterInfo & clusterInfo))
        .stubs()
        .will(returnValue(0));

    auto ret = MxmServerMsgHandle::AppQueryClusterInfo(request.get(), response.get(), {});

    EXPECT_EQ(ret, 0);
}

TEST_F(MLSIpcHandlerTest, TestAppQueryClusterInfo_FailWhenUbseFail)
{
    std::shared_ptr<CommonRequest> request = std::make_shared<CommonRequest>();
    std::shared_ptr<AppQueryClusterInfoResponse> response = std::make_shared<AppQueryClusterInfoResponse>();
    MOCKER_CPP(&UbseMemAdapter::LookUpClusterStatistic, int (*)(ubsmemClusterInfo & clusterInfo))
        .stubs()
        .will(returnValue(-1));

    auto ret = MxmServerMsgHandle::AppQueryClusterInfo(request.get(), response.get(), {});

    EXPECT_EQ(ret, -1);
}

TEST_F(MLSIpcHandlerTest, TestAppQueryClusterInfo_FailWhenNullptr)
{
    auto ret = MxmServerMsgHandle::AppQueryClusterInfo(nullptr, nullptr, {});

    EXPECT_EQ(ret, MXM_ERR_NULLPTR);
}

TEST_F(MLSIpcHandlerTest, TestAppQueryCachedMemory_Success)
{
    std::shared_ptr<CommonRequest> request = std::make_shared<CommonRequest>();
    std::shared_ptr<AppQueryCachedMemoryResponse> response = std::make_shared<AppQueryCachedMemoryResponse>();

    auto ret = MxmServerMsgHandle::AppQueryCachedMemory(request.get(), response.get(), {});

    EXPECT_EQ(ret, UBSM_OK);
}

TEST_F(MLSIpcHandlerTest, TestAppForceFreeCachedMemory_Success)
{
    std::shared_ptr<CommonRequest> request = std::make_shared<CommonRequest>();
    std::shared_ptr<CommonResponse> response = std::make_shared<CommonResponse>();

    MOCKER_CPP(&UbseMemAdapter::LeaseFree, int (*)(const std::string &name, bool isNuma)).stubs().will(returnValue(0));

    auto ret = MxmServerMsgHandle::AppForceFreeCachedMemory(request.get(), response.get(), {});

    EXPECT_EQ(ret, UBSM_OK);
}

TEST_F(MLSIpcHandlerTest, TestAppForceFreeCachedMemory_FailWhenNullptr)
{
    auto ret = MxmServerMsgHandle::AppForceFreeCachedMemory(nullptr, nullptr, {});

    EXPECT_EQ(ret, MXM_ERR_NULLPTR);
}

TEST_F(MLSIpcHandlerTest, TestAppQueryCachedMemory_FailWhenNullptr)
{
    auto ret = MxmServerMsgHandle::AppQueryCachedMemory(nullptr, nullptr, {});

    EXPECT_EQ(ret, MXM_ERR_NULLPTR);
}

// MLSManager::ReuseBufferedMem
TEST_F(MLSIpcHandlerTest, TestReuseBufferedMem_SuccessWhenNumaBorrow)
{
    std::vector<uint64_t> memIds{1, 2, 3, 4, 5, 6, 7, 8};
    MLSMemInfo memory{"TEST_NUMA_BORROW", {1111, 1111, 1111}, 1 << 30, 4, memIds, 1 << 27, 0, 1};
    MLSManager::GetInstance().PreAddUsedMem("TEST_NUMA_BORROW", 1 << 30, {}, true, 0);
    MLSManager::GetInstance().FinishAddUsedMem("TEST_NUMA_BORROW", 4, 1 << 27, 0, memIds);
    MLSManager::GetInstance().PreDeleteUsedMem("TEST_NUMA_BORROW");
    std::vector<uint32_t> slotIds{0, 1};
    MOCKER(ock::mxm::UbseMemAdapter::GetRegionInfo).stubs().with(any(), outBound(slotIds)).will(returnValue(0));
    auto ret = MLSManager::GetInstance().ReuseBufferedMem(1 << 30, 1, "default", {}, memory);

    EXPECT_EQ(ret, 0);
}

TEST_F(MLSIpcHandlerTest, TestReuseBufferedMem_SuccessWhenFdBorrow)
{
    std::vector<uint64_t> memIds{9, 10, 11, 12, 13, 14, 15, 16};
    MLSMemInfo memory{"TEST_FD_BORROW", {1111, 1111, 1111}, 1 << 30, -1, memIds, 1 << 27, 0, 1};
    MLSManager::GetInstance().PreAddUsedMem("TEST_FD_BORROW", 1 << 30, {}, false, 0);
    MLSManager::GetInstance().FinishAddUsedMem("TEST_FD_BORROW", -1, 1 << 27, 0, memIds);
    MLSManager::GetInstance().PreDeleteUsedMem("TEST_FD_BORROW");
    std::vector<uint32_t> slotIds{0, 1};
    MOCKER(ock::mxm::UbseMemAdapter::GetRegionInfo).stubs().with(any(), outBound(slotIds)).will(returnValue(0));
    auto ret = MLSManager::GetInstance().ReuseBufferedMem(1 << 30, 0, "default", {}, memory);

    EXPECT_EQ(ret, 0);
}

TEST_F(MLSIpcHandlerTest, TestGetUsedMemByPid_Success)
{
    auto ret = MLSManager::GetInstance().GetUsedMemByPid(0);
    EXPECT_NE(ret.size(), 0);
}
} // namespace UT