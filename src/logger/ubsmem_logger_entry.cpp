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

#include "ubsmem_logger_entry.h"

#include <securec.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <utility>

#include "ubsmem_logger_manager.h"

namespace ubsmem::log {
static uint64_t GetTimeStamp()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                     std::chrono::high_resolution_clock::now().time_since_epoch())
                                     .count());
}

static pid_t GetProcessId()
{
    return getpid();
}

static unsigned long GetThreadId()
{
    return static_cast<unsigned long>(pthread_self());
}

static void FormatTimestamp(std::ostringstream &oss, uint64_t timestamp)
{
    // 定义日期时间缓冲区大小、每秒微秒数、每毫秒微秒数
    constexpr int dateTimeBufferSize = 32;                // 日期时间缓冲区的大小
    constexpr uint64_t microsecondsPerSecond = 1000000;   // 1秒 = 1000000微秒
    constexpr uint64_t microsecondsPerMillisecond = 1000; // 1毫秒 = 1000微秒
    constexpr int millisecondWidth = 3;                   // 毫秒部分的宽度
    // 将时间戳从微秒转换为秒
    std::time_t seconds = static_cast<std::time_t>(timestamp / microsecondsPerSecond);
    // 定义本地时间和GMT时间的时间结构
    std::tm localTime{}; // 本地时间
    std::tm gmtTime{};   // GMT时间
    // 根据秒数获取本地时间和GMT时间
    localtime_r(&seconds, &localTime); // 获取本地时间（本地时区）
    gmtime_r(&seconds, &gmtTime);      // 获取GMT时间（全球协调时间）
    // 初始化一个缓冲区来存储格式化后的日期时间
    char dateTimeBuffer[dateTimeBufferSize] = {0};
    // 使用本地时间格式化日期时间到缓冲区
    if (strftime(dateTimeBuffer, sizeof(dateTimeBuffer), "%Y-%m-%d %T.", &localTime) == 0) {
        dateTimeBuffer[0] = '\0';
    }
    // 将本地时间和GMT时间转换回秒
    std::time_t localSeconds = mktime(&localTime);
    std::time_t gmtSeconds = mktime(&gmtTime);
    // 计算时区偏移量（本地时间 - GMT时间）
    int offsetSeconds = static_cast<int>(difftime(localSeconds, gmtSeconds));
    // 将偏移量转换为小时和分钟
    int offsetHours = offsetSeconds / 3600;                    // 小时
    int offsetMinutes = (std::abs(offsetSeconds) % 3600) / 60; // 分钟
    // 初始化一个缓冲区存储时区偏移量字符串
    char tzBuffer[16] = {0};
    std::snprintf(tzBuffer, sizeof(tzBuffer), "%+03d:%02d", offsetHours, offsetMinutes);
    // 计算时间戳中的毫秒部分
    uint64_t milliseconds = (timestamp % microsecondsPerSecond) / microsecondsPerMillisecond;
    // 时间戳格式为：[YYYY-MM-DD HH:MM:SS.mmm +HH:MM]
    oss << '[' << dateTimeBuffer << std::setw(millisecondWidth) << std::setfill('0') << milliseconds << ' ' << tzBuffer
        << ']';
}

static const char *LogLevelToString(UbsmemLogLevel level)
{
    switch (level) {
        case UbsmemLogLevel::DEBUG:
            return "DEBUG";
        case UbsmemLogLevel::INFO:
            return "INFO";
        case UbsmemLogLevel::WARN:
            return "WARN";
        case UbsmemLogLevel::ERROR:
            return "ERROR";
        case UbsmemLogLevel::CRIT:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

std::string FormatRetCode(uint32_t retCode)
{
    std::string retCodeString = "ErrorCode=" + std::to_string(retCode);
    return retCodeString;
}

UbsmemLoggerEntry::UbsmemLoggerEntry(UbsmemLogLevel level, const char *file, const char *func, uint32_t line,
                                     bool isAudit)
    : level_(level),
      file_(file),
      func_(func),
      line_(line),
      isAudit_(isAudit),
      maxSize_(sizeof(logEntryBuffer_)),
      currentSize_(0)
{
    timeStamp_ = GetTimeStamp();
    pid_ = GetProcessId();
    tid_ = GetThreadId();
}

UbsmemLoggerEntry::UbsmemLoggerEntry(const UbsmemLoggerEntry &other)
    : timeStamp_(other.timeStamp_),
      pid_(other.pid_),
      tid_(other.tid_),
      level_(other.level_),
      file_(other.file_),
      func_(other.func_),
      line_(other.line_),
      isAudit_(other.isAudit_),
      maxSize_(other.maxSize_),
      currentSize_(other.currentSize_)
{
    if (other.heapBuffer_) {
        heapBuffer_.reset(new (std::nothrow) char[maxSize_]);
        if (heapBuffer_ == nullptr) {
            return;
        }
        errno_t ret = memcpy_s(heapBuffer_.get(), currentSize_, other.heapBuffer_.get(), currentSize_);
        if (EOK != ret) {
            heapBuffer_.reset();
            std::cerr << "Failed to copy heapBuffer." << std::endl;
        }
    } else {
        errno_t ret = memcpy_s(logEntryBuffer_, currentSize_, other.logEntryBuffer_, currentSize_);
        if (EOK != ret) {
            std::cerr << "Failed to copy logEntryBuffer." << std::endl;
        }
    }
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator=(const UbsmemLoggerEntry &other)
{
    if (this == &other) {
        return *this;
    }

    timeStamp_ = other.timeStamp_;
    pid_ = other.pid_;
    tid_ = other.tid_;
    level_ = other.level_;
    file_ = other.file_;
    func_ = other.func_;
    line_ = other.line_;
    isAudit_ = other.isAudit_;
    maxSize_ = other.maxSize_;
    currentSize_ = other.currentSize_;

    if (other.heapBuffer_) {
        heapBuffer_.reset(new (std::nothrow) char[maxSize_]);
        if (heapBuffer_ == nullptr) {
            return *this;
        }
        errno_t ret = memcpy_s(heapBuffer_.get(), currentSize_, other.heapBuffer_.get(), currentSize_);
        if (EOK != ret) {
            heapBuffer_.reset();
            std::cerr << "ERROR: failed to copy." << std::endl;
        }
    } else {
        heapBuffer_.reset();
        errno_t ret = memcpy_s(logEntryBuffer_, currentSize_, other.logEntryBuffer_, currentSize_);
        if (EOK != ret) {
            std::cerr << "ERROR: failed to copy." << std::endl;
        }
    }

    return *this;
}

void UbsmemLoggerEntry::OutPutLog(std::ostream &os) const
{
    std::ostringstream oss;
    const char *start = !heapBuffer_ ? &logEntryBuffer_[0] : &(heapBuffer_.get())[0];
    const char *end = start + currentSize_;

    FormatTimestamp(oss, timeStamp_);
    oss << '[' << LogLevelToString(level_) << "][" << pid_ << "][" << tid_ << "]";
    oss << "[" << file_ << ':' << line_ << "]";
    if (isAudit_) {
        oss << "[AUDIT]";
    }
    oss << " ";
    DecodeData(oss, start, end);
    os << oss.str() << std::endl;
}
void UbsmemLoggerEntry::FormatSyslog(std::ostream &os) const
{
    std::ostringstream oss;
    const char *start = !heapBuffer_ ? &logEntryBuffer_[0] : &(heapBuffer_.get())[0];
    const char *end = start + currentSize_;
    oss << "[" << file_ << ':' << line_ << "] ";
    DecodeData(oss, start, end);
    os << oss.str() << std::endl;
}

const char *UbsmemLoggerEntry::GetFile()
{
    return file_;
}

uint32_t UbsmemLoggerEntry::GetLine()
{
    return line_;
}

UbsmemLogLevel UbsmemLoggerEntry::GetLogLevel()
{
    return level_;
}
uint64_t UbsmemLoggerEntry::GetEntryTimeStamp()
{
    return timeStamp_;
}

void UbsmemLoggerEntry::DecodePayload(std::ostream &os) const
{
    const char *start = !heapBuffer_ ? &logEntryBuffer_[0] : &(heapBuffer_.get())[0];
    const char *end = start + currentSize_;
    DecodeData(os, start, end);
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(char data)
{
    EncodeData<char>(UbsmemLoggerTypeId::CHAR, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(int32_t data)
{
    EncodeData<int32_t>(UbsmemLoggerTypeId::INT32, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(uint32_t data)
{
    EncodeData<uint32_t>(UbsmemLoggerTypeId::UINT32, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(int64_t data)
{
    EncodeData<int64_t>(UbsmemLoggerTypeId::INT64, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(uint64_t data)
{
    EncodeData<uint64_t>(UbsmemLoggerTypeId::UINT64, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(double data)
{
    EncodeData<double>(UbsmemLoggerTypeId::DOUBLE, data);
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(const std::string &data)
{
    EncodeString(data.c_str(), data.length());
    return *this;
}

UbsmemLoggerEntry &UbsmemLoggerEntry::operator<<(const char *data)
{
    EncodeData(data);
    return *this;
}

uint32_t UbsmemLoggerEntry::ResizeBuffer(size_t addSize)
{
    size_t const newSize = currentSize_ + addSize;
    if (newSize <= maxSize_) {
        return 0;
    }

    maxSize_ = std::max(static_cast<size_t>(2 * maxSize_), newSize); // 重新分配2倍大小buffer
    if (!heapBuffer_) {
        heapBuffer_.reset(new (std::nothrow) char[maxSize_]);
        if (heapBuffer_ == nullptr) {
            return -1;
        }
        auto err = memcpy_s(heapBuffer_.get(), maxSize_, logEntryBuffer_, currentSize_);
        if (err != EOK) {
            return -1;
        }
        return 0;
    } else {
        std::unique_ptr<char[]> newHeapBuffer(new (std::nothrow) char[maxSize_]);
        if (newHeapBuffer == nullptr) {
            return -1;
        }
        if (memcpy_s(newHeapBuffer.get(), maxSize_, heapBuffer_.get(), currentSize_) != EOK) {
            return -1;
        }
        heapBuffer_.swap(newHeapBuffer);
        return 0;
    }
}

char *UbsmemLoggerEntry::GetBuffer()
{
    if (!heapBuffer_) {
        return &logEntryBuffer_[currentSize_];
    }
    return &(heapBuffer_.get())[currentSize_];
}

void UbsmemLoggerEntry::EncodeString(const char *data, size_t length)
{
    if (length == 0) {
        return;
    }
    int ret = ResizeBuffer(sizeof(UbsmemLoggerTypeId) + length + 1);
    if (ret != 0) {
        return;
    }
    char *buffer = GetBuffer();
    *reinterpret_cast<UbsmemLoggerTypeId *>(buffer++) = UbsmemLoggerTypeId::STRING;
    if (memcpy_s(buffer, length + 1, data, length + 1) != EOK) {
        return;
    }
    currentSize_ += sizeof(UbsmemLoggerTypeId) + length + 1;
}

void UbsmemLoggerEntry::EncodeData(const char *data)
{
    if (data != nullptr) {
        EncodeString(data, strlen(data));
    }
}

const char *UbsmemLoggerEntry::DecodeChar(std::ostream &os, const char *buffer) const
{
    char data = *reinterpret_cast<const char *>(buffer);
    os << data;
    return buffer + sizeof(char);
}

const char *UbsmemLoggerEntry::DecodeUint(std::ostream &os, const char *buffer) const
{
    uint32_t data = *reinterpret_cast<const uint32_t *>(buffer);
    os << data;
    return buffer + sizeof(uint32_t);
}

const char *UbsmemLoggerEntry::DecodeUlong(std::ostream &os, const char *buffer) const
{
    uint64_t data = *reinterpret_cast<const uint64_t *>(buffer);
    os << data;
    return buffer + sizeof(uint64_t);
}

const char *UbsmemLoggerEntry::DecodeInt(std::ostream &os, const char *buffer) const
{
    int32_t data = *reinterpret_cast<const int32_t *>(buffer);
    os << data;
    return buffer + sizeof(int32_t);
}

const char *UbsmemLoggerEntry::DecodeLong(std::ostream &os, const char *buffer) const
{
    int64_t data = *reinterpret_cast<const int64_t *>(buffer);
    os << data;
    return buffer + sizeof(int64_t);
}
const char *UbsmemLoggerEntry::DecodeDouble(std::ostream &os, const char *buffer) const
{
    double data = *reinterpret_cast<const double *>(buffer);
    os << data;
    return buffer + sizeof(double);
}

const char *UbsmemLoggerEntry::DecodeString(std::ostream &os, const char *buffer) const
{
    while (*buffer != '\0') {
        os << *buffer;
        ++buffer;
    }
    return ++buffer;
}

void UbsmemLoggerEntry::DecodeData(std::ostream &os, const char *start, const char *end) const
{
    while (start < end) {
        auto id = static_cast<UbsmemLoggerTypeId>(*start);
        start++;
        switch (id) {
            case UbsmemLoggerTypeId::CHAR:
                start = DecodeChar(os, start);
                break;
            case UbsmemLoggerTypeId::UINT32:
                start = DecodeUint(os, start);
                break;
            case UbsmemLoggerTypeId::UINT64:
                start = DecodeUlong(os, start);
                break;
            case UbsmemLoggerTypeId::INT32:
                start = DecodeInt(os, start);
                break;
            case UbsmemLoggerTypeId::INT64:
                start = DecodeLong(os, start);
                break;
            case UbsmemLoggerTypeId::DOUBLE:
                start = DecodeDouble(os, start);
                break;
            case UbsmemLoggerTypeId::STRING:
                start = DecodeString(os, start);
                break;
            default:
                return;
        }
    }
}

bool UbsmemLogEnabled(UbsmemLogLevel level)
{
    auto mgr = ubsmem::log::UbsmemLoggerManager::Instance();
    if (mgr == nullptr) {
        return false;
    }
    if (!mgr->IsInited()) {
        if (mgr->EnsureInited() != 0) {
            return false;
        }
    }
    return mgr->IsLog(level);
}

bool UbsmemAuditEnabled(UbsmemLogLevel level)
{
    if (!UbsmemLoggerManager::gInstance || !UbsmemLoggerManager::gInstance->IsAuditConfigured()) {
        return false;
    }
    return UbsmemLogEnabled(level);
}

bool UbsmemLog::operator==(UbsmemLoggerEntry &loggerEntry)
{
    auto mgr = ubsmem::log::UbsmemLoggerManager::Instance();
    if (mgr != nullptr) {
        mgr->Push(std::move(loggerEntry));
    }
    return true;
}
} // namespace ubsmem::log