/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <mockcpp/mockcpp.hpp>

#include "RackMemShm.h"
#include "mx_shm.h"
#include "rack_mem_lib.h"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))

namespace UT {
using namespace ock::mxmd;
using namespace ock::common;

class RackMemShmMmapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_name = "test_shmem";
        valid_length = 1024;
        valid_offset = 0;

        // 设置无效的测试数据
        long_name = std::string(MEM_MAX_ID_LENGTH + 1, 'a'); // 超过限制的名称
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    size_t valid_length;
    off_t valid_offset;
    std::string long_name;
    void *unaligned_start;
};

class RackMemShmUnmmapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_start = reinterpret_cast<void *>(0x1000); // 对齐的地址
        valid_length = 1024;

        // 设置无效的测试数据
        unaligned_start = reinterpret_cast<void *>(0x1001); // 未对齐的地址
        zero_length = 0;
        negative_length = static_cast<size_t>(-1); // 可能会被视为很大的正数
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void *valid_start;
    void *unaligned_start;
    size_t valid_length;
    size_t zero_length;
    size_t negative_length;
};

class RackMemShmOptTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_start = reinterpret_cast<void *>(0x1000); // 对齐的地址
        valid_length = 2;

        // 设置无效的测试数据
        null_start = nullptr;
        zero_length = 0;

        // 设置缓存操作类型
        valid_cache_opt = ShmCacheOpt::RACK_FLUSH;

        // 设置所有权状态
        valid_own_status = ShmOwnStatus::READWRITE;
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    void *valid_start;
    void *null_start;
    size_t valid_length;
    size_t zero_length;
    ShmCacheOpt valid_cache_opt;
    ShmOwnStatus valid_own_status;
};

class RackMemGetShmMemFaultStatusTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置有效的测试数据
        valid_name = "test_shared_memory";
        valid_isShmFault = &isShmFault;

        // 设置无效的测试数据
        long_name = std::string(MEM_MAX_ID_LENGTH + 1, 'a'); // 超过限制的名称
        null_name = nullptr;
        null_isShmFault = nullptr;
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }

    std::string valid_name;
    std::string long_name;
    const char *null_name;
    bool isShmFault;
    bool *valid_isShmFault;
    bool *null_isShmFault;
};

class RpcQueryNodeInfoTest : public testing::Test {
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(RpcQueryNodeInfoTest, NormalCase)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));

    MOCKER_CPP(&RackMemShm::RpcQueryInfo, int (*)(std::string &)).expects(once()).will(returnValue(0));

    int result = RpcQueryNodeInfo();
    EXPECT_EQ(0, result);
}

TEST_F(RpcQueryNodeInfoTest, RpcQueryInfoFailed)
{
    MOCKER_CPP(&RackMemLib::StartRackMem, int (*)()).stubs().will(returnValue(0));

    MOCKER_CPP(&RackMemShm::RpcQueryInfo, int (*)(std::string &)).expects(once()).will(returnValue(-1));

    int result = RpcQueryNodeInfo();
    EXPECT_EQ(-1, result);
}
} // namespace UT