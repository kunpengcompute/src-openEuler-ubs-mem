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

#include "ubsmem_logger_manager.h"

#include <sys/syslog.h>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>

#include "log.h"
#include "ubsmem_logger_constants.h"
#include "ubsmem_logger_ringbuffer.h"

namespace ubsmem::log {
UbsmemLoggerManager *UbsmemLoggerManager::gInstance = nullptr;
bool UbsmemLoggerManager::gInited_ = false;
std::mutex UbsmemLoggerManager::gInitMutex_;
std::atomic<bool> UbsmemLoggerManager::threadRunning_;

UbsmemLoggerManager *UbsmemLoggerManager::Instance()
{
    if (gInstance != nullptr) {
        return gInstance;
    }
    std::lock_guard<std::mutex> lock(gInitMutex_);
    if (gInstance != nullptr) {
        return gInstance;
    }
    gInstance = new (std::nothrow) UbsmemLoggerManager();
    if (gInstance == nullptr) {
        std::cerr << "Failed to new UbseLogger object, probably out of memory";
    }
    return gInstance;
}

int UbsmemLoggerManager::EnsureInited()
{
    std::lock_guard<std::mutex> lock(gInitMutex_);
    if (gInited_) {
        return 0;
    }
    auto writer = std::make_shared<UbsmemLoggerDefaultWriter>();
    UbsmemLoggerOptions opts;
    opts.minLogLevel = UbsmemLogLevel::DEBUG;
    opts.bufferMaxItem = DEFAULT_BUFFER_MAX_ITEM;
    syncMode_ = true;
    return Init(opts, writer);
}

void UbsmemLoggerManager::Destroy()
{
    if (gInstance != nullptr) {
        if (gInited_) {
            gInstance->Exit();
        }
        delete gInstance;
        gInstance = nullptr;
    }
    gInited_ = false;
}

int UbsmemLoggerManager::Init(const UbsmemLoggerOptions &options, const std::shared_ptr<UbsmemLoggerWriter> &logWriter)
{
    if (gInited_) {
        return 0;
    }
    if (logWriter == nullptr) {
        return -1;
    }
    this->syslogOpen_ = options.syslogOpen;
    this->syslogType_ = options.syslogType;
    gInstance->writer_ = logWriter;
    if (!syncMode_) {
        threadRunning_.store(true);
        try {
            logBuffer_ = std::make_unique<UbsmemLoggerDoubleBuffer>(options.bufferMaxItem);
            loggingThread_ = std::thread([this] { UbsmemLoggerManager::Pop(); });
        } catch (...) {
            std::cerr << "Out of memory or create thread failed." << std::endl;
            gInstance->writer_.reset();
            threadRunning_.store(false);
            return -1;
        }
    }
    gInited_ = true;
    return 0;
}

void UbsmemLoggerManager::Exit()
{
    if (logBuffer_ == nullptr) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(logBuffer_->mtx_);
    logBuffer_->stop_ = true;
    lock.unlock();
    threadRunning_.store(false);
    if (loggingThread_.joinable()) {
        loggingThread_.join();
    }
}

bool UbsmemLoggerManager::IsLog(UbsmemLogLevel level)
{
    return level >= minLogLevel_;
}

void UbsmemLoggerManager::Push(UbsmemLoggerEntry &&loggerEntry)
{
    if (syncMode_) {
        if (externLogCallback_ != nullptr) {
            std::ostringstream oss;
            oss << "[UBS_SDK " << loggerEntry.GetFile() << ":" << loggerEntry.GetLine() << "] ";
            loggerEntry.DecodePayload(oss);
            externLogCallback_(static_cast<int>(loggerEntry.GetLogLevel()), oss.str().c_str());
        } else {
            writer_->Write(loggerEntry);
        }
        if (syslogOpen_) {
            LogToSyslog(loggerEntry);
        }
        return;
    }
    if (logBuffer_ == nullptr) {
        return;
    }
    bool dropped = false;
    logBuffer_->Push(std::move(loggerEntry), dropped);
}

void UbsmemLoggerManager::Pop()
{
    UbsmemLoggerEntry loggerEntry(UbsmemLogLevel::INFO, nullptr, nullptr, 0);
    while (threadRunning_.load()) {
        if (logBuffer_->Pop(loggerEntry)) {
            // SDK 外部回调：[UBS_SDK file:line] payload
            if (externLogCallback_ != nullptr) {
                std::ostringstream oss;
                oss << "[UBS_SDK " << loggerEntry.GetFile() << ":" << loggerEntry.GetLine() << "] ";
                loggerEntry.DecodePayload(oss);
                externLogCallback_(static_cast<int>(loggerEntry.GetLogLevel()), oss.str().c_str());
            } else {
                writer_->Write(loggerEntry);
            }
            // syslog打开且不被过滤
            if (syslogOpen_) {
                LogToSyslog(loggerEntry);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 环形缓冲区无数据则线程休眠1毫秒
        }
    }
    while (logBuffer_->Pop(loggerEntry)) {
        // SDK 外部回调
        if (externLogCallback_ != nullptr) {
            std::ostringstream oss;
            oss << "[UBS_SDK " << loggerEntry.GetFile() << ":" << loggerEntry.GetLine() << "] ";
            loggerEntry.DecodePayload(oss);
            externLogCallback_(static_cast<int>(loggerEntry.GetLogLevel()), oss.str().c_str());
        } else {
            writer_->Write(loggerEntry);
        }
        if (this->syslogOpen_) {
            LogToSyslog(loggerEntry);
        }
    }
}

void UbsmemLoggerManager::LogToSyslog(UbsmemLoggerEntry &loggerEntry)
{
    auto level = loggerEntry.GetLogLevel();
    auto syslogLevel = LogToSyslogLevel(level);
    openlog("ubsmem", 0, this->syslogType_);
    std::ostringstream oss;
    loggerEntry.FormatSyslog(oss);
    syslog(syslogLevel, "%s", oss.str().c_str());
    closelog();
}

uint32_t UbsmemLoggerManager::LogToSyslogLevel(UbsmemLogLevel &level)
{
    if (level == UbsmemLogLevel::DEBUG) {
        return LOG_DEBUG;
    }
    if (level == UbsmemLogLevel::INFO) {
        return LOG_INFO;
    }
    if (level == UbsmemLogLevel::WARN) {
        return LOG_WARNING;
    }
    if (level == UbsmemLogLevel::ERROR) {
        return LOG_ERR;
    }
    if (level == UbsmemLogLevel::CRIT) {
        return LOG_CRIT;
    }
    return LOG_INFO;
}

void UbsmemLoggerManager::SetLogLevel(UbsmemLogLevel level)
{
    minLogLevel_ = level;
}

bool UbsmemLoggerManager::InitializeAuditSink(const std::string &basePath, uint32_t maxFileSize, uint32_t maxFileCount)
{
    auto sink = dynamic_cast<UbsmemLoggerFilesink *>(writer_.get());
    if (sink == nullptr) {
        std::cerr << "Audit sink initialization failed: no filesink writer." << std::endl;
        return false;
    }
    if (!sink->InitializeAuditSink(basePath, maxFileSize, maxFileCount)) {
        return false;
    }
    auditConfigured_ = true;
    return true;
}

UbsmemLogLevel UbsmemLoggerManager::StringToLogLevel(const std::string &level)
{
    if (level == "DEBUG") {
        return UbsmemLogLevel::DEBUG;
    }
    if (level == "INFO") {
        return UbsmemLogLevel::INFO;
    }
    if (level == "WARN") {
        return UbsmemLogLevel::WARN;
    }
    if (level == "ERROR") {
        return UbsmemLogLevel::ERROR;
    }
    if (level == "CRIT") {
        return UbsmemLogLevel::CRIT;
    }
    return UbsmemLogLevel::INFO;
}
} // namespace ubsmem::log