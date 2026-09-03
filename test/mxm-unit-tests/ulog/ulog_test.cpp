/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 *
 * ubs-mem is licensed under the Mulan PSL v2.
 */
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>

#include "log.h"
#include "logger/ubsmem_logger_manager.h"
#include "logger/ubsmem_logger_writer.h"

using namespace ubsmem::log;

namespace UT {
static UbsmemLoggerDefaultWriter g_testWriter;

static int InitLoggerForTest()
{
    UbsmemLoggerOptions opts;
    opts.bufferMaxItem = 4096;
    auto mgr = UbsmemLoggerManager::Instance();
    if (mgr == nullptr) {
        return -1;
    }
    return mgr->Init(opts, std::shared_ptr<UbsmemLoggerWriter>(&g_testWriter, [](auto *) {}));
}

static void DestroyLoggerForTest()
{
    auto mgr = UbsmemLoggerManager::Instance();
    mgr->Exit();
    UbsmemLoggerManager::Destroy();
}

TEST(ulog, test_stdout)
{
    ASSERT_EQ(InitLoggerForTest(), 0);
    auto mgr = UbsmemLoggerManager::Instance();
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);

    DBG_LOGINFO("test stdout INFO");
    DBG_LOGDEBUG("test stdout DEBUG");
    DBG_LOGERROR("test stdout ERROR");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DestroyLoggerForTest();
}

TEST(ulog, test_file)
{
    const char *path = "/tmp/ulog_test";
    UbsmemLoggerFilesink sink(path, 20 * 1024 * 1024, 10);
    sink.Initialize();

    UbsmemLoggerOptions opts;
    opts.bufferMaxItem = 4096;

    auto mgr = UbsmemLoggerManager::Instance();
    ASSERT_NE(mgr, nullptr);
    int ret = mgr->Init(opts, std::shared_ptr<UbsmemLoggerWriter>(&sink, [](auto *) {}));
    ASSERT_EQ(ret, 0);
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);

    DBG_LOGINFO("test file INFO");
    DBG_LOGERROR("test file ERROR");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr->Exit();
    UbsmemLoggerManager::Destroy();
}

TEST(ulog, test_logger_entry_operators)
{
    ASSERT_EQ(InitLoggerForTest(), 0);
    auto mgr = UbsmemLoggerManager::Instance();
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);

    UBSMEM_LOG_INFO << "string_test";
    UBSMEM_LOG_DEBUG << 42;
    UBSMEM_LOG_WARN << uint32_t(100);
    UBSMEM_LOG_ERROR << int64_t(-1);
    UBSMEM_LOG_CRIT << uint64_t(999);
    UBSMEM_LOG_INFO << 3.14;
    UBSMEM_LOG_INFO << 'X';
    UBSMEM_LOG_INFO << "multi"
                    << " "
                    << "args";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DestroyLoggerForTest();
}

TEST(ulog, test_log_levels)
{
    ASSERT_EQ(InitLoggerForTest(), 0);
    auto mgr = UbsmemLoggerManager::Instance();

    mgr->SetLogLevel(UbsmemLogLevel::ERROR);
    UBSMEM_LOG_ERROR << "should appear";
    UBSMEM_LOG_INFO << "should NOT appear";

    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);
    UBSMEM_LOG_DEBUG << "back to debug";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DestroyLoggerForTest();
}

TEST(ulog, test_logger_entry_direct)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "test_file", "test_func", 42);
    EXPECT_EQ(entry.GetLogLevel(), UbsmemLogLevel::INFO);
    EXPECT_STREQ(entry.GetFile(), "test_file");
    EXPECT_EQ(entry.GetLine(), 42);
    EXPECT_GT(entry.GetEntryTimeStamp(), 0);
    EXPECT_FALSE(entry.IsAudit());

    UbsmemLoggerEntry entry2(UbsmemLogLevel::ERROR, "f", "g", 1, true);
    EXPECT_TRUE(entry2.IsAudit());

    UbsmemLoggerEntry entry3;
    EXPECT_EQ(entry3.GetLogLevel(), UbsmemLogLevel::INFO);
}

TEST(ulog, test_logger_entry_operators_direct)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "test", "func", 1);
    entry << "hello" << 42 << uint32_t(100) << int64_t(-1) << uint64_t(999) << 3.14 << 'X';
    std::ostringstream oss;
    entry.OutPutLog(oss);
    EXPECT_GT(oss.str().size(), 0);
}

TEST(ulog, test_logger_entry_long_data)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "test", "func", 1);
    std::string big(8000, 'B');
    entry << big;
    std::ostringstream oss;
    entry.DecodePayload(oss);
    EXPECT_GT(oss.str().size(), 0);
}

TEST(ulog, test_logger_entry_copy)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::WARN, "src", "copy_test", 10);
    entry << "test_data";

    UbsmemLoggerEntry copied(entry);
    EXPECT_EQ(copied.GetLogLevel(), UbsmemLogLevel::WARN);
    EXPECT_STREQ(copied.GetFile(), "src");

    UbsmemLoggerEntry assigned(UbsmemLogLevel::DEBUG, "dst", "assign_test", 20);
    assigned = entry;
    EXPECT_EQ(assigned.GetLogLevel(), UbsmemLogLevel::WARN);
    EXPECT_STREQ(assigned.GetFile(), "src");
}

TEST(ulog, test_logger_entry_move)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "src", "move_test", 5);
    entry << "data";

    UbsmemLoggerEntry moved(std::move(entry));
    EXPECT_EQ(moved.GetLogLevel(), UbsmemLogLevel::INFO);
}

TEST(ulog, test_format_ret_code)
{
    std::string result = FormatRetCode(12345);
    EXPECT_NE(result.find("12345"), std::string::npos);
}

TEST(ulog, test_output_log)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "out_test", "output", 7);
    entry << "content";
    std::ostringstream oss;
    entry.OutPutLog(oss);
    EXPECT_NE(oss.str().size(), 0);
    EXPECT_NE(oss.str().find("out_test"), std::string::npos);
    EXPECT_NE(oss.str().find("INFO"), std::string::npos);
}

TEST(ulog, test_format_syslog)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::ERROR, "sys_test", "syslog", 3);
    entry << "sysmsg";
    std::ostringstream oss;
    entry.FormatSyslog(oss);
    EXPECT_NE(oss.str().size(), 0);
    EXPECT_NE(oss.str().find("sys_test"), std::string::npos);
}

TEST(ulog, test_decode_payload)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::DEBUG, "decode", "decode", 1);
    entry << "payload_data";
    std::ostringstream oss;
    entry.DecodePayload(oss);
    EXPECT_NE(oss.str().size(), 0);
    EXPECT_NE(oss.str().find("payload_data"), std::string::npos);
}

TEST(ulog, test_long_message_resize)
{
    UbsmemLoggerEntry entry(UbsmemLogLevel::INFO, "resize", "resize", 1);

    std::string longStr(2000, 'A');
    entry << longStr;

    std::ostringstream oss;
    entry.DecodePayload(oss);
    EXPECT_NE(oss.str().size(), 0);
}

TEST(ulog, test_log_enabled)
{
    EXPECT_FALSE(UbsmemAuditEnabled(UbsmemLogLevel::INFO));
}

TEST(ulog, test_filesink_init_fail)
{
    UbsmemLoggerFilesink sink("", 0, 0);
    EXPECT_FALSE(sink.Initialize());
}

TEST(ulog, test_string_to_log_level)
{
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("DEBUG"), UbsmemLogLevel::DEBUG);
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("INFO"), UbsmemLogLevel::INFO);
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("WARN"), UbsmemLogLevel::WARN);
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("ERROR"), UbsmemLogLevel::ERROR);
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("CRIT"), UbsmemLogLevel::CRIT);
    EXPECT_EQ(UbsmemLoggerManager::StringToLogLevel("UNKNOWN"), UbsmemLogLevel::INFO);
}

TEST(ulog, test_initialize_audit_sink_no_filesink)
{
    InitLoggerForTest();
    auto mgr = UbsmemLoggerManager::Instance();
    EXPECT_FALSE(mgr->InitializeAuditSink("/tmp", 1024, 5));
    DestroyLoggerForTest();
}

TEST(ulog, test_reinit)
{
    ASSERT_EQ(InitLoggerForTest(), 0);
    auto mgr = UbsmemLoggerManager::Instance();
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);
    UBSMEM_LOG_INFO << "first init";
    DestroyLoggerForTest();

    ASSERT_EQ(InitLoggerForTest(), 0);
    mgr = UbsmemLoggerManager::Instance();
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);
    UBSMEM_LOG_INFO << "second init";
    DestroyLoggerForTest();
}

TEST(ulog, test_filesink_write)
{
    char tmpl[] = "/tmp/ulog_fsink_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);

    UbsmemLoggerOptions opts;
    opts.bufferMaxItem = 4096;

    UbsmemLoggerFilesink sink(dir, 1024 * 1024, 10);
    sink.Initialize();

    auto mgr = UbsmemLoggerManager::Instance();
    ASSERT_NE(mgr, nullptr);
    int ret = mgr->Init(opts, std::shared_ptr<UbsmemLoggerWriter>(&sink, [](auto *) {}));
    ASSERT_EQ(ret, 0);
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);

    UBSMEM_LOG_INFO << "filesink write test";
    UBSMEM_LOG_DEBUG << "debug message " << 42;
    UBSMEM_LOG_WARN << "warning: value=" << uint64_t(100);
    UBSMEM_LOG_ERROR << "error: code=" << int32_t(-1);
    UBSMEM_LOG_CRIT << "critical error!";
    UBSMEM_LOG_INFO << "a=" << 'X' << " b=" << int64_t(-999) << " c=" << 3.14;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr->Exit();
    UbsmemLoggerManager::Destroy();

    std::string cmd = std::string("rm -rf ") + dir;
    system(cmd.c_str());
}

TEST(ulog, test_filesink_roll)
{
    char tmpl[] = "/tmp/ulog_roll_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);

    UbsmemLoggerOptions opts;
    opts.bufferMaxItem = 4096;

    UbsmemLoggerFilesink sink(dir, 1, 5);
    sink.Initialize();

    auto mgr = UbsmemLoggerManager::Instance();
    ASSERT_NE(mgr, nullptr);
    int ret = mgr->Init(opts, std::shared_ptr<UbsmemLoggerWriter>(&sink, [](auto *) {}));
    ASSERT_EQ(ret, 0);
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);

    UBSMEM_LOG_INFO << "trigger roll";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    UBSMEM_LOG_INFO << "second write after roll";

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr->Exit();
    UbsmemLoggerManager::Destroy();

    std::string cmd = std::string("rm -rf ") + dir;
    system(cmd.c_str());
}

TEST(ulog, test_filesink_audit_init)
{
    char tmpl[] = "/tmp/ulog_audit_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);

    UbsmemLoggerFilesink sink(dir, 1024 * 1024, 10);
    sink.Initialize();

    EXPECT_TRUE(sink.InitializeAuditSink(dir, 1024 * 1024, 10));

    std::string cmd = std::string("rm -rf ") + dir;
    system(cmd.c_str());
}

TEST(ulog, test_ensure_inited)
{
    auto mgr = UbsmemLoggerManager::Instance();
    ASSERT_NE(mgr, nullptr);
    int ret = mgr->EnsureInited();
    EXPECT_EQ(ret, 0);
    mgr->SetLogLevel(UbsmemLogLevel::DEBUG);
    UBSMEM_LOG_INFO << "ensure inited test";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    mgr->Exit();
    UbsmemLoggerManager::Destroy();
}

TEST(ulog, test_filesink_init_audit_create_dir)
{
    char tmpl[] = "/tmp/ulog_audit_create_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);
    std::string subDir = std::string(dir) + "/sub";
    UbsmemLoggerFilesink sink(dir, 1024 * 1024, 10);
    sink.Initialize();
    EXPECT_TRUE(sink.InitializeAuditSink(subDir, 1024 * 1024, 10));
    std::string cmd = std::string("rm -rf ") + dir;
    system(cmd.c_str());
}

TEST(ulog, test_filesink_init_fail_zero_params)
{
    UbsmemLoggerFilesink sink("/tmp", 0, 0);
    EXPECT_FALSE(sink.Initialize());
}

TEST(ulog, test_filesink_init_empty_path)
{
    UbsmemLoggerFilesink sink("", 1024, 5);
    EXPECT_FALSE(sink.Initialize());
}

TEST(ulog, test_filesink_is_file_status_changed)
{
    char tmpl[] = "/tmp/ulog_fstat_XXXXXX";
    char *dir = mkdtemp(tmpl);
    ASSERT_NE(dir, nullptr);

    UbsmemLoggerFilesink sink(dir, 1024 * 1024, 10);
    sink.Initialize();
    EXPECT_TRUE(sink.InitializeAuditSink(dir, 1024 * 1024, 10));

    std::string cmd = std::string("rm -rf ") + dir;
    system(cmd.c_str());
}
} // namespace UT