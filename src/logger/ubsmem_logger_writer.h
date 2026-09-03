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

#ifndef UBSMEM_LOGGER_WRITER_H
#define UBSMEM_LOGGER_WRITER_H
#include <iostream>
#include <utility>
#include "referable/dg_ref.h"
#include "syslog.h"
#include "ubsmem_logger_constants.h"
#include "ubsmem_logger_entry.h"

namespace ubsmem::log {
using namespace ock::dagger;

struct UbsmemLoggerOptions {
    UbsmemLogLevel minLogLevel = UbsmemLogLevel::INFO;
    uint32_t maxFileSizeInMB = 2;                     // 日志文件最大大小
    uint32_t rotationFileCount = 2;                   // 绕接个数
    uint32_t bufferMaxItem = DEFAULT_BUFFER_MAX_ITEM; // 日志缓冲区最大日志条目
    UbsmemLogLevel minSyslogLevel = UbsmemLogLevel::INFO;
    std::string logPath;
    bool syslogOpen = false;
    uint32_t syslogType = LOG_USER;
};

class UbsmemLoggerWriter {
public:
    virtual ~UbsmemLoggerWriter() = default;
    explicit UbsmemLoggerWriter() = default;

    virtual bool Write(const UbsmemLoggerEntry &loggerEntry) = 0;
};

class UbsmemLoggerDefaultWriter : public UbsmemLoggerWriter {
public:
    explicit UbsmemLoggerDefaultWriter() = default;

    bool Write(const UbsmemLoggerEntry &loggerEntry) override
    {
        loggerEntry.OutPutLog(std::cout);
        return true;
    }
};
} // namespace ubsmem::log
#endif // UBSMEM_LOGGER_WRITER_H
