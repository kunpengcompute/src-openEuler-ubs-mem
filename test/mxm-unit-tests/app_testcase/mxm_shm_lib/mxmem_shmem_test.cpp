/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <mockcpp/mockcpp.hpp>

#define private public
#include "UbseMemExecutor.h"
#undef private
#include "ubs_mem.h"
#include "RackMemShm.h"
#include "app_ipc_stub.h"
#include "mx_shm.h"
#include "rack_mem_err.h"
#include "rack_mem_lib.h"
#include "shm_ipc_command.h"
#include "system_adapter.h"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
namespace UT {
using namespace ock::mxmd;
using namespace ock::common;
using namespace ock::ubsm;
const auto PAGE_SIZE = static_cast<size_t>(sysconf(_SC_PAGESIZE));

class UbsmemShmemAllocateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        valid_size = 4194304; // 4194304 是需要接近的 pageSize
        valid_flags = UBSM_FLAG_CACHE;
        valid_mode = 0644;
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_region_name = "default";
    std::string valid_name = "valid_name";
    size_t valid_size;
    uint64_t valid_flags;
    mode_t valid_mode;
};

TEST_F(UbsmemShmemAllocateTest, NormalCase)
{
    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmShmIpcClientSendStub));
    MOCKER_CPP(&RackMemShm::UbsMemShmCreate,
               uint32_t(*)(const std::string &regionName, const std::string &name, uint64_t size,
                           const std::string &baseNid, const SHMRegionDesc &shmRegion, int flags, mode_t mode))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_allocate(valid_region_name.c_str(), valid_name.c_str(), valid_size, valid_mode,
                                             valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, NormalCaseIpcStub)
{
    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmShmIpcClientSendStub));

    EXPECT_EQ(UBSM_OK, ubsmem_shmem_allocate(valid_region_name.c_str(), valid_name.c_str(), valid_size, valid_mode,
                                             valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, RegionNameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate(nullptr, valid_name.c_str(), valid_size, valid_mode, valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, NameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate(valid_region_name.c_str(), nullptr, valid_size, valid_mode, valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, RegionNameZeroLength)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate("", valid_name.c_str(), valid_size, valid_mode, valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, NameZeroLength)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate(valid_region_name.c_str(), "", valid_size, valid_mode, valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, NameTooLong)
{
    std::string longName = std::string(MAX_SHM_NAME_LENGTH, 'a');
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate(valid_region_name.c_str(), longName.c_str(), valid_size, valid_mode, valid_flags));
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_allocate(longName.c_str(), valid_name.c_str(), valid_size, valid_mode, valid_flags));
}

TEST_F(UbsmemShmemAllocateTest, FlagsInvalid)
{
    uint64_t invalid_flags = 0xFFFFFFFF; // 无效的标志
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_allocate(valid_region_name.c_str(), valid_name.c_str(), valid_size,
                                                            valid_mode, invalid_flags));
}

class UbsmemShmemDeallocateTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string validName = "valid_name";
};

TEST_F(UbsmemShmemDeallocateTest, NormalCase)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmDelete,
               uint32_t(*)(const std::string &regionName, const std::string &name, uint64_t size,
                           const std::string &baseNid, const SHMRegionDesc &shmRegion, int flags, mode_t mode))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_deallocate(validName.c_str()));
}

TEST_F(UbsmemShmemDeallocateTest, NameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_deallocate(nullptr));
}

TEST_F(UbsmemShmemDeallocateTest, NameZeroLength)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_deallocate(""));
}

TEST_F(UbsmemShmemDeallocateTest, NameTooLong)
{
    std::string longName = std::string(MAX_SHM_NAME_LENGTH, 'a');
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_deallocate(longName.c_str()));
}

TEST_F(UbsmemShmemDeallocateTest, DeleteFailed)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmDelete,
               uint32_t(*)(const std::string &regionName, const std::string &name, uint64_t size,
                           const std::string &baseNid, const SHMRegionDesc &shmRegion, int flags, mode_t mode))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_MALLOC_FAIL)));
    EXPECT_EQ(UBSM_ERR_MALLOC_FAIL, ubsmem_shmem_deallocate(validName.c_str())); // 6022 是预期的返回值
}

class UbsmemShmemMapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        valid_name = "valid_name";
        fail_name = "fail_name";
        nullptr_name = "nullptr_name";
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    std::string fail_name;
    std::string nullptr_name;
};

TEST_F(UbsmemShmemMapTest, NormalCaseLocalPtrIsNil)
{
    void *result_ptr;
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;

    MOCKER_CPP(&RackMemShm::UbsMemShmMmap, int (*)(void *start, size_t mapSize, int prot, int flags,
                                                   const std::string &name, off_t off, void **local_ptr))
        .stubs()
        .will(returnValue(0));

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags,
                                                       valid_name.c_str(), valid_offset, &result_ptr));
}

TEST_F(UbsmemShmemMapTest, NameNullptr)
{
    void *result_ptr;
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags, nullptr, valid_offset, &result_ptr));
}

TEST_F(UbsmemShmemMapTest, LocalPtrNullptr)
{
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags,
                                                       valid_name.c_str(), valid_offset, nullptr));
}

TEST_F(UbsmemShmemMapTest, LengthInvalid)
{
    void *result_ptr;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_map(nullptr, 0, valid_prot, valid_flags, valid_name.c_str(), valid_offset, &result_ptr));
}

TEST_F(UbsmemShmemMapTest, NameZeroLength)
{
    void *result_ptr;
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;
    std::string empty_name = "";

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags,
                                                       empty_name.c_str(), valid_offset, &result_ptr));
}

TEST_F(UbsmemShmemMapTest, NameTooLong)
{
    void *result_ptr;
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;
    std::string too_long_name = std::string(MAX_SHM_NAME_LENGTH, 'a');

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags,
                                                       too_long_name.c_str(), valid_offset, &result_ptr));
}

TEST_F(UbsmemShmemMapTest, MapFailed)
{
    void *result_ptr;
    size_t valid_length = 1024;
    off_t valid_offset = 0;
    int valid_prot = PROT_READ | PROT_WRITE;
    int valid_flags = MAP_SHARED;
    MOCKER_CPP(&RackMemShm::UbsMemShmMmap, int (*)(void *start, size_t mapSize, int prot, int flags,
                                                   const std::string &name, off_t off, void **local_ptr))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_shmem_map(nullptr, valid_length, valid_prot, valid_flags, fail_name.c_str(),
                                                   valid_offset, &result_ptr));
}

class UbsmemShmemUnmapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 分配一个有效的内存指针用于测试
        valid_ptr = malloc(PAGE_SIZE);
        fail_ptr = reinterpret_cast<void *>(0xDEADBEEF); // 特殊指针，模拟失败
    }

    void TearDown() override
    {
        if (valid_ptr != nullptr) {
            free(valid_ptr);
            valid_ptr = nullptr;
        }
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void *valid_ptr;
    void *fail_ptr;
};

TEST_F(UbsmemShmemUnmapTest, NormalCase)
{
    size_t valid_length = 1024;
    MOCKER_CPP(&RackMemShm::UbsMemShmUnmmap, uint32_t(*)(void *start, size_t size)).stubs().will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_unmap(valid_ptr, valid_length));
}

TEST_F(UbsmemShmemUnmapTest, LocalPtrNullptr)
{
    size_t valid_length = 1024;
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unmap(nullptr, valid_length));
}

TEST_F(UbsmemShmemUnmapTest, LengthZero)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unmap(valid_ptr, 0));
}

TEST_F(UbsmemShmemUnmapTest, UnmapFailed)
{
    size_t valid_length = 1024;
    MOCKER_CPP(&RackMemShm::UbsMemShmUnmmap, uint32_t(*)(void *start, size_t size))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_PARAM_INVALID)));
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unmap(fail_ptr, valid_length));
}

TEST_F(UbsmemShmemUnmapTest, LengthNeedsRounding)
{
    size_t need_round_length = 100;
    MOCKER_CPP(&RackMemShm::UbsMemShmUnmmap, uint32_t(*)(void *start, size_t size)).stubs().will(returnValue(0));

    EXPECT_EQ(UBSM_OK, ubsmem_shmem_unmap(valid_ptr, need_round_length));
}

class UbsmemShmemSetOwnershipTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 分配一个有效的内存指针用于测试
        valid_ptr = malloc(PAGE_SIZE);

        // 使用 std::string 初始化有效的测试数据
        valid_name = "valid_name";
        fail_name = "fail_name";
    }

    void TearDown() override
    {
        if (valid_ptr != nullptr) {
            free(valid_ptr);
            valid_ptr = nullptr;
        }
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void *valid_ptr;
    std::string valid_name;
    std::string fail_name;
};

TEST_F(UbsmemShmemSetOwnershipTest, NormalCaseProtNone)
{
    size_t valid_length = PAGE_SIZE;
    MOCKER_CPP(&RackMemShm::UbsMemShmSetOwnerShip,
               int32_t(*)(const std::string &name, void *start, size_t length, ShmOwnStatus status))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_set_ownership(valid_name.c_str(), valid_ptr, valid_length, PROT_NONE));
}

TEST_F(UbsmemShmemSetOwnershipTest, NameNullptr)
{
    size_t valid_length = PAGE_SIZE;
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_set_ownership(nullptr, valid_ptr, valid_length, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, NameZeroLength)
{
    size_t valid_length = PAGE_SIZE;
    std::string empty_name = "";
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_set_ownership(empty_name.c_str(), valid_ptr, valid_length, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, NameTooLong)
{
    size_t valid_length = PAGE_SIZE;
    std::string too_long_name = std::string(MAX_SHM_NAME_LENGTH, 'a');
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_set_ownership(too_long_name.c_str(), valid_ptr, valid_length, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, StartNullptr)
{
    size_t valid_length = PAGE_SIZE;
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_set_ownership(valid_name.c_str(), nullptr, valid_length, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, LengthZero)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_set_ownership(valid_name.c_str(), valid_ptr, 0, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, LengthNotMultipleOfPageSize)
{
    size_t invalid_length = PAGE_SIZE + 1;
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_set_ownership(valid_name.c_str(), valid_ptr, invalid_length, PROT_READ));
}

TEST_F(UbsmemShmemSetOwnershipTest, ProtInvalid)
{
    size_t valid_length = PAGE_SIZE;
    int invalid_prot = PROT_WRITE; // 单独的PROT_WRITE是无效的
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID,
              ubsmem_shmem_set_ownership(valid_name.c_str(), valid_ptr, valid_length, invalid_prot));
}

TEST_F(UbsmemShmemSetOwnershipTest, SetOwnershipFailed)
{
    size_t valid_length = PAGE_SIZE;
    MOCKER_CPP(&RackMemShm::UbsMemShmSetOwnerShip,
               int32_t(*)(const std::string &name, void *start, size_t length, ShmOwnStatus status))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_shmem_set_ownership(fail_name.c_str(), valid_ptr, valid_length, PROT_READ));
}

class UbsmLookupRegionsTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(UbsmLookupRegionsTest, NormalCase)
{
    ubsmem_regions_t regions;
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemLookupResourceRegions,
               uint32_t(*)(const std::string &baseNid, ShmRegionType type, SHMRegions &list))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_lookup_regions(&regions));
}

TEST_F(UbsmLookupRegionsTest, RegionsNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_lookup_regions(nullptr));
}

TEST_F(UbsmLookupRegionsTest, StartRackMemFailed)
{
    ubsmem_regions_t regions;
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(-1));
    EXPECT_EQ(UBSM_ERR_MEMLIB, ubsmem_lookup_regions(&regions));
}

TEST_F(UbsmLookupRegionsTest, LookupResourceRegionsFailed)
{
    ubsmem_regions_t regions;
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemLookupResourceRegions,
               uint32_t(*)(const std::string &baseNid, ShmRegionType type, SHMRegions &list))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_MEMLIB)));
    EXPECT_EQ(UBSM_ERR_MEMLIB, ubsmem_lookup_regions(&regions));
}

class UbsmemCreateRegionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        valid_region_name = "test_region";
        valid_size = 0;

        empty_region_name = "";
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    const char *valid_region_name{};
    size_t valid_size{};
    ubsmem_region_attributes_t valid_reg_attr{.host_num = 2, .hosts = {{"host1"}, {"host2"}}};

    const char *empty_region_name{};
};

TEST_F(UbsmemCreateRegionTest, RegionNameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_create_region(nullptr, valid_size, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, RegionNameDefault)
{
    EXPECT_EQ(UBSM_ERR_NO_NEEDED, ubsmem_create_region("default", valid_size, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, SizeInvalid)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_create_region("name", 1, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, RegionNameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_create_region(empty_region_name, valid_size, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, RegAttrNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_create_region(valid_region_name, valid_size, nullptr));
}

TEST_F(UbsmemCreateRegionTest, RegAttrInvalid)
{
    ubsmem_region_attributes_t invalid_reg_attr{.host_num = 2, .hosts = {{""}, {"host2"}}};
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_create_region("name", valid_size, &invalid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, LibNotInit)
{
    EXPECT_EQ(UBSM_ERR_MEMLIB, ubsmem_create_region("name", valid_size, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, CreateRegionNetError)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));

    EXPECT_EQ(UBSM_ERR_NET, ubsmem_create_region("name", valid_size, &valid_reg_attr));
}

TEST_F(UbsmemCreateRegionTest, CreateRegion2)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));

    MOCKER_CPP(&MxmComIpcClientSend, int (*)(uint16_t opCode, MsgBase * request, MsgBase * response))
        .stubs()
        .will(invoke(MxmShmIpcClientSendStub));
    EXPECT_EQ(UBSM_CHECK_RESOURCE_ERROR, ubsmem_create_region("name", valid_size, &valid_reg_attr));
}

class UbsmemLookupRegionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        valid_region_name = "test_region";
        memset_s(&valid_region_desc, 0, sizeof(valid_region_desc), 0);

        // 设置无效的测试数据
        empty_region_name = "";
        long_region_name = std::string(MAX_REGION_NAME_DESC_LENGTH, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_region_name;
    ubsmem_region_desc_t valid_region_desc{};

    std::string empty_region_name;
    std::string long_region_name;
};

TEST_F(UbsmemLookupRegionTest, NormalCase)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemLookupResourceRegion,
               uint32_t(*)(const std::string &regionName, SHMRegionDesc &region))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_lookup_region(valid_region_name.c_str(), &valid_region_desc));
}

TEST_F(UbsmemLookupRegionTest, RegionNameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_lookup_region(nullptr, &valid_region_desc));
}

TEST_F(UbsmemLookupRegionTest, RegionDescNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_lookup_region(valid_region_name.c_str(), nullptr));
}

TEST_F(UbsmemLookupRegionTest, RegionNameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_lookup_region(empty_region_name.c_str(), &valid_region_desc));
}

TEST_F(UbsmemLookupRegionTest, RegionNameTooLong)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_lookup_region(long_region_name.c_str(), &valid_region_desc));
}

TEST_F(UbsmemLookupRegionTest, StartRackMemFailed)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(-1));
    EXPECT_EQ(UBSM_ERR_MEMLIB, ubsmem_lookup_region(valid_region_name.c_str(), &valid_region_desc));
}

TEST_F(UbsmemLookupRegionTest, LookupResourceRegionFailed)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemLookupResourceRegion,
               uint32_t(*)(const std::string &regionName, SHMRegionDesc &region))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_lookup_region(valid_region_name.c_str(), &valid_region_desc));
}

class UbsmemDestroyRegionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_region_name = "test_region";

        // 设置无效的测试数据
        empty_region_name = "";
        long_region_name = std::string(MAX_REGION_NAME_DESC_LENGTH, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_region_name;
    std::string empty_region_name;
    std::string long_region_name;
};

TEST_F(UbsmemDestroyRegionTest, NormalCase)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemDestroyResourceRegion, uint32_t(*)(const std::string &regionName))
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_destroy_region(valid_region_name.c_str()));
}

TEST_F(UbsmemDestroyRegionTest, RegionNameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_destroy_region(nullptr));
}

TEST_F(UbsmemDestroyRegionTest, RegionNameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_destroy_region(empty_region_name.c_str()));
}

TEST_F(UbsmemDestroyRegionTest, RegionNameTooLong)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_destroy_region(long_region_name.c_str()));
}

TEST_F(UbsmemDestroyRegionTest, StartRackMemFailed)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(static_cast<int>(MXM_ERR_MEMLIB)));
    EXPECT_EQ(UBSM_ERR_MEMLIB, ubsmem_destroy_region(valid_region_name.c_str()));
}

TEST_F(UbsmemDestroyRegionTest, DestroyResourceRegionFailed)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&RackMemShm::RackMemDestroyResourceRegion, uint32_t(*)(const std::string &regionName))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_destroy_region(valid_region_name.c_str()));
}

class UbsmemShmemWriteLockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_name = "test_shmem";

        // 设置无效的测试数据
        empty_name = "";
        long_name = std::string(MAX_SHM_NAME_LENGTH, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    std::string empty_name;
    std::string long_name;
};

TEST_F(UbsmemShmemWriteLockTest, NormalCase)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmWriteLock, uint32_t(*)(const std::string &name)).stubs().will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_write_lock(valid_name.c_str()));
}

TEST_F(UbsmemShmemWriteLockTest, NameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_write_lock(nullptr));
}

TEST_F(UbsmemShmemWriteLockTest, NameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_write_lock(empty_name.c_str()));
}

TEST_F(UbsmemShmemWriteLockTest, NameTooLong)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_write_lock(long_name.c_str()));
}

TEST_F(UbsmemShmemWriteLockTest, WriteLockFailed)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmWriteLock, uint32_t(*)(const std::string &name))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_shmem_write_lock(valid_name.c_str()));
}

class UbsmemShmemReadLockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_name = "test_shmem";

        // 设置无效的测试数据
        empty_name = "";
        long_name = std::string(MAX_SHM_NAME_LENGTH, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    std::string empty_name;
    std::string long_name;
};

TEST_F(UbsmemShmemReadLockTest, NormalCase)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmReadLock, uint32_t(*)(const std::string &name)).stubs().will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_read_lock(valid_name.c_str()));
}

TEST_F(UbsmemShmemReadLockTest, NameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_read_lock(nullptr));
}

TEST_F(UbsmemShmemReadLockTest, NameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_read_lock(empty_name.c_str()));
}

TEST_F(UbsmemShmemReadLockTest, NameTooLong)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_read_lock(long_name.c_str()));
}

TEST_F(UbsmemShmemReadLockTest, ReadLockFailed)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmReadLock, uint32_t(*)(const std::string &name))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_shmem_read_lock(valid_name.c_str()));
}

class UbsmemShmemUnLockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_name = "test_shmem";

        // 设置无效的测试数据
        empty_name = "";
        long_name = std::string(MAX_SHM_NAME_LENGTH, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    std::string empty_name;
    std::string long_name;
};

TEST_F(UbsmemShmemUnLockTest, NormalCase)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmUnLock, uint32_t(*)(const std::string &name)).stubs().will(returnValue(0));
    EXPECT_EQ(UBSM_OK, ubsmem_shmem_unlock(valid_name.c_str()));
}

TEST_F(UbsmemShmemUnLockTest, NameNullptr)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unlock(nullptr));
}

TEST_F(UbsmemShmemUnLockTest, NameEmpty)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unlock(empty_name.c_str()));
}

TEST_F(UbsmemShmemUnLockTest, NameTooLong)
{
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_unlock(long_name.c_str()));
}

TEST_F(UbsmemShmemUnLockTest, UnLockFailed)
{
    MOCKER_CPP(&RackMemShm::UbsMemShmUnLock, uint32_t(*)(const std::string &name))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    EXPECT_EQ(UBSM_ERR_NOT_FOUND, ubsmem_shmem_unlock(valid_name.c_str()));
}

class UbsmemSecuritiesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

int32_t UbseFaultEventRegisterStub(ubs_mem_shm_fault_handler registerFunc)
{
    return 0;
}
int32_t shmemFaultsFuncStub(const char *shm_name, ubsmem_fault_type_t fault_type)
{
    return 0;
}
void engine_log_callback_register(ubs_engine_log_handler handler) {}

void *RegisterFaultMock(void *handle, const char *name)
{
    std::string nameStr;
    if (name != nullptr) {
        nameStr = name;
    }
    if (nameStr == "ubs_mem_shm_fault_register") {
        return (void *)UbseFaultEventRegisterStub;
    } else if (nameStr == "ubs_engine_log_callback_register") {
        return (void *)engine_log_callback_register;
    }
    return nullptr;
}

TEST_F(UbsmemSecuritiesTest, TestUbsmemLocalNidQueryNull)
{
    uint32_t *nid = nullptr;
    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_local_nid_query(nid));
}

TEST_F(UbsmemSecuritiesTest, TestUbsmemLocalNidQueryNull01)
{
    MOCKER(ShmIpcCommand::AppGetLocalSlotId).stubs().will(returnValue(1000));
    uint32_t nid = 0;
    EXPECT_EQ(UBSM_ERR_BUFF, ubsmem_local_nid_query(&nid));
}

TEST_F(UbsmemSecuritiesTest, TestUbsmemLocalNidQueryNull02)
{
    MOCKER(ShmIpcCommand::AppGetLocalSlotId).stubs().will(returnValue(801));
    uint32_t nid = 0;
    EXPECT_EQ(UBSM_ERR_UBSE, ubsmem_local_nid_query(&nid));
}

TEST_F(UbsmemSecuritiesTest, TestUbsmemLocalNidQuery)
{
    MOCKER(ShmIpcCommand::AppGetLocalSlotId).stubs().with(outBound(1u)).will(returnValue(0));
    uint32_t nid = 0;
    EXPECT_EQ(0, ubsmem_local_nid_query(&nid));
    EXPECT_EQ(nid, 1);
}

TEST_F(UbsmemSecuritiesTest, TestubsmemShmemFaultsRegister)
{
    const char *shmFaultRegister = "ubs_mem_shm_fault_register";
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER_CPP(&SystemAdapter::DlSym, void *(*)(void *handle, const char *name))
        .stubs()
        .will(invoke(RegisterFaultMock));

    EXPECT_EQ(0, ubsmem_shmem_faults_register(shmemFaultsFuncStub));
}

TEST_F(UbsmemSecuritiesTest, TestubsmemShmemFaultsRegisterNull)
{
    const char *shmFaultRegister = "ubs_mem_shm_fault_register";
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));

    MOCKER_CPP(&SystemAdapter::DlSym, void *(*)(void *handle, const char *name))
        .stubs()
        .will(invoke(RegisterFaultMock));

    EXPECT_EQ(UBSM_ERR_PARAM_INVALID, ubsmem_shmem_faults_register(nullptr));
    UbseMemExecutor::GetInstance().handle = nullptr;
}
} // namespace UT