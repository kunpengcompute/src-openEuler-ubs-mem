/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
#include <gtest/gtest.h>
#include <mockcpp/mokc.h>
#include <systemd/sd-daemon.h>
#include <mockcpp/mockcpp.hpp>
#include "configuration.h"
#include "fstream"
#include "kv_parser.h"
#include "systemd_wrapper.h"
#include "ubsm_file_util.h"
#include "util/ref.h"

using namespace ock::utils;

namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class UtilFileTest : public testing::Test {
public:
    UtilFileTest();
    virtual void SetUp(void);
    virtual void TearDown(void);

protected:
};

UtilFileTest::UtilFileTest() {}

void UtilFileTest::SetUp() {}

void UtilFileTest::TearDown() {}

TEST_F(UtilFileTest, TestFiles)
{
    bool result = true;

    std::string testPath = "/tmp/testFileUtil";
    std::string testPath2 = "/tmp/testFileUtil2";
    uint32_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    result = FileUtil::MakeDirRecursive(testPath, mode);
    EXPECT_EQ(result, true);

    result = FileUtil::MakeDir(testPath, mode);
    EXPECT_EQ(result, true);

    result = FileUtil::Exist(testPath);
    EXPECT_EQ(result, true);

    result = FileUtil::Exist(testPath2);
    EXPECT_EQ(result, false);

    result = FileUtil::Readable(testPath);
    EXPECT_EQ(result, true);
    result = FileUtil::Writable(testPath);
    EXPECT_EQ(result, true);
    result = FileUtil::ReadAndWritable(testPath);
    EXPECT_EQ(result, true);

    result = FileUtil::Readable(testPath2);
    EXPECT_EQ(result, false);
    result = FileUtil::Writable(testPath2);
    EXPECT_EQ(result, false);
    result = FileUtil::ReadAndWritable(testPath2);
    EXPECT_EQ(result, false);

    auto testTouch = "touch " + testPath + "/a.txt";
    auto testRemove = "rm -f  " + testPath + "/a.txt";
    system(testTouch.c_str());
    std::ifstream in(testPath + "/a.txt");
    result = FileUtil::CheckFileSize(in, 100U);
    in.close();
    EXPECT_EQ(result, false);
    system(testRemove.c_str());

    result = FileUtil::Remove(testPath);
    EXPECT_EQ(result, true);
}

TEST_F(UtilFileTest, TestConfigurationFail)
{
    MOCKER_CPP(&ock::common::Configuration::Initialized, bool (*)()).stubs().will(returnValue(false));

    std::string filePath = "/path";
    auto ConfigPtr = ock::common::Configuration::FromFile(filePath);
    EXPECT_EQ(ConfigPtr, nullptr);
}

TEST_F(UtilFileTest, TestConfigurationFail01)
{
    MOCKER_CPP(&ock::common::KVParser::FromFile, HRESULT(*)(const std::string &filePath)).stubs().will(returnValue(0));
    MOCKER_CPP(&ock::common::KVParser::Size, uint32_t(*)()).stubs().will(returnValue(101));
    std::string filePath = "/path";
    auto ConfigPtr = ock::common::Configuration::FromFile(filePath);
    EXPECT_EQ(ConfigPtr, nullptr);
}

TEST_F(UtilFileTest, TestValidateFilePath)
{
    std::vector<std::pair<std::string, bool>> filePaths;
    filePaths.emplace_back(std::make_pair("key1", true));
    ock::common::Configuration cfg;
    auto err = cfg.ValidateFilePath(filePaths);
    EXPECT_EQ(err.empty(), false);
}

TEST_F(UtilFileTest, TestVFileAccess)
{
    ock::common::VFileAccess vFile = {"name", false};
    auto ret = vFile.Validate("");
    EXPECT_EQ(ret, true);
    ock::common::VFileAccess vFile1 = {"name", true};
    ret = vFile1.Validate("");
    EXPECT_EQ(ret, false);
    ret = vFile1.Validate("test_file.txt");
    EXPECT_EQ(ret, false);
    ret = vFile1.Validate("/tmp/nonexistent_file.txt");
    EXPECT_EQ(ret, false);
}

TEST_F(UtilFileTest, TestAuditLogInit)
{
    auto ret = ock::common::LogAdapter::AuditLogInit("name", 1, 1);
    EXPECT_EQ(ret, HFAIL);
}

TEST_F(UtilFileTest, TestConLogLevel)
{
    ock::common::ConLogLevel logLevel;
    auto level = logLevel.Convert("INFO");

    EXPECT_EQ(level, "2");
}
} // namespace UT