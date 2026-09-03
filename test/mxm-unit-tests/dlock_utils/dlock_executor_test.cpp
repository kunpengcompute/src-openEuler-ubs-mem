/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include "dlock_executor_test.h"
#include <dlfcn.h>
#include <mockcpp/mokc.h>
#include <mockcpp/mockcpp.hpp>
#include "dlock_utils/ubsm_lock.h"
#include "system_adapter.h"

using namespace ock::dlock_utils;
using namespace UT;
using namespace ock::ubsm;
int MockDLockSuccess()
{
    return dlock::DLOCK_SUCCESS;
}
int MockDLockFail()
{
    return -1;
}

void MockDlsym(const char *name)
{
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlSym)
        .stubs()
        .with(any(), checkWith([name](const char *str) mutable -> bool { return strcmp(str, name) == 0; }))
        .will(returnValue(static_cast<void *>(nullptr)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
}
TEST_F(DlockExecutorTestSuite, TestInitDLockServerDlopenLibSuccess)
{
    // given
    int mockHandle = 0;
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockServerDlopenLib();

    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockServerDlopenLibFail1)
{
    // given
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("dserver_lib_init");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockServerDlopenLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockServerDlopenLibFail2)
{
    // given
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("dserver_lib_deinit");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockServerDlopenLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockServerDlopenLibFail3)
{
    // given
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("server_start");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockServerDlopenLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockServerDlopenLibFail4)
{
    // given
    int mockHandle = 0;
    MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(static_cast<void *>(&mockHandle)));
    MockDlsym("server_stop");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockServerDlopenLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestDLockClientLockInitLibSuccess)
{
    // given
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail)
{
    // given
    MockDlsym("dclient_lib_init");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail1)
{
    // given
    MockDlsym("dclient_lib_deinit");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail2)
{
    // given
    MockDlsym("client_init");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail3)
{
    // given
    MockDlsym("client_reinit");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail4)
{
    // given
    MockDlsym("client_deinit");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail5)
{
    // given
    MockDlsym("client_reinit_done");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();

    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockInitLibFail6)
{
    // given
    MockDlsym("client_heartbeat");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockInitLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibSuccess)
{
    // given
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail)
{
    // given
    MockDlsym("get_lock");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail1)
{
    // given
    MockDlsym("release_lock");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail2)
{
    // given
    MockDlsym("trylock");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail3)
{
    // given
    MockDlsym("lock");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail4)
{
    // given
    MockDlsym("unlock");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockGetLibFail5)
{
    // given
    MockDlsym("lock_extend");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockGetLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockMgmtLibSuccess)
{
    // given
    void *mockFunc = reinterpret_cast<void *>(MockDLockSuccess);
    MOCKER(SystemAdapter::DlSym).stubs().with(any(), any()).will(returnValue(mockFunc));
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockMgmtLib();
    // then
    ASSERT_EQ(ret, UBSM_OK);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockMgmtLibFail)
{
    // given
    MockDlsym("update_all_locks");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockMgmtLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockMgmtLibFail1)
{
    // given
    MockDlsym("lock_request_async");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockMgmtLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockMgmtLibFail2)
{
    // given
    MockDlsym("lock_result_check");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockMgmtLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}

TEST_F(DlockExecutorTestSuite, TestInitDLockClientLockMgmtLibFail3)
{
    // given
    MockDlsym("get_client_debug_stats");
    // when
    auto ret = DLockExecutor::GetInstance().InitDLockClientLockMgmtLib();
    // then
    ASSERT_EQ(ret, MXM_ERR_DLOCK_LIB);
}