/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "rack_mem_functions.h"

using testing::Test;

namespace UT {
using namespace ock::mxmd;

class RackMemFunctionsTestSuite : public Test {
public:
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

class Task : public Runnable {
public:
    void Run() override
    {
        std::cout << "task is executed" << std::endl;
    }
};

TEST_F(RackMemFunctionsTestSuite, TestGetThreadExecutorService)
{
    ASSERT_NE(GetOneThreadExecutorService(), nullptr);
    ASSERT_NE(GetMoreThreadExecutorService(), nullptr);
}

TEST_F(RackMemFunctionsTestSuite, TestMemStrUtil)
{
    uint64_t value = 0;
    ASSERT_TRUE(MemStrUtil::StrToULL("0x0", value, 16U));
    ASSERT_EQ(value, 0);
    ASSERT_TRUE(MemStrUtil::StrToULL("0x00001000", value, 16U));
    ASSERT_EQ(value, 4096);
    ASSERT_FALSE(MemStrUtil::StrToULL("xxxx", value, 16U));
    ASSERT_TRUE(MemStrUtil::StrToULL("0", value, 10U));
    ASSERT_EQ(value, 0);
    ASSERT_TRUE(MemStrUtil::StrToULL("123", value, 10U));
    ASSERT_EQ(value, 123);

    std::vector<std::string> out = MemStrUtil::SplitTrim("Test|Mem||Str|Util", "|");
    ASSERT_EQ(out.size(), (std::size_t)4);
    std::vector<std::string> out2 = MemStrUtil::SplitTrim("", "|");
    ASSERT_EQ(out2.size(), (std::size_t)0);
    std::vector<std::string> out3 = MemStrUtil::SplitTrim("NoSep", ",");
    ASSERT_EQ(out3.size(), (std::size_t)1);
}

TEST_F(RackMemFunctionsTestSuite, TestMinMemId)
{
    std::vector<uint64_t> ids{5, 3, 9, 1, 7};
    EXPECT_EQ(MinMemId(ids), 1);
    std::vector<uint64_t> emptyIds;
    EXPECT_EQ(MinMemId(emptyIds), INVALID_MEM_ID);
}

TEST_F(RackMemFunctionsTestSuite, TestMemToStr)
{
    std::vector<uint64_t> ids{100, 200, 300};
    std::string result = MemToStr(ids);
    EXPECT_EQ(result, "[100,200,300]");
    std::vector<uint64_t> single{42};
    EXPECT_EQ(MemToStr(single), "[42]");
    std::vector<uint64_t> empty;
    EXPECT_EQ(MemToStr(empty), "[]");
}

TEST_F(RackMemFunctionsTestSuite, TestGetErrCode)
{
    EXPECT_EQ(GetErrCode(MXM_OK), UBSM_OK);
    EXPECT_EQ(GetErrCode(MXM_ERR_NULLPTR), UBSM_ERR_PARAM_INVALID);
    EXPECT_EQ(GetErrCode(MXM_ERR_MEMLIB), UBSM_ERR_MEMLIB);
    EXPECT_EQ(GetErrCode(MXM_ERR_MALLOC_FAIL), UBSM_ERR_MALLOC_FAIL);
    EXPECT_EQ(GetErrCode(MXM_ERR_MEMORY), UBSM_ERR_MEMORY);
    EXPECT_EQ(GetErrCode(MXM_ERR_PARAM_INVALID), UBSM_ERR_PARAM_INVALID);
    EXPECT_EQ(GetErrCode(MXM_ERR_MMAP_VA_FAILED), UBSM_ERR_PARAM_INVALID);
    EXPECT_EQ(GetErrCode(MXM_ERR_CHECK_RESOURCE), UBSM_CHECK_RESOURCE_ERROR);
    EXPECT_EQ(GetErrCode(MXM_ERR_DAEMON_NO_FEATURE_ENABLED), UBSM_ERR_UNIMPL);
    EXPECT_EQ(GetErrCode(MXM_ERR_NAME_BUSY), UBSM_ERR_BUSY);
    EXPECT_EQ(GetErrCode(MXM_ERR_IPC_INIT_CALL), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_IPC_SYNC_CALL), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_IPC_HCOM_INNER_SYNC_CALL), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_IPC_CRC_CHECK_ERROR), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_IPC_SERIALIZE_DESERIALIZE_ERROR), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGISTER_OP_CODE), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGISTER_HANDLER_NULL), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_RPC_QUERY_ERROR), UBSM_ERR_NET);
    EXPECT_EQ(GetErrCode(MXM_ERR_LOCK_NOT_READY), UBSM_ERR_LOCK_NOT_SUPPORTED);
    EXPECT_EQ(GetErrCode(MXM_ERR_LOCK_ALREADY_LOCKED), UBSM_ERR_LOCK_ALREADY_LOCKED);
    EXPECT_EQ(GetErrCode(MXM_ERR_LOCK_NOT_FOUND), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_DLOCK_LIB), UBSM_ERR_DLOCK);
    EXPECT_EQ(GetErrCode(MXM_ERR_DLOCK_INNER), UBSM_ERR_DLOCK);
    EXPECT_EQ(GetErrCode(MXM_ERR_RECORD_DELETE), UBSM_ERR_RECORD);
    EXPECT_EQ(GetErrCode(MXM_ERR_RECORD_ADD), UBSM_ERR_RECORD);
    EXPECT_EQ(GetErrCode(MXM_ERR_RECORD_CHANGE_STATE), UBSM_ERR_RECORD);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGION_NOT_FOUND), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGION_PARAM_INVALID), UBSM_ERR_PARAM_INVALID);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGION_EXIST), UBSM_ERR_ALREADY_EXIST);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGION_NOT_EXIST), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_REGION_NO_NEEDED), UBSM_ERR_NO_NEEDED);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_NOT_FOUND), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_ALREADY_EXIST), UBSM_ERR_ALREADY_EXIST);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_PERMISSION_DENIED), UBSM_ERR_NOPERM);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_IN_USING), UBSM_ERR_IN_USING);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_NOT_EXIST), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_LEASE_NOT_FOUND), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_LEASE_NOT_EXIST), UBSM_ERR_NOT_FOUND);
    EXPECT_EQ(GetErrCode(MXM_ERR_UBSE_INNER), UBSM_ERR_UBSE);
    EXPECT_EQ(GetErrCode(MXM_ERR_UBSE_LIB), UBSM_ERR_UBSE);
    EXPECT_EQ(GetErrCode(MXM_ERR_UBSE_NOT_ATTACH), UBSM_ERR_UBSE);
    EXPECT_EQ(GetErrCode(MXM_ERR_UBSE_NOT_SUPPORTED), UBSM_ERR_NOT_SUPPORTED);
    EXPECT_EQ(GetErrCode(MXM_ERR_OBMM_INNER), UBSM_ERR_OBMM);
    EXPECT_EQ(GetErrCode(MXM_ERR_SHM_OBMM_OPEN), UBSM_ERR_OBMM);
    uint32_t unknownErr = 9999;
    EXPECT_EQ(GetErrCode(unknownErr), UBSM_ERR_BUFF);
}

TEST_F(RackMemFunctionsTestSuite, TestConvertBoolToString)
{
    EXPECT_EQ(ConvertBoolToString(true), "true");
    EXPECT_EQ(ConvertBoolToString(false), "false");
}

TEST_F(RackMemFunctionsTestSuite, TestConvertErrorToString)
{
    EXPECT_EQ(ConvertErrorToString(MXM_OK), "MXM_OK");
    EXPECT_NE(ConvertErrorToString(9999).find("UNKNOWN_ERROR"), std::string::npos);
}

TEST_F(RackMemFunctionsTestSuite, TestSafeMath)
{
    uint64_t result = 0;
    EXPECT_FALSE(SafeAdd(uint64_t(10), uint64_t(20), result));
    EXPECT_EQ(result, 30);
    EXPECT_FALSE(SafeSub(uint64_t(20), uint64_t(10), result));
    EXPECT_EQ(result, 10);
    EXPECT_FALSE(SafeMul(uint64_t(5), uint64_t(6), result));
    EXPECT_EQ(result, 30);
}

TEST_F(RackMemFunctionsTestSuite, TestStrToULLMore)
{
    uint64_t value = 0;
    EXPECT_FALSE(MemStrUtil::StrToULL("", value, 10U));
}

TEST_F(RackMemFunctionsTestSuite, TestGetHugeTlbPmdSize)
{
    auto size = GetHugeTlbPmdSize();
    EXPECT_TRUE(size == HUGE_PAGES_2M || size == HUGE_PAGES_512M);
}
} // namespace UT
