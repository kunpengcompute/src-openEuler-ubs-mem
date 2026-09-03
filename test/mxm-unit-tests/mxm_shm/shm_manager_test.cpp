#include <fcntl.h>
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include "shm_manager_test.h"

#include <record_store.h>
#include <shm_manager.h>
#include "rack_mem_err.h"

using namespace UT;
using namespace ock::share::service;
using namespace ock::common;

TEST_F(SHMManagerTestSuite, TestGetMemoryUsersByPid)
{
    ock::share::service::SHMManager::GetInstance().PrepareAddShareMemoryInfo("shm1", 134217728, 1002);
    SHMManager::GetInstance().AddFullShareMemoryInfo("shm1", 1002,
                                                     {.memIds = {1},
                                                      .unitSize = 134217728,
                                                      .createFlags = 0,
                                                      .openFlag = O_RDWR,
                                                      .ownerUserId = 0,
                                                      .ownerGroupId = 0});
    auto res = SHMManager::GetInstance().GetMemoryUsersByPid(1002);
    SHMManager::GetInstance().RemoveMemoryUserInfo("name", 1002);
    SHMManager::GetInstance().RemoveMemoryInfo("shm1");
    ASSERT_EQ(res.empty(), false);
}

TEST_F(SHMManagerTestSuite, TestGetAllUsers)
{
    ock::share::service::SHMManager::GetInstance().PrepareAddShareMemoryInfo("shm1", 134217728, 1002);
    SHMManager::GetInstance().AddFullShareMemoryInfo("shm1", 1002,
                                                     {.memIds = {1},
                                                      .unitSize = 134217728,
                                                      .createFlags = 0,
                                                      .openFlag = O_RDWR,
                                                      .ownerUserId = 0,
                                                      .ownerGroupId = 0});
    auto map = SHMManager::GetInstance().GetAllUsers();
    SHMManager::GetInstance().RemoveMemoryUserInfo("name", 1002);
    SHMManager::GetInstance().RemoveMemoryInfo("shm1");
    ASSERT_EQ(map.empty(), false);
}

TEST_F(SHMManagerTestSuite, TestGetShareMemoryInfo)
{
    ock::share::service::SHMManager::GetInstance().PrepareAddShareMemoryInfo("name", 134217728, 1002);
    SHMManager::GetInstance().AddFullShareMemoryInfo("name", 1002,
                                                     {.memIds = {1},
                                                      .unitSize = 134217728,
                                                      .createFlags = 0,
                                                      .openFlag = O_RDWR,
                                                      .ownerUserId = 0,
                                                      .ownerGroupId = 0});
    AttachedShareMemory info;
    auto ret = SHMManager::GetInstance().GetShareMemoryInfo("name", info);
    SHMManager::GetInstance().RemoveMemoryUserInfo("name", 1002);
    SHMManager::GetInstance().RemoveMemoryInfo("name");
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(info.name, "name");
}

TEST_F(SHMManagerTestSuite, TestGetShareMemoryInfoFail)
{
    AttachedShareMemory info;
    auto ret = SHMManager::GetInstance().GetShareMemoryInfo("notFound", info);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryUserInfoFail)
{
    MOCKER_CPP(&ock::ubsm::RecordStore::DelShmRefRecord, int (*)(pid_t pid, const std::string &name))
        .stubs()
        .will(returnValue(-1));
    auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo("name", 1002);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryUserInfoFail01)
{
    MOCKER_CPP(&ock::ubsm::RecordStore::DelShmRefRecord, int (*)(pid_t pid, const std::string &name))
        .stubs()
        .will(returnValue(0));
    auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo("name", 0);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryUserInfoFail02)
{
    MOCKER_CPP(&ock::ubsm::RecordStore::DelShmRefRecord, int (*)(pid_t pid, const std::string &name))
        .stubs()
        .will(returnValue(0));
    auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo("nameErr", 0);
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryInfoFail)
{
    auto ret = SHMManager::GetInstance().RemoveMemoryInfo("name");
    ASSERT_NE(ret, 0);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryUserInfo)
{
    ock::share::service::SHMManager::GetInstance().PrepareAddShareMemoryInfo("name", 134217728, 1002);
    SHMManager::GetInstance().AddFullShareMemoryInfo("name", 1002,
                                                     {.memIds = {1},
                                                      .unitSize = 134217728,
                                                      .createFlags = 0,
                                                      .openFlag = O_RDWR,
                                                      .ownerUserId = 0,
                                                      .ownerGroupId = 0});
    auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo("name", 1002);
    SHMManager::GetInstance().RemoveMemoryInfo("name");
    ASSERT_EQ(ret, 0);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryInfo)
{
    ock::share::service::SHMManager::GetInstance().PrepareAddShareMemoryInfo("name", 134217728, 1002);
    auto ret = SHMManager::GetInstance().RemoveMemoryInfo("name");
    ASSERT_EQ(ret, 0);
}

TEST_F(SHMManagerTestSuite, TestRemoveMemoryInfoFail01)
{
    MOCKER_CPP(&ock::ubsm::RecordStore::DelShmImportRecord, int (*)(const std::string &name))
        .stubs()
        .will(returnValue(static_cast<int>(MXM_ERR_SHM_NOT_FOUND)));
    auto ret = SHMManager::GetInstance().RemoveMemoryInfo("name");
    ASSERT_EQ(ret, MXM_ERR_SHM_NOT_FOUND);
}