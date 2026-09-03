/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "app_ipc_stub.h"
#include "mxmem_lease.cpp"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
namespace UT {
using namespace ock::mxmd;
constexpr uint64_t MOCK_SIZE = 4096;

class MxmemLeaseTestSuite : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocRegionNameNull)
{
    void *ptr = nullptr;
    uint32_t ret = ubsmem_lease_malloc_impl(nullptr, MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &ptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    ASSERT_EQ(ubsmem_lease_malloc(nullptr, MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &ptr),
              GetErrCode(MXM_ERR_PARAM_INVALID));
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocWithLocation)
{
    void *ptr = nullptr;
    uint32_t ret = ubsmem_lease_malloc_with_location(nullptr, MOCK_SIZE, 0, &ptr);
    ASSERT_EQ(ret, UBSM_ERR_PARAM_INVALID);

    ubs_mem_location_t lender = {0};
    ret = ubsmem_lease_malloc_with_location(&lender, MOCK_SIZE, 0, nullptr);
    ASSERT_EQ(ret, UBSM_ERR_PARAM_INVALID);

    ret = ubsmem_lease_malloc_with_location(&lender, -1, 0, &ptr);
    ASSERT_EQ(ret, UBSM_ERR_PARAM_INVALID);

    ret = ubsmem_lease_malloc_with_location(&lender, MOCK_SIZE, 0, &ptr);
    ASSERT_EQ(ret, UBSM_ERR_PARAM_INVALID);

    void *mockAddr = reinterpret_cast<void *>(0x12345678);
    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmComIpcClientSendStub));
    MOCKER_CPP(&RackMem::MemoryIDUsedByFd,
               void *(*)(AppBorrowMetaDesc & desc, const std::vector<uint64_t> &memIds, int64_t numaId, size_t unitSize,
                         const std::string &name, uint64_t))
        .stubs()
        .will(returnValue(mockAddr));
    ret = ubsmem_lease_malloc_with_location(&lender, MOCK_SIZE * 1024, 0, &ptr);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(ptr, mockAddr);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocRegionNameEmpty)
{
    void *ptr = nullptr;
    std::string emptyName = "";
    uint32_t ret = ubsmem_lease_malloc_impl(emptyName.c_str(), MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &ptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocRegionNameTooLong)
{
    void *ptr = nullptr;
    std::string longName = std::string(MAX_REGION_NAME_DESC_LENGTH, 'a');
    uint32_t ret = ubsmem_lease_malloc_impl(longName.c_str(), MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &ptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocLocalPtrNull)
{
    std::string name = "valid_region";
    uint32_t ret = ubsmem_lease_malloc_impl(name.c_str(), MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, nullptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocInvalidSize)
{
    void *ptr = nullptr;
    std::string name = "valid_region";
    MOCKER(CheckRackMemSize).stubs().will(returnValue(false));
    uint32_t ret = ubsmem_lease_malloc_impl(name.c_str(), 0, DISTANCE_DIRECT_NODE, 0, &ptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocInvalidDistance)
{
    void *ptr = nullptr;
    std::string name = "valid_region";
    ubsmem_distance_t invalidDist = DISTANCE_HOP_NODE;
    uint32_t ret = ubsmem_lease_malloc_impl(name.c_str(), MOCK_SIZE, invalidDist, 0, &ptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocSuccess)
{
    void *mockPtr = reinterpret_cast<void *>(0x12345678);
    void *outputPtr = nullptr;
    std::string name = "valid_region";

    MOCKER(CheckRackMemSize).stubs().will(returnValue(true));

    MOCKER_CPP(&RackMem::UbsMemMalloc, int (*)(const std::string &, size_t, PerfLevel, uint64_t, void **))
        .expects(once())
        .will(returnValue(0));

    uint32_t ret = ubsmem_lease_malloc_impl(name.c_str(), MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &outputPtr);
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseMallocAllocationFailed)
{
    void *outputPtr = nullptr;
    std::string name = "valid_region";

    MOCKER(CheckRackMemSize).stubs().will(returnValue(true));

    MOCKER_CPP(&RackMem::UbsMemMalloc, int (*)(const std::string &, size_t, PerfLevel, uint64_t, void **))
        .expects(once())
        .will(returnValue(1));

    uint32_t ret = ubsmem_lease_malloc_impl(name.c_str(), MOCK_SIZE, DISTANCE_DIRECT_NODE, 0, &outputPtr);
    ASSERT_EQ(ret, 1);
    ASSERT_EQ(outputPtr, nullptr);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseFreeNullPtr)
{
    uint32_t ret = ubsmem_lease_free_impl(nullptr);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
    ASSERT_EQ(ubsmem_lease_free(nullptr), GetErrCode(MXM_ERR_PARAM_INVALID));
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseFreeSuccess)
{
    void *ptr = reinterpret_cast<void *>(0x1234);
    MOCKER_CPP(&RackMem::UbsMemFree, int (*)(void *)).expects(once()).will(returnValue(0));
    uint32_t ret = ubsmem_lease_free_impl(ptr);
    ASSERT_EQ(ret, 0);
}

TEST_F(MxmemLeaseTestSuite, TestUbsmemLeaseFreeFailed)
{
    void *ptr = reinterpret_cast<void *>(0x1234);
    const int expectedErr = -5;
    MOCKER_CPP(&RackMem::UbsMemFree, int (*)(void *)).stubs().will(returnValue(expectedErr));
    uint32_t ret = ubsmem_lease_free_impl(ptr);
    ASSERT_EQ(ret, expectedErr);
}
} // namespace UT