/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "log_adapter.h"

#include <memory>

#include "configuration.h"
#include "functions.h"
#include "logger/ubsmem_logger_constants.h"
#include "logger/ubsmem_logger_filesink.h"
#include "logger/ubsmem_logger_manager.h"
#include "logger/ubsmem_logger_writer.h"

namespace ock {
namespace common {

Lock LogAdapter::gLock;
Lock LogAdapter::gAuditLock;
uint32_t LogAdapter::gLevel = DBG_LOG_LEVEL_MAX;

static ubsmem::log::UbsmemLogLevel MapToUbsmLevel(int dbgLevel)
{
    // DBG_LOG 1-5 → UbsmemLogLevel 0-4
    switch (dbgLevel) {
        case DBG_LOG_DEBUG:
            return ubsmem::log::UbsmemLogLevel::DEBUG;
        case DBG_LOG_INFO:
            return ubsmem::log::UbsmemLogLevel::INFO;
        case DBG_LOG_WARN:
            return ubsmem::log::UbsmemLogLevel::WARN;
        case DBG_LOG_ERROR:
            return ubsmem::log::UbsmemLogLevel::ERROR;
        case DBG_LOG_CRITICAL:
            return ubsmem::log::UbsmemLogLevel::CRIT;
        default:
            return ubsmem::log::UbsmemLogLevel::INFO;
    }
}

HRESULT LogAdapter::LogServerInit(int minLogLevel, const std::string &path, int rotationFileSize, int rotationFileCount)
{
    GUARD(&gLock, gLock);

    gLevel = static_cast<uint32_t>(minLogLevel);

    std::string dirPath = path;
    if (dirPath.empty()) {
        dirPath = ConfConstant::MXMD_SERVER_LOG_PATH.second;
    }
    // 去掉尾部 '/' 和文件名，得到纯目录路径；FileSink 内部会拼 ubsmd.log
    if (!dirPath.empty() && dirPath.back() == '/') {
        dirPath.pop_back();
    }

    std::shared_ptr<ubsmem::log::UbsmemLoggerFilesink> sink;
    try {
        sink = std::make_shared<ubsmem::log::UbsmemLoggerFilesink>(dirPath, static_cast<uint32_t>(rotationFileSize),
                                                                   static_cast<uint32_t>(rotationFileCount));
    } catch (const std::exception &e) {
        std::cerr << "Failed to create log file sink: " << e.what() << std::endl;
        return HFAIL;
    }
    if (!sink->Initialize()) {
        DBG_LOGERROR("Failed to initialize log file sink, path=" << dirPath);
        return HFAIL;
    }

    ubsmem::log::UbsmemLoggerOptions opts;
    opts.bufferMaxItem = ubsmem::log::DEFAULT_BUFFER_MAX_ITEM;

    auto mgr = ubsmem::log::UbsmemLoggerManager::Instance();
    if (mgr == nullptr) {
        return HFAIL;
    }
    int ret = mgr->Init(opts, sink);
    if (ret != 0) {
        return HFAIL;
    }
    mgr->SetLogLevel(MapToUbsmLevel(minLogLevel));

    DBG_LOGINFO("Server log created successfully.");
    return HOK;
}

HRESULT LogAdapter::AuditLogInit(const std::string &path, int rotationFileSize, int rotationFileCount)
{
    GUARD(&gAuditLock, gAuditLock);

    std::string auditPath = path;
    if (auditPath.empty()) {
        auditPath = ConfConstant::MXMD_SERVER_AUDIT_LOG_PATH.second;
    }
    if (!ubsmem::log::UbsmemLoggerManager::Instance()->InitializeAuditSink(
            auditPath, static_cast<uint32_t>(rotationFileSize), static_cast<uint32_t>(rotationFileCount))) {
        DBG_LOGWARN("Failed to initialize audit log sink");
        return HFAIL;
    }

    DBG_LOGINFO("Audit log created successfully.");
    return HOK;
}

static int32_t MapStringToLevel(const std::string &level)
{
    if (level == "DEBUG")
        return DBG_LOG_DEBUG;
    if (level == "INFO")
        return DBG_LOG_INFO;
    if (level == "WARN")
        return DBG_LOG_WARN;
    if (level == "ERROR")
        return DBG_LOG_ERROR;
    if (level == "CRITICAL")
        return DBG_LOG_CRITICAL;
    return -1;
}

int32_t LogAdapter::StringToLogLevel(const std::string &level)
{
    auto logLevel = MapStringToLevel(level);
    if (logLevel >= 0) {
        return logLevel;
    }
    DBG_LOGERROR("Configuration module: The log level is incorrect. Use log level: INFO.");
    return DBG_LOG_INFO;
}

} // namespace common
} // namespace ock
