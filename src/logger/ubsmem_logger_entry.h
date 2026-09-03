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

#ifndef UBSMEM_LOGGER_H
#define UBSMEM_LOGGER_H
#include <pthread.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>

namespace ubsmem::log {

/**
 * @brief 定义Module名，为调试日志写入的文件名
 * @param mn [in] Module名，如ubsmem
 */
#ifndef FILENAME
#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define UBSMEM_LOGGER_CRIT                                              \
    ubsmem::log::UbsmemLogEnabled(ubsmem::log::UbsmemLogLevel::CRIT) && \
        ubsmem::log::UbsmemLog() ==                                     \
            ubsmem::log::UbsmemLoggerEntry(ubsmem::log::UbsmemLogLevel::CRIT, FILENAME, __func__, __LINE__)

#define UBSMEM_LOGGER_ERROR                                              \
    ubsmem::log::UbsmemLogEnabled(ubsmem::log::UbsmemLogLevel::ERROR) && \
        ubsmem::log::UbsmemLog() ==                                      \
            ubsmem::log::UbsmemLoggerEntry(ubsmem::log::UbsmemLogLevel::ERROR, FILENAME, __func__, __LINE__)

#define UBSMEM_LOGGER_WARN                                              \
    ubsmem::log::UbsmemLogEnabled(ubsmem::log::UbsmemLogLevel::WARN) && \
        ubsmem::log::UbsmemLog() ==                                     \
            ubsmem::log::UbsmemLoggerEntry(ubsmem::log::UbsmemLogLevel::WARN, FILENAME, __func__, __LINE__)

#define UBSMEM_LOGGER_INFO                                              \
    ubsmem::log::UbsmemLogEnabled(ubsmem::log::UbsmemLogLevel::INFO) && \
        ubsmem::log::UbsmemLog() ==                                     \
            ubsmem::log::UbsmemLoggerEntry(ubsmem::log::UbsmemLogLevel::INFO, FILENAME, __func__, __LINE__)

#define UBSMEM_LOGGER_DEBUG                                              \
    ubsmem::log::UbsmemLogEnabled(ubsmem::log::UbsmemLogLevel::DEBUG) && \
        ubsmem::log::UbsmemLog() ==                                      \
            ubsmem::log::UbsmemLoggerEntry(ubsmem::log::UbsmemLogLevel::DEBUG, FILENAME, __func__, __LINE__)

#define UBSMEM_LOG_CRIT UBSMEM_LOGGER_CRIT
#define UBSMEM_LOG_ERROR UBSMEM_LOGGER_ERROR
#define UBSMEM_LOG_WARN UBSMEM_LOGGER_WARN
#define UBSMEM_LOG_INFO UBSMEM_LOGGER_INFO
#define UBSMEM_LOG_DEBUG UBSMEM_LOGGER_DEBUG

#define UBSMEM_AUDIT_LOGGER(level)            \
    ubsmem::log::UbsmemAuditEnabled(level) && \
        ubsmem::log::UbsmemLog() == ubsmem::log::UbsmemLoggerEntry(level, FILENAME, __func__, __LINE__, true)

#define UBSMEM_AUDIT_CRIT UBSMEM_AUDIT_LOGGER(ubsmem::log::UbsmemLogLevel::CRIT)
#define UBSMEM_AUDIT_ERROR UBSMEM_AUDIT_LOGGER(ubsmem::log::UbsmemLogLevel::ERROR)
#define UBSMEM_AUDIT_WARN UBSMEM_AUDIT_LOGGER(ubsmem::log::UbsmemLogLevel::WARN)
#define UBSMEM_AUDIT_INFO UBSMEM_AUDIT_LOGGER(ubsmem::log::UbsmemLogLevel::INFO)

enum class UbsmemLogLevel : uint32_t { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, CRIT = 4, COUNT = 5 };

/**
 * @brief 格式化错误码
 * @param retCode [in] 错误码
 * @return 格式化后的错误码
 */
std::string FormatRetCode(uint32_t retCode);

enum class UbsmemLoggerTypeId : uint8_t { CHAR = 0, UINT32, UINT64, INT32, INT64, DOUBLE, STRING };

class UbsmemLoggerEntry {
public:
    UbsmemLoggerEntry(UbsmemLogLevel level, const char *file, const char *func, uint32_t line, bool isAudit = false);
    ~UbsmemLoggerEntry() = default;

    UbsmemLoggerEntry() = default;
    UbsmemLoggerEntry(const UbsmemLoggerEntry &other);
    UbsmemLoggerEntry &operator=(const UbsmemLoggerEntry &other);
    UbsmemLoggerEntry(UbsmemLoggerEntry &&) = default;
    UbsmemLoggerEntry &operator=(UbsmemLoggerEntry &&) = default;

    void OutPutLog(std::ostream &os) const;
    void FormatSyslog(std::ostream &os) const;

    UbsmemLogLevel GetLogLevel();
    const char *GetFile();
    uint32_t GetLine();
    bool IsAudit() const
    {
        return isAudit_;
    }
    void DecodePayload(std::ostream &os) const;
    uint64_t GetEntryTimeStamp();

    UbsmemLoggerEntry &operator<<(char data);
    UbsmemLoggerEntry &operator<<(int32_t data);
    UbsmemLoggerEntry &operator<<(uint32_t data);
    UbsmemLoggerEntry &operator<<(int64_t data);
    UbsmemLoggerEntry &operator<<(uint64_t data);
    UbsmemLoggerEntry &operator<<(double data);
    UbsmemLoggerEntry &operator<<(const std::string &data);
    UbsmemLoggerEntry &operator<<(const char *data);

    // 泛型兜底：转为 string 再编码
    template <typename T,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<T>, char> && !std::is_same_v<std::decay_t<T>, int32_t> &&
                  !std::is_same_v<std::decay_t<T>, uint32_t> && !std::is_same_v<std::decay_t<T>, int64_t> &&
                  !std::is_same_v<std::decay_t<T>, uint64_t> && !std::is_same_v<std::decay_t<T>, double> &&
                  !std::is_same_v<std::decay_t<T>, std::string> && !std::is_same_v<std::decay_t<T>, const char *>>>
    UbsmemLoggerEntry &operator<<(const T &data)
    {
        std::ostringstream oss;
        oss << data;
        return operator<<(oss.str());
    }

private:
    uint32_t ResizeBuffer(size_t addSize);

    char *GetBuffer();

    void EncodeString(const char *data, size_t length);

    template <typename T>
    void EncodeData(T data)
    {
        *reinterpret_cast<T *>(GetBuffer()) = data;
        currentSize_ += sizeof(T);
    }

    template <typename T>
    void EncodeData(UbsmemLoggerTypeId id, T data)
    {
        uint32_t ret = ResizeBuffer(sizeof(UbsmemLoggerTypeId) + sizeof(T));
        if (ret != 0) {
            return;
        }
        EncodeData<UbsmemLoggerTypeId>(id);
        EncodeData<T>(data);
    }

    void EncodeData(const char *data);
    void DecodeData(std::ostream &os, const char *start, const char *end) const;
    const char *DecodeChar(std::ostream &os, const char *buffer) const;
    const char *DecodeUint(std::ostream &os, const char *buffer) const;
    const char *DecodeUlong(std::ostream &os, const char *buffer) const;
    const char *DecodeInt(std::ostream &os, const char *buffer) const;
    const char *DecodeLong(std::ostream &os, const char *buffer) const;
    const char *DecodeDouble(std::ostream &os, const char *buffer) const;
    const char *DecodeString(std::ostream &os, const char *buffer) const;

    uint64_t timeStamp_ = 0;
    pid_t pid_ = 0;
    unsigned long tid_ = 0;
    UbsmemLogLevel level_ = UbsmemLogLevel::INFO;
    const char *file_ = nullptr;
    const char *func_ = nullptr;
    uint32_t line_ = 0;
    bool isAudit_ = false;

    size_t maxSize_ = 0;
    char logEntryBuffer_[512] = {0};
    std::unique_ptr<char[]> heapBuffer_{};
    size_t currentSize_ = 0;
};

bool UbsmemLogEnabled(UbsmemLogLevel level);
bool UbsmemAuditEnabled(UbsmemLogLevel level);

struct UbsmemLog {
    bool operator==(UbsmemLoggerEntry &loggerEntry);
};
} // namespace ubsmem::log
#endif // UBSMEM_LOGGER_H
