/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "record_store.h"
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "rack_mem_constants.h"

namespace UT {
using namespace ock::ubsm;
using namespace ock::mxmd;
static constexpr auto TEST_REGION_NAME = "TEST_REGION_NAME";
static constexpr auto TEST_LEASE_NAME = "TEST_LEASE_NAME";
static constexpr auto TEST_SHM_NAME = "TEST_SHM_NAME";

AppContext TEST_APP_CTX = {65535, 1011, 1011};
CreateRegionInput TEST_REGION_RECORD{};
MemLeaseInfo TEST_LEASE_RECORD{};
ShareMemImportInfo TEST_SHM_IMPORT_RECORD{};

std::unique_ptr<MemIdRecordPool> ptr = std::make_unique<MemIdRecordPool>();
RecordIdPoolAllocator poolAllocator_;

class RecordStoreTest : public testing::Test {
public:
    void SetUp() override {}

    void TearDown() override {}

    static void TearDownTestSuite()
    {
        poolAllocator_.Destroy();
    }

    static void SetUpTestSuite()
    {
        TEST_REGION_RECORD = CreateRegionInput(TEST_REGION_NAME, NO_1, {NO_1}, {true});

        TEST_LEASE_RECORD.first = {
            .fdMode = true, .name = TEST_LEASE_NAME, .size = 1 << 30, .distance = 0, .appContext = TEST_APP_CTX};
        TEST_LEASE_RECORD.second = {.memIds = {1, 2, 3, 4, 5, 6, 7, 8}, .numaId = -1, .unitSize = 1 << 27};

        TEST_SHM_IMPORT_RECORD = {.name = TEST_SHM_NAME,
                                  .size = 1 << 30,
                                  .appContext = TEST_APP_CTX,
                                  .memIds = {1, 2, 3, 4, 5, 6, 7, 8},
                                  .unitSize = 1 << 27};

        auto list = RecordStore::GetInstance().ListMemLeaseRecord();
        for (auto it : list) {
            RecordStore::GetInstance().DelMemLeaseRecord(it.first.name);
        }
        auto listRegion = RecordStore::GetInstance().ListRegionRecord();
        for (auto itRegion : listRegion) {
            RecordStore::GetInstance().DelRegionRecord(itRegion.name);
        }

        poolAllocator_.Initialize(ptr.get());
    }
};

// AddRegionRecord
TEST_F(RecordStoreTest, TestAddRegionRecord_FailWhenNameLengthOverLimit)
{
    CreateRegionInput input;
    input.name = std::string(NO_128, 'x');
    auto ret = RecordStore::GetInstance().AddRegionRecord(input);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddRegionRecord_FailWhenNodeIdOverLimit)
{
    CreateRegionInput input;
    input.name = std::string(NO_32, 'x');
    input.size = NO_1;
    input.nodeIds.push_back(NO_2048);
    input.affinity.push_back(true);
    auto ret = RecordStore::GetInstance().AddRegionRecord(input);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddRegionRecord_FailWhenNodeNumNotEqualsToAffinities)
{
    CreateRegionInput input;
    input.name = std::string(NO_32, 'x');
    input.size = NO_1;
    input.nodeIds.push_back(NO_1);
    input.affinity.push_back(true);
    input.affinity.push_back(true);
    auto ret = RecordStore::GetInstance().AddRegionRecord(input);
    EXPECT_EQ(ret, -1);
}

// DelRegionRecord
TEST_F(RecordStoreTest, TestAddRegionRecord_DelRegionRecord_Success)
{
    auto ret = RecordStore::GetInstance().AddRegionRecord(TEST_REGION_RECORD);
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().DelRegionRecord(TEST_REGION_RECORD.name);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestDelRegionRecord_FailWhenRegionNotExist)
{
    auto ret = RecordStore::GetInstance().DelRegionRecord(TEST_REGION_NAME);
    EXPECT_EQ(ret, -1);
}

// ListRegionRecord
TEST_F(RecordStoreTest, TestListRegionRecord_WhenEmpty)
{
    auto regions = RecordStore::GetInstance().ListRegionRecord();
    EXPECT_EQ(regions.size(), 0);
}

TEST_F(RecordStoreTest, TestListRegionRecord_AfterAdd_AfterDelete)
{
    auto ret = RecordStore::GetInstance().AddRegionRecord(TEST_REGION_RECORD);
    EXPECT_EQ(ret, 0);

    auto regionsBefore = RecordStore::GetInstance().ListRegionRecord();
    EXPECT_EQ(regionsBefore.size(), 1);
    ret = RecordStore::GetInstance().DelRegionRecord(TEST_REGION_RECORD.name);
    EXPECT_EQ(ret, 0);
    auto regionsAfter = RecordStore::GetInstance().ListRegionRecord();
    EXPECT_EQ(regionsAfter.size(), 0);
}

// AddMemLeaseRecord
TEST_F(RecordStoreTest, TestAddMemLeaseRecord_FailWhenNameOverLimit)
{
    MemLeaseInfo leaseInfo;
    leaseInfo.first.name = std::string(NO_64, 'x');
    auto ret = RecordStore::GetInstance().AddMemLeaseRecord(leaseInfo);
    EXPECT_EQ(ret, -1);
}

// DelMemLeaseRecord
TEST_F(RecordStoreTest, TestAddMemLeaseRecord_DelMemLeaseRecord_Success)
{
    auto ret = RecordStore::GetInstance().AddMemLeaseRecord(TEST_LEASE_RECORD);
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().DelMemLeaseRecord(TEST_LEASE_NAME);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestDelMemLeaseRecord_WhenRecordNotExist)
{
    auto ret = RecordStore::GetInstance().DelMemLeaseRecord(TEST_LEASE_NAME);
    EXPECT_EQ(ret, -1);
}

// ListMemLeaseRecord
TEST_F(RecordStoreTest, TestListMemLeaseRecord_AfterAdd_AfterDelete)
{
    auto ret = RecordStore::GetInstance().AddMemLeaseRecord(TEST_LEASE_RECORD);
    EXPECT_EQ(ret, 0);

    auto list = RecordStore::GetInstance().ListMemLeaseRecord();
    EXPECT_EQ(list.size(), NO_1);
    EXPECT_EQ(list[0].first.name, TEST_LEASE_NAME);

    ret = RecordStore::GetInstance().DelMemLeaseRecord(TEST_LEASE_NAME);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestListMemLeaseRecord_WhenEmpty)
{
    auto list = RecordStore::GetInstance().ListMemLeaseRecord();
    EXPECT_EQ(list.size(), 0);
}

// UpdateMemLeaseRecord
TEST_F(RecordStoreTest, TestUpdateMemLeaseRecord_WhenNameNotExist)
{
    auto ret = RecordStore::GetInstance().UpdateMemLeaseRecord(TEST_LEASE_NAME, TEST_APP_CTX, 0);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestUpdateMemLeaseRecord_Success)
{
    AppContext ctx{0, 0, 0};
    auto ret = RecordStore::GetInstance().AddMemLeaseRecord(TEST_LEASE_RECORD);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().UpdateMemLeaseRecord(TEST_LEASE_NAME, ctx, 0);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().DelMemLeaseRecord(TEST_LEASE_NAME);
    EXPECT_EQ(ret, 0);
}

// AddShmImportRecord
TEST_F(RecordStoreTest, TestAddShmImportRecord_FailWhenNameLengthOverLimit)
{
    ShareMemImportInfo record{};
    record.name = std::string(NO_64, 'x');
    auto ret = RecordStore::GetInstance().AddShmImportRecord(record);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddShmImportRecord_FailWhenAlreadyExist)
{
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    EXPECT_EQ(ret, -1);
    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

// DelShmImportRecord
TEST_F(RecordStoreTest, TestAddShmImportRecord_DelShmImportRecord_Success)
{
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

// ListShmImportRecord
TEST_F(RecordStoreTest, TestListShmImportRecord_AfterAdd_AfterDelete)
{
    auto records1 = RecordStore::GetInstance().ListShmImportRecord();
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    EXPECT_EQ(ret, 0);

    auto records2 = RecordStore::GetInstance().ListShmImportRecord();
    EXPECT_EQ(records2.size() - records1.size(), 1);

    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

// AddShmRefRecord
TEST_F(RecordStoreTest, TestAddShmRefRecord_FailWhenPidInvalid)
{
    pid_t pid = 0;
    auto ret = RecordStore::GetInstance().AddShmRefRecord(pid, TEST_SHM_NAME);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddShmRefRecord_FailWhenNameLengthOverLimit)
{
    auto ret = RecordStore::GetInstance().AddShmRefRecord(NO_60000, std::string(NO_64, 'x'));
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddShmRefRecord_FailWhenNameEmpty)
{
    auto ret = RecordStore::GetInstance().AddShmRefRecord(NO_60000, "");
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestAddShmRefRecord_FailWhenNameNotImport)
{
    auto ret = RecordStore::GetInstance().AddShmRefRecord(NO_60000, TEST_SHM_NAME);
    EXPECT_EQ(ret, -1);
}

// DelShmRefRecord
TEST_F(RecordStoreTest, TestAddShmRefRecord_DelShmRefRecord_Success)
{
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().AddShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                     TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().DelShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                     TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestDelShmRefRecord_FailWhenParameterInvalid)
{
    auto ret = RecordStore::GetInstance().DelShmRefRecord(0, TEST_SHM_NAME);
    EXPECT_EQ(ret, -1);
    ret = RecordStore::GetInstance().DelShmRefRecord(NO_60000, std::string(NO_64, 'x'));
    EXPECT_EQ(ret, -1);
    ret = RecordStore::GetInstance().DelShmRefRecord(NO_60000, "");
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestDelShmRefRecord_FailWhenNameNotExist)
{
    auto ret = RecordStore::GetInstance().DelShmRefRecord(NO_60000, TEST_SHM_NAME);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestDelShmImportRecord_FailWhenRefRecordLeft)
{
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    ret += RecordStore::GetInstance().AddShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                      TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);

    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, -1);

    ret = RecordStore::GetInstance().DelShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                     TEST_SHM_NAME);
    ret += RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

// DelShmRefRecordsByPid
TEST_F(RecordStoreTest, TestDelShmRefRecordsByPid_Success)
{
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    ret += RecordStore::GetInstance().AddShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                      TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().DelShmRefRecordsByPid(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid));
    EXPECT_EQ(ret, 0);
    ret = RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestDelShmRefRecordsByPid_FailWhenPidIsZero)
{
    auto ret = RecordStore::GetInstance().DelShmRefRecordsByPid(0);
    EXPECT_EQ(ret, -1);
}

TEST_F(RecordStoreTest, TestDelShmRefRecordsByPid_SuccessWhenPidNoRef)
{
    auto ret = RecordStore::GetInstance().DelShmRefRecordsByPid(NO_60000);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestListShmRefRecordNameF_AfterAddRef)
{
    auto records1 = RecordStore::GetInstance().ListShmRefRecordNameF();
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    ret += RecordStore::GetInstance().AddShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                      TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);
    auto records2 = RecordStore::GetInstance().ListShmRefRecordNameF();
    EXPECT_EQ(records2.size() - records1.size(), 1);
    auto pos = records2[TEST_SHM_NAME].find(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid));
    EXPECT_NE(pos, records2[TEST_SHM_NAME].end());

    ret = RecordStore::GetInstance().DelShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                     TEST_SHM_NAME);
    ret += RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestListShmRefRecordPidF_AfterAddRef)
{
    auto records1 = RecordStore::GetInstance().ListShmRefRecordNameF();
    auto ret = RecordStore::GetInstance().AddShmImportRecord(TEST_SHM_IMPORT_RECORD);
    ret += RecordStore::GetInstance().AddShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                      TEST_SHM_NAME);
    EXPECT_EQ(ret, 0);
    auto records2 = RecordStore::GetInstance().ListShmRefRecordPidF();
    EXPECT_EQ(records2.size() - records1.size(), 1);
    auto pos = records2[static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid)].find(TEST_SHM_NAME);
    EXPECT_NE(pos, records2[static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid)].end());

    ret = RecordStore::GetInstance().DelShmRefRecord(static_cast<pid_t>(TEST_SHM_IMPORT_RECORD.appContext.pid),
                                                     TEST_SHM_NAME);
    ret += RecordStore::GetInstance().DelShmImportRecord(TEST_SHM_IMPORT_RECORD.name);
    EXPECT_EQ(ret, 0);
}

// RecordIdPoolAllocator
TEST_F(RecordStoreTest, TestFillAllocated_Success)
{
    uint32_t headIdx;
    auto ret = poolAllocator_.Allocate(TEST_LEASE_RECORD.second.memIds, headIdx);
    EXPECT_EQ(ret, 0);
    ret = poolAllocator_.FillAllocated(headIdx, TEST_LEASE_RECORD.second.memIds);
    EXPECT_EQ(ret, 0);
    ret = poolAllocator_.Release(headIdx);
    EXPECT_EQ(ret, 0);
}

TEST_F(RecordStoreTest, TestFillAllocated_FailWhenHeadIndexInvalid)
{
    auto ret = poolAllocator_.FillAllocated(RECORD_MEM_ID_POOL_LINE_COUNT, TEST_LEASE_RECORD.second.memIds);
    EXPECT_EQ(ret, -1);
    ret = poolAllocator_.FillAllocated(7777U, TEST_LEASE_RECORD.second.memIds);
    EXPECT_EQ(ret, -1);
}
} // namespace UT