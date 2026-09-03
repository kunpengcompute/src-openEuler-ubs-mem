// Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

//
// Created by g00693968 on 2023/11/10.
//
#include <sanitizer/asan_interface.h>
#include "daemon_test_common.h"

using namespace ock::common;
using namespace UT::Daemon;

namespace UT {
TEST(daemon_common, configuration)
{
    Configuration::DestroyInstance();
    DaemonTestCommon::CreateConf("ubsm.server.log.path = " + DaemonTestCommon::CWD() +
                                 "/log\n"
                                 "ubsm.server.log.level = DEBUG\n"
                                 "ubsm.server.log.rotation.file.size = 200\n"
                                 "ubsm.server.log.rotation.file.count = 100\n"
                                 "ubsm.server.audit.enable = on\n"
                                 "ubsm.server.audit.log.path = " +
                                 DaemonTestCommon::CWD() +
                                 "/log/../log/\n"
                                 "ubsm.server.audit.log.rotation.file.count = 100\n"
                                 "ubsm.server.audit.log.rotation.file.size = 400\n"
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
    char confPath[2048];
    auto ret = sprintf_s(confPath, sizeof(confPath), "%s/config/ubsmd.conf\0", DaemonTestCommon::CWD().c_str());
    EXPECT_GE(ret, 0);
    auto conf = Configuration::GetInstance(confPath);
    EXPECT_NE(conf, nullptr);
    EXPECT_EQ(conf->Initialized(), true);
    DaemonTestCommon::DeleteConf();

    int v1 = conf->GetInt("ubsm.server.log.rotation.file.size");
    EXPECT_EQ(v1, 200);
    std::string v2 = conf->GetString("ubsm.server.log.level");
    EXPECT_EQ(v2, std::string("DEBUG"));

    conf->Set("ubsm.server.log.rotation.file.count", 50);
    int v4 = conf->GetInt("ubsm.server.log.rotation.file.count");
    EXPECT_EQ(v4, 50);

    std::string v6 = conf->GetConvertedValue("ubsm.server.audit.log.rotation.file.size");
    EXPECT_EQ(v6, std::string("400"));

    std::string v7 = conf->GetConvertedValue("ubsm.server.audit.log.path");
    EXPECT_EQ(v7, std::string(DaemonTestCommon::CWD() + "/log"));

    std::string v8 = conf->GetConvertedValue("mxmd.features.log.power");
    EXPECT_EQ(v8, std::string(""));
    conf->Set("ubsm.server.log.path", DaemonTestCommon::CWD() + "/log\n");
    conf->Set("ubsm.server.log.path", false);
    conf->Set("ubsm.server.log.path", (int)100);
    conf->Set("ubsm.server.log.path", (float)0.0);
    conf->Set("ubsm.server.log.path", (long)100);
    conf->AddConverter("ubsm.server.log.path", nullptr);
    conf->Validate(true, true, true, true);
}

TEST(daemon_common, FuncSplitStrSet)
{
    std::set<std::string> out_set;

    SplitStr("aaa|bb", "|", out_set);
    EXPECT_EQ(out_set.size(), (std::size_t)2U);

    SplitStr("aaa|aaa", "|", out_set);
    EXPECT_EQ(out_set.size(), (std::size_t)2U);

    out_set.clear();
    SplitStr("aaa|", "|", out_set);
    EXPECT_EQ(out_set.size(), (std::size_t)1);

    out_set.clear();
    SplitStr("aaa", "|", out_set);
    EXPECT_EQ(out_set.size(), (std::size_t)1);
}

TEST(daemon_common, FuncSplitStrList)
{
    std::list<std::string> out_list;

    SplitStr("aaa|bb", "|", out_list);
    EXPECT_EQ(out_list.size(), 2);

    SplitStr("aaa|aaa", "|", out_list);
    EXPECT_EQ(out_list.size(), 4);

    out_list.clear();
    SplitStr("aaa|", "|", out_list);
    EXPECT_EQ(out_list.size(), 1);

    out_list.clear();
    SplitStr("aaa", "|", out_list);
    EXPECT_EQ(out_list.size(), 1);
}

TEST(daemon_common, FuncStol)
{
    long value = 0;
    bool ret = OckStol("12345", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, 12345);
}

TEST(daemon_common, FuncStof)
{
    float value = 0;
    bool ret = OckStof("12345.1", value);
    EXPECT_EQ(ret, true);
}

TEST(daemon_common, FuncIsBool)
{
    bool value = false;
    bool ret = IsBool("true", value);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(value, true);
}

TEST(daemon_common, OckStol)
{
    long int value = 0;
    std::string s1 = "12365";
    std::string s2 = std::to_string(LONG_MAX);
    std::string s3 = std::to_string(LONG_MIN);
    std::string s4 = "INT32_MAX";
    std::string s5 = "-1";
    std::string s6 = "ab123";
    std::string s7 = "56.3";
    EXPECT_EQ(OckStol(s1, value), true);
    EXPECT_EQ(value, 12365);
    EXPECT_EQ(OckStol(s2, value), true);
    EXPECT_EQ(value, LONG_MAX);
    EXPECT_EQ(OckStol(s3, value), true);
    EXPECT_EQ(value, LONG_MIN);
    EXPECT_EQ(OckStol(s4, value), false);
    EXPECT_EQ(OckStol(s5, value), true);
    EXPECT_EQ(value, -1);
    EXPECT_EQ(OckStol(s6, value), false);
    EXPECT_EQ(OckStol(s7, value), false);
}

TEST(daemon_common, KVParserTest)
{
    ock::common::KVParser parser;
    std::string outValue;
    auto ret = parser.GetItem("abc", outValue);
    EXPECT_EQ(HFAIL, ret);
    parser.Dump();
}
} // namespace UT