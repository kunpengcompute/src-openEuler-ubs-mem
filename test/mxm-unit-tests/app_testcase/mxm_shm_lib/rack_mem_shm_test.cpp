/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "RackMemShm.cpp"
#include "RackMemShm.h"
#include "ipc_proxy.h"
#include "shm_ipc_command.h"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
namespace UT {
using namespace ock::mxmd;

class RackMemShmTest : public testing::Test {
protected:
    void SetUp() override
    {
        validAddr = malloc(4);
    }

    void TearDown() override
    {
        free(validAddr);
        validAddr = nullptr;
        mockcpp::GlobalMockObject::reset();
    }

    void *validAddr;
    size_t validSize = 4096;
    std::string validName = "test_shm";
    ShmAppMetaData shmMetaData;
    std::vector<uint64_t> memIds = {123, 456};
};

TEST_F(RackMemShmTest, TestUbsMemShmCreateParameterCheckFailure)
{
    std::string invalidNid = "invalidNid";
    SHMRegionDesc mockRegionDesc = {};
    MOCKER(RackMemShm::CheckRegionPar)
        .stubs()
        .with(eq(invalidNid), any())
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_MALLOC_FAIL)));

    uint32_t result =
        RackMemShm::GetInstance().UbsMemShmCreate("region", "name", 1024, invalidNid, mockRegionDesc, 0, 0644);
    EXPECT_EQ(result, MXM_ERR_MALLOC_FAIL);
}

TEST_F(RackMemShmTest, TestUbsMemShmCreateIpcCallFailure)
{
    std::string validNid = "validNid";
    SHMRegionDesc mockRegionDesc = {};
    const uint32_t expectedError = MXM_ERR_MALLOC_FAIL;

    MOCKER(RackMemShm::CheckRegionPar)
        .stubs()
        .with(eq(validNid), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    MOCKER(ShmIpcCommand::IpcCallShmCreate)
        .stubs()
        .with(any(), any(), any(), any(), any(), any(), any())
        .will(returnValue(expectedError));

    uint32_t result =
        RackMemShm::GetInstance().UbsMemShmCreate("region", "name", 1024, validNid, mockRegionDesc, 0, 0644);
    EXPECT_EQ(result, expectedError);
}

TEST_F(RackMemShmTest, TestUbsMemShmCreateSuccess)
{
    std::string validNid = "validNid";
    SHMRegionDesc mockRegionDesc = {};
    MOCKER(RackMemShm::CheckRegionPar)
        .stubs()
        .with(eq(validNid), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    MOCKER(ShmIpcCommand::IpcCallShmCreate)
        .stubs()
        .with(any(), any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    uint32_t result =
        RackMemShm::GetInstance().UbsMemShmCreate("region", "name", 1024, validNid, mockRegionDesc, 0, 0644);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestUnMapWhenErrorCallsCleanupFunctions)
{
    MOCKER(SystemAdapter::MemoryUnMap).stubs().with(any(), any()).will(returnValue(-1));

    MOCKER(ShmIpcCommand::IpcCallShmUnMap)
        .stubs()
        .with(eq(validName))
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_MALLOC_FAIL)));

    shmMetaData.AddAddr(validAddr, validSize);
    shmMetaData.AddFds(123);

    UnMapWhenError(validAddr, validSize, validName, shmMetaData);

    EXPECT_TRUE(shmMetaData.addr == nullptr);
    EXPECT_TRUE(shmMetaData.fd == -1);
}

TEST_F(RackMemShmTest, TestParameterCheckNameAlreadyMapped)
{
    MOCKER_CPP(&ShmMetaDataMgr::GetAddr, void *(*)(const std::string &name))
        .stubs()
        .with(any())
        .will(returnValue(validAddr));

    uint32_t result = RackMemShmMmapParameterCheck(nullptr, validSize, PROT_READ, MAP_SHARED, validName, 0);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestParameterCheckZeroSize)
{
    uint32_t result = RackMemShmMmapParameterCheck(nullptr, 0, PROT_READ, MAP_SHARED, validName, 0);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestParameterCheckNonZeroOffset)
{
    uint32_t result = RackMemShmMmapParameterCheck(nullptr, validSize, PROT_READ, MAP_SHARED, validName, 1);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestParameterCheckInvalidProt)
{
    uint32_t result = RackMemShmMmapParameterCheck(validAddr, validSize, PROT_EXEC, MAP_SHARED, validName, 0);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestParameterCheckInvalidFlags)
{
    uint32_t result = RackMemShmMmapParameterCheck(nullptr, validSize, PROT_READ, MAP_FIXED, validName, 0);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestParameterCheckSuccess)
{
    uint32_t result = RackMemShmMmapParameterCheck(nullptr, validSize, PROT_READ, MAP_SHARED, validName, 0);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestMmapByMemIDOpenFailure)
{
    MOCKER(ObmmOpenInternal).stubs().with(any(), any(), any()).will(returnValue(-1));

    uint32_t result =
        RackMemShmMmapByMemID(validAddr, validSize, 2, memIds, shmMetaData, O_RDWR, 0, PROT_READ, MAP_SHARED);
    EXPECT_EQ(result, MXM_ERR_SHM_OBMM_OPEN);
}

TEST_F(RackMemShmTest, TestMmapByMemIDMapFailure)
{
    MOCKER(ObmmOpenInternal).stubs().with(any(), any(), any()).will(returnValue(123));

    MOCKER_CPP(&RackMemFdMap::MapForEachFd, uint32_t(*)(void *, size_t, int, int, int, off_t))
        .stubs()
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_MMAP_VA_FAILED)));

    uint32_t result =
        RackMemShmMmapByMemID(validAddr, validSize, 2, memIds, shmMetaData, O_RDWR, 0, PROT_READ, MAP_SHARED);
    EXPECT_EQ(result, MXM_ERR_MMAP_VA_FAILED);
}

TEST_F(RackMemShmTest, TestMmapByMemIDSuccess)
{
    MOCKER(ObmmOpenInternal).stubs().with(any(), any(), any()).will(returnValue(123));

    MOCKER_CPP(&RackMemFdMap::MapForEachFd, uint32_t(*)(void *, size_t, int, int, int, off_t))
        .stubs()
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    uint32_t result =
        RackMemShmMmapByMemID(validAddr, validSize, 2, memIds, shmMetaData, O_RDWR, 0, PROT_READ, MAP_SHARED);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestGetOwnStatusAllCases)
{
    EXPECT_EQ(GetOwnStatus(PROT_NONE), ShmOwnStatus::UNACCESS);
    EXPECT_EQ(GetOwnStatus(PROT_READ), ShmOwnStatus::READ);
    EXPECT_EQ(GetOwnStatus(PROT_WRITE), ShmOwnStatus::WRITE);
    EXPECT_EQ(GetOwnStatus(PROT_READ | PROT_WRITE), ShmOwnStatus::READWRITE);
    EXPECT_EQ(GetOwnStatus(PROT_EXEC), OWNBUFF);
}

TEST_F(RackMemShmTest, TestConvertToFileModeAllCases)
{
    EXPECT_EQ(convertToFileMode(ShmOwnStatus::WRITE), S_IWUSR);
    EXPECT_EQ(convertToFileMode(ShmOwnStatus::READ), S_IRUSR);
    EXPECT_EQ(convertToFileMode(ShmOwnStatus::READWRITE), S_IRUSR | S_IWUSR);
    EXPECT_EQ(convertToFileMode(ShmOwnStatus::UNACCESS), 0);
    EXPECT_EQ(convertToFileMode(static_cast<ShmOwnStatus>(999)), 0);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapNullLocalPtr)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void **local_ptr = nullptr; // 故意设置为nullptr

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapInvalidProt)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_NONE;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;

    // 模拟GetOwnStatus返回无效状态
    MOCKER(GetOwnStatus).stubs().with(prot).will(returnValue(OWNBUFF));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapParameterCheckFailure)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;

    // 模拟参数检查失败
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<int>(MXM_ERR_PARAM_INVALID)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapIpcMapFailure)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用失败
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), any(), any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_MALLOC_FAIL)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_MALLOC_FAIL);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapSizeMismatch)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用成功但返回大小不匹配
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), any(), outBound(mapSize * 2), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟取消映射调用
    MOCKER(ShmIpcCommand::IpcCallShmUnMap).stubs().with(any()).will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapFileMapFailure)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用成功
    std::vector<uint64_t> mockMemIds;
    size_t mockUnitSize = 4096;
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), outBound(mockMemIds), outBound(mapSize), outBound(mockUnitSize), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟文件映射失败
    MOCKER_CPP(&RackMemShm::GetPreAllocateAddress, int (*)(void *start, size_t length, int flags, void **result))
        .stubs()
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_MMAP_VA_FAILED)));

    // 模拟取消映射调用
    MOCKER(ShmIpcCommand::IpcCallShmUnMap).stubs().with(any()).will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_MMAP_VA_FAILED);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapMemIdMapFailure)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;
    std::vector<uint64_t> memIds = {123, 456};
    size_t unitSize = 8;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用成功
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), outBound(memIds), outBound(mapSize), outBound(unitSize), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟内存ID映射失败
    MOCKER(RackMemShmMmapByMemID).stubs().will(returnValue(static_cast<uint32_t>(MXM_ERR_MALLOC_FAIL)));

    // 模拟错误处理函数
    MOCKER(UnMapWhenError).stubs().with(any(), any(), any(), any()).will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_MALLOC_FAIL);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapAddMetaDataFailure)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;
    std::vector<uint64_t> memIds = {123, 456};
    size_t unitSize = 8;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用成功
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), outBound(memIds), outBound(mapSize), outBound(unitSize), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟内存ID映射成功
    MOCKER(RackMemShmMmapByMemID).stubs().will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟添加元数据失败
    MOCKER_CPP(&ShmMetaDataMgr::AddMetaData,
               uint32_t(*)(const std::string &name, void *mAddr, ShmAppMetaData &shmAppMetaData))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(MXM_ERR_NULLPTR)));

    // 模拟错误处理函数
    MOCKER(UnMapWhenError).stubs().with(any(), any(), any(), any()).will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, MXM_ERR_NULLPTR);
}

TEST_F(RackMemShmTest, TestUbsMemShmMmapSuccess)
{
    // 定义测试变量
    void *start = nullptr;
    size_t mapSize = 4096;
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    std::string name = "test_shm";
    off_t off = 0;
    void *local_ptr = nullptr;
    void **local_ptr_addr = &local_ptr;
    std::vector<uint64_t> memIds = {123, 456};
    size_t unitSize = 8;

    // 模拟参数检查成功
    MOCKER(RackMemShmMmapParameterCheck)
        .stubs()
        .with(any(), any(), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟IPC调用成功
    MOCKER(ShmIpcCommand::IpcCallShmMap)
        .stubs()
        .with(any(), any(), outBound(memIds), outBound(mapSize), outBound(unitSize), any(), any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟内存ID映射成功
    MOCKER(RackMemShmMmapByMemID).stubs().will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    // 模拟添加元数据成功
    MOCKER_CPP(&ShmMetaDataMgr::AddMetaData,
               uint32_t(*)(const std::string &name, void *mAddr, ShmAppMetaData &shmAppMetaData))
        .stubs()
        .with(any(), any(), any())
        .will(returnValue(static_cast<uint32_t>(UBSM_OK)));

    int result = RackMemShm::GetInstance().UbsMemShmMmap(start, mapSize, prot, flags, name, off, local_ptr_addr);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestUbsMemShmDeleteFailure)
{
    MOCKER(ShmIpcCommand::IpcCallShmDelete).stubs().will(returnValue(static_cast<uint32_t>(MXM_ERR_MALLOC_FAIL)));
    auto result = RackMemShm::GetInstance().UbsMemShmDelete("test");
    ASSERT_EQ(result, MXM_ERR_MALLOC_FAIL);
}

TEST_F(RackMemShmTest, TestUbsMemShmDeleteSuccess)
{
    MOCKER(ShmIpcCommand::IpcCallShmDelete).stubs().will(returnValue(static_cast<uint32_t>(UBSM_OK)));
    auto result = RackMemShm::GetInstance().UbsMemShmDelete("test");
    ASSERT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestCheckRegionParNumInvalidHigh)
{
    // 准备测试数据
    std::string baseNid = "node1";
    SHMRegionDesc region;
    region.num = MEM_TOPOLOGY_MAX_HOSTS + 1; // 超出最大值
    region.type = RackMemShmRegionType::ONE2ALL_SHARE;

    // 执行测试
    uint32_t result = RackMemShm::CheckRegionPar(baseNid, region);

    // 验证结果
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestCheckRegionParNumInvalidZero)
{
    SHMRegionDesc region;
    region.num = 0; // 无效数值
    region.type = RackMemShmRegionType::ALL2ALL_SHARE;

    uint32_t result = RackMemShm::CheckRegionPar("node1", region);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestCheckRegionParTypeInvalid)
{
    SHMRegionDesc region;
    region.num = 3;
    region.type = static_cast<RackMemShmRegionType>(999); // 999 为模拟的无效类型

    uint32_t result = RackMemShm::CheckRegionPar("node1", region);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestCheckRegionParBaseNidEmpty)
{
    SHMRegionDesc region;
    region.num = 2;
    region.type = RackMemShmRegionType::ALL2ALL_SHARE;

    // 空baseNid直接返回成功
    uint32_t result = RackMemShm::CheckRegionPar("", region);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestCheckRegionParOne2AllNodeMismatch)
{
    std::string baseNid = "node1";
    SHMRegionDesc region;
    region.num = 1;
    region.type = RackMemShmRegionType::ONE2ALL_SHARE;
    strcpy_s(region.nodeId[0], 15, "different_node"); // 不匹配 baseNid

    std::cout << region.nodeId[0] << std::endl;
    uint32_t result = RackMemShm::CheckRegionPar(baseNid, region);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestCheckRegionParOne2AllValid)
{
    std::string baseNid = "node1";
    SHMRegionDesc region;
    region.num = 1;
    region.type = RackMemShmRegionType::ONE2ALL_SHARE;
    strcpy_s(region.nodeId[0], 6, baseNid.c_str());

    uint32_t result = RackMemShm::CheckRegionPar(baseNid, region);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestCheckRegionParAll2AllMissingBase)
{
    std::string baseNid = "node_missing";
    SHMRegionDesc region;
    region.num = 3;
    region.type = RackMemShmRegionType::ALL2ALL_SHARE;
    strcpy_s(region.nodeId[0], 6, "node1");
    strcpy_s(region.nodeId[1], 6, "node2");
    strcpy_s(region.nodeId[2], 6, "node3");

    uint32_t result = RackMemShm::CheckRegionPar(baseNid, region);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestCheckRegionParAll2AllValid)
{
    std::string baseNid = "node2";
    SHMRegionDesc region;
    region.num = 3;
    region.type = RackMemShmRegionType::ALL2ALL_SHARE;
    strcpy_s(region.nodeId[0], 6, "node1");
    strcpy_s(region.nodeId[1], 6, "node2");
    strcpy_s(region.nodeId[2], 6, "node3");

    uint32_t result = RackMemShm::CheckRegionPar(baseNid, region);
    EXPECT_EQ(result, UBSM_OK);
}

TEST_F(RackMemShmTest, TestSetUbsMemShmSetOwnerShipInvalidParameter)
{
    std::string name = "test";
    void *start = nullptr;
    size_t length = 10;
    ShmOwnStatus status = READWRITE;
    auto ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShip(name, start, length, status);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);

    name = "";
    start = reinterpret_cast<void *>(0x123456789);
    length = 10;
    status = READWRITE;
    ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShip(name, start, length, status);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);

    name = "test";
    start = reinterpret_cast<void *>(0x123456789);
    length = 0;
    status = READWRITE;
    ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShip(name, start, length, status);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}

TEST_F(RackMemShmTest, TestUbsMemShmSetOwnerShipCheckAddrFaiure)
{
    std::string name = "test";
    void *start = reinterpret_cast<void *>(0x123456789);
    size_t length = 10;
    ShmOwnStatus status = READWRITE;
    MOCKER_CPP(&ShmMetaDataMgr::CheckAddr,
               int32_t(*)(std::string name, void *start, size_t length, ShmAppMetaData &shmAppMetaData,
                          uint64_t &usedFirst, std::vector<int> &indices))
        .stubs()
        .will(returnValue(static_cast<int32_t>(MXM_ERR_PARAM_INVALID)));

    auto ret = RackMemShm::GetInstance().UbsMemShmSetOwnerShip(name, start, length, status);
    ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID);
}
} // namespace UT