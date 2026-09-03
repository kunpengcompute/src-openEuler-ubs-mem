/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Author: bao
 */

#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>
#include <chrono>
#include <mockcpp/mockcpp.hpp>
#include "util/systemd_wrapper.h"

#include <mx_shm.h>

namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class UtilSystemdTest : public testing::Test {
public:
    UtilSystemdTest();
    virtual void SetUp(void);
    virtual void TearDown(void);

protected:
};

UtilSystemdTest::UtilSystemdTest() {}

void UtilSystemdTest::SetUp()
{
    GlobalMockObject::reset();
}

void UtilSystemdTest::TearDown()
{
    GlobalMockObject::reset();
}

TEST_F(UtilSystemdTest, UtilSystemdTest)
{
    int ret = ock::common::systemd::NotifyReady();
    EXPECT_EQ(0, ret);
    ret = ock::common::systemd::NotifyStopping();
    EXPECT_EQ(0, ret);
    ret = ock::common::systemd::NotifyWatchdog();
    EXPECT_EQ(0, ret);
    std::chrono::microseconds us(1);
    bool isOk = ock::common::systemd::GetWatchdogTimeout(us);
    EXPECT_EQ(false, isOk);
}

TEST_F(UtilSystemdTest, TestSystemdStoreFd)
{
    MOCKER_CPP(&sd_pid_notify_with_fds,
               int (*)(pid_t pid, int unset_environment, const char *state, const int *fds, unsigned n_fds))
        .stubs()
        .will(returnValue(0));
    std::string name = "name";
    auto ret = ock::common::systemd::StoreFd(name, 1);
    EXPECT_EQ(ret, 0);
}
TEST_F(UtilSystemdTest, TestSystemdStoreFdFail)
{
    MOCKER_CPP(&sd_pid_notify_with_fds,
               int (*)(pid_t pid, int unset_environment, const char *state, const int *fds, unsigned n_fds))
        .stubs()
        .will(returnValue(-1));
    std::string name = "name";
    auto ret = ock::common::systemd::StoreFd(name, 1);
    EXPECT_EQ(ret, -1);
}

TEST_F(UtilSystemdTest, TestLoadFdFail)
{
    MOCKER_CPP(&sd_listen_fds_with_names, int (*)(int unset_environment, char ***names)).stubs().will(returnValue(-1));

    int fd = 1;
    auto ret = ock::common::systemd::LoadFd("name", fd);
    EXPECT_EQ(ret, -1);
}

TEST_F(UtilSystemdTest, TestLoadFdFail01)
{
    MOCKER_CPP(&sd_listen_fds_with_names, int (*)(int unset_environment, char ***names)).stubs().will(returnValue(0));
    int fd = 1;
    auto ret = ock::common::systemd::LoadFd("name", fd);
    EXPECT_EQ(ret, -1);
}
} // namespace UT