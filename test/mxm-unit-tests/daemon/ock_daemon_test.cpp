/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <iostream>
#include <mockcpp/mockcpp.hpp>
#include "daemon_test_common.h"
#include "ubs_certify_handler.h"

#define private public
#include "ock_daemon.h"
#undef private

using namespace UT::Daemon;
using namespace ock::ubsm;
#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))
namespace UT {
using namespace ock::daemon;
std::string testPath = "/tmp/test_bin_path_XXXXXX";
std::string validBinPathHeader = "-binpath=";

std::string testConf = R"(# the log level of ubsm server, (DEBUG, INFO, WARN, ERROR, CRITICAL)
ubsm.server.log.level = INFO
# the log file path, must be canonical path
ubsm.server.log.path = /var/log/ubsm
# log file count, min is 1, max is 50
    ubsm.server.log.rotation.file.count = 10
# log file size(MB), min is 2, max is 100
ubsm.server.log.rotation.file.size = 20

# enable or disable audit log, (on, off)
ubsm.server.audit.enable = off
# audit log, the configuration item value range is the same as 'ubsm.server.log.*'
ubsm.server.audit.log.path = /var/log/ubsm
    ubsm.server.audit.log.rotation.file.count = 10
ubsm.server.audit.log.rotation.file.size = 20

# CC lock
ubsm.lock.enable = off
ubsm.lock.dev.name = bonding_dev_0
ubsm.lock.dev.eid = 0

# Zen discovery
# election timeout, min is 0, max is 2000
ubsm.discovery.election.timeout = 1000
# min nodes, min is 0, max is 30
ubsm.discovery.min.nodes = 0

ubsm.server.rpc.local.ipseg = 127.0.0.1:7201
ubsm.server.rpc.remote.ipseg = 127.0.0.1:7301
# tls options
ubsm.server.tls.enable = off
ubsm.server.tls.ciphersuits = aes_gcm_128
ubsm.server.tls.ca.path = /path/cacert.pem
    ubsm.server.tls.crl.path = /path/crl.pem
    ubsm.server.tls.cert.path = /path/cert.pem
    ubsm.server.tls.key.path = /path/key.pem
    ubsm.server.tls.keypass.path = /path/keypass.txt

# max is 256, default 128
    ubsm.hcom.max.connect.num = 128
# enable or disable memory lease cache, (on, off)
ubsm.server.lease.cache.enable = off
ubsm.performance.statistics.enable = off)";

class OckDaemonTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        mkdir(testPath.c_str(), 0755);
        std::string confDir = testPath + "/config";
        mkdir(confDir.c_str(), 0755);
        std::string fileName = confDir + "/ubsmd.conf";
        int fd = open(fileName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        auto written = write(fd, testConf.c_str(), testConf.size());
        if (written == -1) {
            std::cerr << "write failed" << strerror(errno) << std::endl;
        }
        close(fd);
    }

    static void TearDownTestCase()
    {
        std::string cmd = "rm -rf " + testPath;
        auto res = system(cmd.c_str());
        ASSERT_EQ(0, res);
    }

protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(OckDaemonTest, TestCheckParamInvalidHeader)
{
    std::string invalidPath = "invalid=" + testPath;
    ock::daemon::OckDaemon ockDaemon;
    auto result = ockDaemon.CheckParam(invalidPath);

    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestCheckParamPathTooLong)
{
    std::string binPath = validBinPathHeader + std::string(PATH_MAX * 2, 'a');
    ock::daemon::OckDaemon ockDaemon;
    auto result = ockDaemon.CheckParam(binPath);

    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestCheckParamNonExist)
{
    std::string nonExistentPath = validBinPathHeader + "/non/existent/path";
    ock::daemon::OckDaemon ockDaemon;
    auto result = ockDaemon.CheckParam(nonExistentPath);

    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestCheckParamValidPath)
{
    std::string validPath = validBinPathHeader + testPath;
    ock::daemon::OckDaemon ockDaemon;
    auto result = ockDaemon.CheckParam(validPath);
    EXPECT_EQ(HOK, result);
}

void EmptyStub()
{
    printf("EmptyStub.\n");
}

TEST_F(OckDaemonTest, TestInitializeMHomePathEmpty)
{
    ock::daemon::OckDaemon ockDaemon;
    auto result = ockDaemon.Initialize();
    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestInitializeMHomePathValid)
{
    std::string validPath = validBinPathHeader + testPath;
    ock::daemon::OckDaemon ockDaemon;
    ockDaemon.CheckParam(validPath);
    auto result = ockDaemon.Initialize();
}

TEST_F(OckDaemonTest, TestInitializeUbsmLock)
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
    hr = daemon->InitHandler();
    EXPECT_EQ(hr, HOK);
    hr = daemon->InitUbsmLock();
    ASSERT_EQ(HOK, hr);
    DaemonTestCommon::DeleteConf();
}

TEST_F(OckDaemonTest, TestInitializeUbsmTlsShouldFailed)
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
                                 "ubsm.lock.tls.enable = on\n"
                                 "ubsm.lock.ub_token.enable = on\n"
                                 "ubsm.lock.expire.time = 300\n"
                                 "ubsm.server.lease.cache.enable = on\n"
                                 "ubsm.hcom.max.connect.num = 128\n"
                                 "ubsm.discovery.election.timeout = 1000\n"
                                 "ubsm.discovery.min.nodes = 0\n"
                                 "ubsm.server.tls.enable = on\n"
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
    hr = daemon->InitHandler();
    EXPECT_EQ(hr, HOK);
    hr = daemon->InitRpcTlsConfig();
    ASSERT_EQ(HFAIL, hr);
    hr = daemon->InitLockTlsConfig();
    ASSERT_EQ(HFAIL, hr);
    DaemonTestCommon::DeleteConf();
}

TEST_F(OckDaemonTest, TestockDaemonStartShouldFailed)
{
    ock::daemon::OckDaemon ockDaemon;
    auto start = std::chrono::steady_clock::now();
    auto result = ockDaemon.Start(start);
    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestockDaemonStartShouldSuccess)
{
    std::string validPath = validBinPathHeader + testPath;
    ock::daemon::OckDaemon ockDaemon;
    ockDaemon.CheckParam(validPath);
    ockDaemon.Initialize();
    auto start = std::chrono::steady_clock::now();
    auto result = ockDaemon.Start(start);
    ASSERT_EQ(HFAIL, result);
}

TEST_F(OckDaemonTest, TestockDaemonGenerateShmShouldSuccess)
{
    ock::daemon::OckDaemon ockDaemon;
    auto hr = ockDaemon.GenerateShareMemory("ubsm_records");
    EXPECT_TRUE(hr);
}

TEST_F(OckDaemonTest, TestockDaemonStopShouldSuccess)
{
    ock::daemon::OckDaemon ockDaemon;
    ockDaemon.TryStop();
}

TEST_F(OckDaemonTest, TestockDaemonShutDownShouldFailed)
{
    ock::daemon::OckDaemon ockDaemon;
    auto hr = ockDaemon.Shutdown();
    EXPECT_EQ(HFAIL, hr);
}

TEST_F(OckDaemonTest, TestockDaemonUninitializeShouldFailed)
{
    ock::daemon::OckDaemon ockDaemon;
    auto hr = ockDaemon.Uninitialize();
    EXPECT_EQ(HFAIL, hr);
}

TEST_F(OckDaemonTest, TestockDaemonUninitializeShouldFailed1)
{
    ock::daemon::OckDaemon ockDaemon;
    auto hr = ockDaemon.Uninitialize();
    EXPECT_EQ(HFAIL, hr);
}

TEST_F(OckDaemonTest, TestockDaemonStoppingKeepAlive)
{
    ock::daemon::OckDaemon ockDaemon;
    ockDaemon.StoppingKeepAlive();
}

TEST_F(OckDaemonTest, TestockDaemonHandleSignal)
{
    ock::daemon::OckDaemon ockDaemon;
    int sigNum = 6;
    ock::daemon::OckDaemon::HandleSignal(sigNum);
}

TEST_F(OckDaemonTest, TestockDaemonHandleSigpipe)
{
    ock::daemon::OckDaemon ockDaemon;
    int sigNum = 6;
    ock::daemon::OckDaemon::HandleSigpipe(sigNum);
}

TEST_F(OckDaemonTest, TestockDaemonPrintStartTime)
{
    ock::daemon::OckDaemon ockDaemon;
    auto start = std::chrono::steady_clock::now();
    ock::daemon::OckDaemon::PrintStartTime(start, "START");
}

} // namespace UT
