/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
#include <sanitizer/asan_interface.h>
#include "daemon_test_common.h"
#include "ock_daemon.h"

using namespace ock::daemon;
using namespace ock::common;
using namespace UT::Daemon;

namespace UT {
const HRESULT HRESULT_NO_FEATURE_ENABLED = static_cast<HRESULT>(MXM_ERR_DAEMON_NO_FEATURE_ENABLED);

TEST(daemon_init, ulog_init)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.lock.enable = on\n"
                                 "ubsm.lock.tls.enable = off\n"
                                 "ubsm.lock.ub_token.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto result = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(result, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HRESULT_NO_FEATURE_ENABLED);
    DaemonTestCommon::DeleteConf();

    int ret = 0; // was ULOG_AuditLogMessage
    EXPECT_EQ(ret, 0);
    DaemonTestCommon::RmLogFile();
}

TEST(daemon_init, ulog_init_check_params_2)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s/not_exist\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HFAIL);

    ret = sprintf_s(binPath, sizeof(binPath), "-path=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

TEST(daemon_init, ulog_level_convert)
{
    int32_t levelInt(0);
    levelInt = LogAdapter::StringToLogLevel("DEBUG");
    EXPECT_EQ(levelInt, DBG_LOG_DEBUG);
    levelInt = LogAdapter::StringToLogLevel("WARN");
    EXPECT_EQ(levelInt, DBG_LOG_WARN);
    levelInt = LogAdapter::StringToLogLevel("INFO");
    EXPECT_EQ(levelInt, DBG_LOG_INFO);
    levelInt = LogAdapter::StringToLogLevel("ERROR");
    EXPECT_EQ(levelInt, DBG_LOG_ERROR);
    levelInt = LogAdapter::StringToLogLevel("CRITICAL");
    EXPECT_EQ(levelInt, DBG_LOG_CRITICAL);
    levelInt = LogAdapter::StringToLogLevel("INVALID");
    EXPECT_EQ(levelInt, DBG_LOG_INFO);
}

// 错误的 ubsm.server.audit.enable 配置
TEST(daemon_init, ulog_audit_conf_1)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = invalid\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto result = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(result, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);
    DaemonTestCommon::DeleteConf();

    int ret = 0; // was ULOG_AuditLogMessage
    EXPECT_EQ(ret, 0);
    DaemonTestCommon::RmLogFile();
}

TEST(daemon_init, init_number_conf_1)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 2.a\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);
    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

// 缺省的 ubsm.server.audit.enable 配置
TEST(daemon_init, ulog_audit_conf_2)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto result = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(result, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);
    DaemonTestCommon::DeleteConf();

    int ret = 0; // was ULOG_AuditLogMessage
    EXPECT_EQ(ret, 0);
    DaemonTestCommon::RmLogFile();
}

// 不开启审计日志
TEST(daemon_init, ulog_audit_conf_3)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = off\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.lock.enable = on\n"
                                 "ubsm.lock.tls.enable = off\n"
                                 "ubsm.lock.ub_token.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto result = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(result, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HRESULT_NO_FEATURE_ENABLED);
    DaemonTestCommon::DeleteConf();

    int ret = 0; // was ULOG_AuditLogMessage
    EXPECT_EQ(ret, 0);
    DaemonTestCommon::RmLogFile();
}

// 初始化服务缺少动态库
TEST(daemon_init, init_service_without_lib)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.lock.enable = on\n"
                                 "ubsm.lock.tls.enable = off\n"
                                 "ubsm.lock.ub_token.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);
    hr = daemon->Initialize();
    EXPECT_EQ(hr, HRESULT_NO_FEATURE_ENABLED);
    DaemonTestCommon::DeleteConf();

    OckService *service = new OckService();
    auto adapter = new OckServiceAdapter("mls", "NULL");
    hr = service->Initialize(adapter);
    EXPECT_EQ(hr, HFAIL); // 缺少动态库会使特定服务初始化失败
    delete service;
    delete adapter;
    service = nullptr;
    adapter = nullptr;
    DaemonTestCommon::RmLogFile();
}

TEST(daemon_init, append_args)
{
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);

    int newArgc = 0;
    char **newArgv = nullptr;
    HRESULT hr = ock::common::CreateArgs(newArgc, newArgv, {"\0", binPath, "1", "off"});
    EXPECT_EQ(hr, HOK);
    EXPECT_EQ(newArgc, 4);
    EXPECT_STREQ(newArgv[0], "");
    EXPECT_STREQ(newArgv[1], binPath);
    EXPECT_STREQ(newArgv[2], "1");
    EXPECT_STREQ(newArgv[3], "off");

    hr = ock::common::CreateArgs(newArgc, newArgv, {"\0", binPath, "2", "on"});
    EXPECT_EQ(hr, HFAIL); // newArgv不为空

    ock::common::DeleteStrArray(newArgc, newArgv);
    EXPECT_EQ(newArgv, nullptr);

    hr = ock::common::CreateArgs(newArgc, newArgv, {"\0", binPath, "3", "off"});
    EXPECT_EQ(hr, HOK);
    delete[] newArgv[2];
    newArgv[2] = nullptr;
    ock::common::DeleteStrArray(newArgc, newArgv);
    EXPECT_EQ(newArgv, nullptr);
}

// path为空
TEST(daemon_init, conf_1)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = \n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);

    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

// ENUM为空
TEST(daemon_init, conf_2)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = \n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);

    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);

    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

// int为空
TEST(daemon_init, conf_3)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = \n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);

    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

// int超出范围
TEST(daemon_init, conf_5)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = 20\n"
                                 "ubsm.server.log.rotation.file.count = 0\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);

    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

// int错误
TEST(daemon_init, conf_6)
{
    DaemonTestCommon::ClearUlog();
    Configuration::DestroyInstance();
    ock::daemon::OCKDaemonPtr daemon = new ock::daemon::OckDaemon();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = INFO\n"
                                 "ubsm.server.log.rotation.file.size = invalid\n"
                                 "ubsm.server.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.audit.log.rotation.file.count = 10\n"
                                 "ubsm.server.audit.log.rotation.file.size = 20\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.server.tls.enable = off\n"
                                 "ubsm.server.tls.ciphersuits = aes_gcm_128\n"
                                 "ubsm.performance.statistics.enable = off\n");
    char *av[2];
    char empty = '\0';
    char binPath[MAX_SIZE];
    auto ret = sprintf_s(binPath, sizeof(binPath), "-binpath=%s\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    av[0] = &empty;
    av[1] = binPath;

    auto hr = daemon->CheckParam(av[1]);
    EXPECT_EQ(hr, HOK);

    hr = daemon->Initialize();
    EXPECT_EQ(hr, HFAIL);

    DaemonTestCommon::DeleteConf();
    DaemonTestCommon::RmLogFile();
}

TEST(daemon_init, ulog_recover)
{
    auto ret = DaemonTestCommon::RecoverUlog();
    EXPECT_EQ(ret, 0);
}

} // namespace UT
