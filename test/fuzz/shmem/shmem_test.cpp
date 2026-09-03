/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <secodefuzz/secodeFuzz.h>
#include <mockcpp/mockcpp.hpp>
#include "ubs_mem.h"
#include "RmLibObmmExecutor.h"
#include "dlock_types.h"
#include "dt_fuzz.h"
#include "mls_manager.h"
#include "mxm_ipc_client_interface.cpp"
#include "rack_mem_fd_map.h"
#include "rack_mem_lib_common.h"
#include "shmem_stubs.h"
#include "system_adapter.h"
#include "ubse_mem_adapter_stub.h"
#include "ubsm_lock.h"
namespace FUZZ {
using namespace ock::com::ipc;
using namespace ock::dlock_utils;
static size_t SIZE = 1024 * 1024 * 1024;
int MockDLockSuccess()
{
    return dlock::DLOCK_SUCCESS;
}
class TestUbsmem : public testing::Test {
public:
    void SetUp() override
    {
        DT_Enable_Leak_Check(0, 0);
        DT_Set_Running_Time_Second(DT_RUNNING_TIME);
        MOCKER(MxmComStartIpcClient).stubs().will(returnValue(0));
        MOCKER(MxmComStopIpcClient).stubs().will(invoke(MxmComStopIpcClientStub));
        MOCKER(MxmComIpcClientSend).stubs().will(invoke(MxmComIpcClientSendStub));
        MOCKER(ock::mxmd::ObmmOpenInternal).stubs().will(returnValue(1));
        MOCKER_CPP(&ock::ubsm::SystemAdapter::Close, int (*)(int fd)).stubs().will(returnValue(0));
        auto &cfg = DLockContext::Instance().GetConfig();
        cfg.dlockDevName = "udma0";
        cfg.dlockDevEid = "0000:0000:0000:1000:0010:0000:df00:0404";
    }
    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

void MockTest()
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmImport, int (*)(const ImportShmParam &param, ImportShmResult &result))
        .stubs()
        .will(invoke(ShmImportStub));
    MOCKER_CPP(&ock::mxmd::RackMemFdMap::MapForEachFd, uint32_t(*)(void *startAddr, size_t length, int fd, int prot))
        .stubs()
        .will(returnValue(0));
}

void MockDlockTest()
{
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::ShmImport, int (*)(const ImportShmParam &param, ImportShmResult &result))
        .stubs()
        .will(invoke(ShmImportStub2));
    MOCKER_CPP(&ock::mxmd::RackMemFdMap::MapForEachFd, uint32_t(*)(void *startAddr, size_t length, int fd, int prot))
        .stubs()
        .will(returnValue(0));
    DLockExecutor::GetInstance().DLockClientLibInitFunc = static_cast<DLockClientLibInitPtr>(
        [](const struct dlock::client_cfg *pClientCfg) -> int { return dlock::DLOCK_SUCCESS; });
    DLockExecutor::GetInstance().DLockClientGetLockFunc = static_cast<DLockClientGetLockPtr>(
        [](int clientId, const struct dlock::lock_desc *desc, int *lockId) -> int { return dlock::DLOCK_SUCCESS; });
    DLockExecutor::GetInstance().DLockTryLockFunc = static_cast<DLockTryLockPtr>(
        [](int clientId, const struct dlock::lock_request *req, void *result) -> int { return dlock::DLOCK_SUCCESS; });
    DLockExecutor::GetInstance().DLockUnlockFunc =
        static_cast<DLockUnlockPtr>([](int clientId, int lockId, void *result) -> int { return dlock::DLOCK_SUCCESS; });
    DLockExecutor::GetInstance().DLockReleaseLockFunc =
        static_cast<DLockReleaseLockPtr>([](int clientId, int lockId) -> int { return dlock::DLOCK_SUCCESS; });
    MOCKER_CPP(&DLockExecutor::InitDLockClientDlopenLib, int32_t(*)()).stubs().will(returnValue(0));
    MOCKER_CPP(&DLockExecutor::ClientInitWrapper, int (*)(int *clientId, const char *serverIp))
        .stubs()
        .will(returnValue(0));
}

TEST_F(TestUbsmem, Test_ubsmem_init_attributes)
{
    char fuzzName[] = "Test_ubsmem_init_attributes";
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        bool passNull = bool(*(u8 *)DT_SetGetNumberRangeV3(0, 0, 0, 1));

        ubsmem_options_t opts;

        int ret = passNull ? ubsmem_init_attributes(nullptr) : ubsmem_init_attributes(&opts);

        if (passNull) {
            ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID) << "Expected MXM_ERR_PARAM_INVALID when passing NULL";
        } else {
            ASSERT_EQ(ret, UBSM_OK) << "Expected UBSM_OK when passing valid pointer";
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, Test_ubsmem_init)
{
    char fuzzName[] = "Test_ubsmem_init";
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        bool passNull = bool(*(u8 *)DT_SetGetNumberRangeV3(0, 0, 0, 1));

        ubsmem_options_t opts;
        int ret = passNull ? ubsmem_initialize(nullptr) : ubsmem_initialize(&opts);

        if (passNull) {
            ASSERT_EQ(ret, MXM_ERR_PARAM_INVALID) << "Expected MXM_ERR_PARAM_INVALID when passing NULL";
        } else {
            ASSERT_EQ(ret, UBSM_OK) << "Expected UBSM_OK when passing valid pointer";
        }
    }
    DT_FUZZ_END()
}

static bool IsInvalidCreateParam(const char *safeRegionName, uint64_t size, const ubsmem_region_attributes_t *reg_attr,
                                 int i)
{
    return (safeRegionName == nullptr || reg_attr == nullptr || strlen(safeRegionName) == 0 || size != 0 ||
            reg_attr->host_num > MAX_REGION_NODE_NUM || reg_attr->host_num < NO_2 ||
            strnlen(safeRegionName, MAX_REGION_NAME_DESC_LENGTH) >= MAX_REGION_NAME_DESC_LENGTH ||
            strnlen(reg_attr->hosts[i].host_name, MAX_HOST_NAME_DESC_LENGTH) == 0 ||
            strnlen(reg_attr->hosts[i].host_name, MAX_HOST_NAME_DESC_LENGTH) >= MAX_HOST_NAME_DESC_LENGTH);
}

static bool IsInvalidLookupParam(const char *safeRegionName, const ubsmem_region_desc_t *regionDesc)
{
    return (safeRegionName == nullptr || regionDesc == nullptr || strlen(safeRegionName) == 0 ||
            strnlen(safeRegionName, MAX_REGION_NAME_DESC_LENGTH) >= MAX_REGION_NAME_DESC_LENGTH);
}

static bool IsInvalidDestroyParam(const char *safeRegionName)
{
    return (safeRegionName == nullptr || strlen(safeRegionName) == 0 ||
            strnlen(safeRegionName, MAX_REGION_NAME_DESC_LENGTH) >= MAX_REGION_NAME_DESC_LENGTH);
}

TEST_F(TestUbsmem, Test_ubsmem_create_region)
{
    char fuzzName[] = "Test_ubsmem_create_region";
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::LookupRegionList, HRESULT(*)(SHMRegions & regions))
        .stubs()
        .will(invoke(FUZZ::LookupRegionListFuzzStub));

    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char regionNameTest[] = "ubsmem_create_region";
        const char *regionName = DT_SetGetString(&g_Element[0], strlen(regionNameTest) + 1, 1024, regionNameTest);

        char safeRegionName[MAX_REGION_NAME_DESC_LENGTH];
        if (regionName != nullptr) {
            strncpy_s(safeRegionName, MAX_REGION_NAME_DESC_LENGTH, regionName,
                      strnlen(safeRegionName, MAX_REGION_NAME_DESC_LENGTH));
        }

        uint64_t size = 0;

        ubsmem_region_attributes_t reg_attr_obj = {};
        ubsmem_region_attributes_t *reg_attr = &reg_attr_obj;
        reg_attr->host_num = 2;
        printf("host_num: %lu\n", reg_attr->host_num);
        for (int i = 0; i < reg_attr->host_num && i < MAX_REGION_NODE_NUM; ++i) {
            const char *fixedHostName = "1";
            strncpy_s(reg_attr->hosts[i].host_name, MAX_HOST_NAME_DESC_LENGTH, fixedHostName,
                      strnlen(fixedHostName, MAX_HOST_NAME_DESC_LENGTH));
            reg_attr->hosts[i].affinity = static_cast<bool>(*(u8 *)DT_SetGetNumberRangeV3(0, 0, 0, 1));
        }

        ubsmem_region_desc_t regionDesc_obj = {};
        ubsmem_region_desc_t *regionDesc = &regionDesc_obj;
        strncpy_s(regionDesc->region_name, MAX_REGION_NAME_DESC_LENGTH, safeRegionName,
                  strnlen(safeRegionName, MAX_REGION_NAME_DESC_LENGTH));

        regionDesc->size = size;
        regionDesc->region_attr = *reg_attr;

        ubsmem_regions_t region_t_obj = {};
        ubsmem_regions_t *region_t = &region_t_obj;
        region_t->num = 1;
        region_t->region[0] = *reg_attr;

        int ret = ubsmem_create_region(safeRegionName, size, reg_attr);

        for (int i = 0; i < reg_attr->host_num && i < MAX_REGION_NODE_NUM; ++i) {
            if (IsInvalidCreateParam(safeRegionName, size, reg_attr, i)) {
                EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
            } else {
                EXPECT_EQ(ret, UBSM_OK);
            }
        }

        int ret_lookup_region = ubsmem_lookup_region(safeRegionName, regionDesc);
        for (int i = 0; i < reg_attr->host_num && i < MAX_REGION_NODE_NUM; ++i) {
            if (IsInvalidLookupParam(safeRegionName, regionDesc)) {
                EXPECT_EQ(ret_lookup_region, MXM_ERR_PARAM_INVALID);
            } else {
                EXPECT_EQ(ret_lookup_region, UBSM_OK);
            }
        }

        int ret_destroy_region = ubsmem_destroy_region(safeRegionName);
        for (int i = 0; i < reg_attr->host_num && i < MAX_REGION_NODE_NUM; ++i) {
            if (IsInvalidDestroyParam(safeRegionName)) {
                EXPECT_EQ(ret_destroy_region, MXM_ERR_PARAM_INVALID);
            } else {
                EXPECT_EQ(ret_destroy_region, UBSM_OK);
            }
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_allocate)
{
    char fuzzName[] = "TEST_ubsmem_shmem_allocate";
    RegionInfo expected;
    MOCKER_CPP(&MxmServerMsgHandle::RegionLookupRegion,
               int (*)(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo))
        .stubs()
        .will(invoke(RegionLookupRegionStub));
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::LookupRegionList, HRESULT(*)(SHMRegions & regions))
        .stubs()
        .will(invoke(UT::LookupRegionListStub));
    MOCKER_CPP(&MxmServerMsgHandle::ShmCreate, int (*)(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo))
        .stubs()
        .will(invoke(ShmCreateStub));
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char regionNameTest[] = "shmem_allocate";
        char nameTest[] = "test";
        const char *regionName = DT_SetGetString(&g_Element[0], strlen(regionNameTest) + 1, 1024, regionNameTest);
        const char *name = DT_SetGetString(&g_Element[1], strlen(nameTest) + 1, 1024, nameTest);
        mode_t mode = static_cast<mode_t>(1);
        int ret = ubsmem_shmem_allocate(regionName, name, SIZE, mode, 0);
        if (name == nullptr || regionName == nullptr || strlen(name) == 0 || strlen(regionName) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH ||
            strnlen(regionName, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
            ret = ubsmem_shmem_deallocate(name);
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_deallocate)
{
    char fuzzName[] = "TEST_ubsmem_shmem_deallocate";
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int ret = ubsmem_shmem_deallocate(name);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_map)
{
    char fuzzName[] = "TEST_ubsmem_shmem_map";
    MockTest();
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumProt[] = {PROT_NONE, PROT_READ, PROT_READ | PROT_WRITE};
        int prot = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], PROT_READ, enumProt, 3);
        void *localPtr = nullptr;
        int ret = ubsmem_shmem_map(0, SIZE, prot, 1, name, 0, &localPtr);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
            ret = ubsmem_shmem_unmap(localPtr, SIZE);
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_set_ownership)
{
    char fuzzName[] = "TEST_ubsmem_shmem_set_ownership";
    MockTest();
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumProt[] = {PROT_NONE, PROT_READ, PROT_READ | PROT_WRITE};
        int prot = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], PROT_READ, enumProt, 3);
        void *localPtr = nullptr;
        int ret = ubsmem_shmem_set_ownership(name, localPtr, SIZE, prot);
        EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        int ret1 = ubsmem_shmem_map(0, SIZE, prot, 1, name, 0, &localPtr);
        ret = ubsmem_shmem_set_ownership(name, localPtr, SIZE, prot);
        if (ret1 == UBSM_OK) {
            EXPECT_EQ(ret, UBSM_OK);
            ubsmem_shmem_unmap(localPtr, SIZE);
        } else {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_write_lock)
{
    char fuzzName[] = "TEST_ubsmem_shmem_write_lock";
    MockDlockTest();
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumProt[] = {PROT_NONE, PROT_READ, PROT_READ | PROT_WRITE};
        int prot = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], PROT_READ, enumProt, 3);
        void *localPtr = nullptr;
        UbsmLock::Instance().Init();
        int ret = ubsmem_shmem_write_lock(name);
        EXPECT_NE(ret, UBSM_OK);
        int ret1 = ubsmem_shmem_map(0, SIZE, PROT_READ, 1, name, 0, &localPtr);
        ret = ubsmem_shmem_write_lock(name);
        if (ret1 == UBSM_OK) {
            EXPECT_EQ(ret, UBSM_OK);
            ubsmem_shmem_unlock(name);
            ubsmem_shmem_unmap(localPtr, SIZE);
        } else {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        }
        UbsmLock::Instance().DeInit();
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_read_lock)
{
    char fuzzName[] = "TEST_ubsmem_shmem_read_lock";
    MockDlockTest();
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumProt[] = {PROT_NONE, PROT_READ, PROT_READ | PROT_WRITE};
        int prot = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], PROT_READ, enumProt, 3);
        void *localPtr = nullptr;
        UbsmLock::Instance().Init();
        int ret = ubsmem_shmem_read_lock(name);
        EXPECT_NE(ret, UBSM_OK);
        int ret1 = ubsmem_shmem_map(0, SIZE, PROT_READ, 1, name, 0, &localPtr);
        ret = ubsmem_shmem_read_lock(name);
        if (ret1 == UBSM_OK) {
            EXPECT_EQ(ret, UBSM_OK);
            ubsmem_shmem_unlock(name);
            ubsmem_shmem_unmap(localPtr, SIZE);
        } else {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        }
        UbsmLock::Instance().DeInit();
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_unlock)
{
    char fuzzName[] = "TEST_ubsmem_shmem_unlock";
    MockDlockTest();
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumProt[] = {PROT_NONE, PROT_READ, PROT_READ | PROT_WRITE};
        int prot = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], PROT_READ, enumProt, 3);
        void *localPtr = nullptr;
        UbsmLock::Instance().Init();
        int ret = ubsmem_shmem_unlock(name);
        EXPECT_NE(ret, UBSM_OK);
        ubsmem_shmem_map(0, SIZE, PROT_READ, 1, name, 0, &localPtr);
        int ret1 = ubsmem_shmem_read_lock(name);
        ret = ubsmem_shmem_unlock(name);
        if (ret1 == UBSM_OK) {
            EXPECT_EQ(ret, UBSM_OK);
            ubsmem_shmem_unmap(localPtr, SIZE);
        } else {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        }
        UbsmLock::Instance().DeInit();
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_list_lookup)
{
    char fuzzName[] = "TEST_ubsmem_shmem_list_lookup";

    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *prefix = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        uint32_t shm_cnt = 1;
        ubsmem_shmem_desc_t shm_list_obj = {};
        ubsmem_shmem_desc_t *shm_list = &shm_list_obj;

        int ret = ubsmem_shmem_list_lookup(prefix, shm_list, &shm_cnt);
        if (prefix == nullptr || strnlen(prefix, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(prefix, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_lookup)
{
    char fuzzName[] = "TEST_ubsmem_shmem_lookup";

    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        ubsmem_shmem_info_t shm_info_obj = {};
        ubsmem_shmem_info_t *shm_info = &shm_info_obj;

        int ret = ubsmem_shmem_lookup(name, shm_info);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_attach)
{
    char fuzzName[] = "TEST_ubsmem_shmem_attach";

    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int ret = ubsmem_shmem_attach(name);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_shmem_detach)
{
    char fuzzName[] = "ubsmem_shmem_detach";

    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        ubsmem_shmem_attach(name);
        int ret = ubsmem_shmem_detach(name);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, TEST_ubsmem_lease_malloc)
{
    char fuzzName[] = "TEST_ubsmem_lease_malloc";
    ubsmem_distance_t distance;
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::LeaseMalloc,
               int (*)(const ock::mxm::LeaseMallocParam &param, ock::mxm::LeaseMallocResult &result))
        .stubs()
        .will(invoke(LeaseMallocStub));
    MOCKER_CPP(&ock::mxmd::RackMemFdMap::MapForEachFd, uint32_t(*)(void *startAddr, size_t length, int fd, int prot))
        .stubs()
        .will(returnValue(0));
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumRange[] = {0, 1};
        int distance = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], 0, enumRange, 2);
        void *localPtr = nullptr;
        int ret = ubsmem_lease_malloc(name, SIZE, static_cast<ubsmem_distance_t>(distance), false, &localPtr);
        if (name == nullptr || strnlen(name, MAX_SHM_NAME_LENGTH) == 0 ||
            strnlen(name, MAX_SHM_NAME_LENGTH) >= MAX_SHM_NAME_LENGTH || distance != 0) {
            EXPECT_EQ(ret, MXM_ERR_PARAM_INVALID);
        } else {
            EXPECT_EQ(ret, UBSM_OK);
            ubsmem_lease_free(localPtr);
        }
    }
    DT_FUZZ_END()
}

TEST_F(TestUbsmem, Test_ubsmem_lease_free)
{
    char fuzzName[] = "Test_ubsmem_lease_free";
    MOCKER_CPP(&ock::mxm::UbseMemAdapter::LeaseMalloc,
               int (*)(const ock::mxm::LeaseMallocParam &param, ock::mxm::LeaseMallocResult &result))
        .stubs()
        .will(invoke(LeaseMallocStub));
    MOCKER_CPP(&ock::mxmd::RackMemFdMap::MapForEachFd, uint32_t(*)(void *startAddr, size_t length, int fd, int prot))
        .stubs()
        .will(returnValue(0));
    DT_FUZZ_START(0, CAPI_FUZZ_COUNT, fuzzName, 0)
    {
        char nameTest[] = "test";
        const char *name = DT_SetGetString(&g_Element[0], strlen(nameTest) + 1, 1024, nameTest);
        int enumRange[] = {0, 1};
        int distance = *(uint32_t *)DT_SetGetNumberEnum(&g_Element[1], 0, enumRange, 2);
        void *localPtr = nullptr;
        int ret1 = ubsmem_lease_malloc(name, SIZE, static_cast<ubsmem_distance_t>(distance), false, &localPtr);
        int ret = ubsmem_lease_free(localPtr);
        if (ret1 == UBSM_OK) {
            EXPECT_EQ(ret, UBSM_OK);
        } else {
            EXPECT_NE(ret, UBSM_OK);
        }
    }
    DT_FUZZ_END()
}

} // namespace FUZZ