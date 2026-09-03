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

#ifndef UBSMEM_LOGGER_MANAGER_H
#define UBSMEM_LOGGER_MANAGER_H

#include <sys/syslog.h>
#include <functional>
#include "ubsmem_logger_entry.h"
#include "ubsmem_logger_filesink.h"
#include "ubsmem_logger_ringbuffer.h"
#include "ubsmem_logger_writer.h"

namespace ubsmem::log {

class UbsmemLoggerManager {
public:
    static UbsmemLoggerManager *Instance();

    static void Destroy();

    explicit UbsmemLoggerManager() = default;

    ~UbsmemLoggerManager() = default;

    int Init(const UbsmemLoggerOptions &options, const std::shared_ptr<UbsmemLoggerWriter> &logWriter);

    int EnsureInited();

    bool IsInited() const
    {
        return logBuffer_ != nullptr;
    }

    void Exit();

    bool IsLog(UbsmemLogLevel level);

    void Push(UbsmemLoggerEntry &&loggerEntry);

    void Pop();
    void LogToSyslog(UbsmemLoggerEntry &loggerEntry);

    static uint32_t LogToSyslogLevel(UbsmemLogLevel &level);

    void SetLogLevel(UbsmemLogLevel level);

    void SetExternLogCallback(void (*func)(int, const char *))
    {
        externLogCallback_ = func;
    }
    void (*GetExternLogCallback() const)(int, const char *)
    {
        auto *target = externLogCallback_.target<void (*)(int, const char *)>();
        return target != nullptr ? *target : nullptr;
    }

    bool InitializeAuditSink(const std::string &basePath, uint32_t maxFileSize, uint32_t maxFileCount);
    bool IsAuditConfigured() const
    {
        return auditConfigured_;
    }

    static UbsmemLogLevel StringToLogLevel(const std::string &level);

    static UbsmemLoggerManager *gInstance;

private:
    static bool gInited_;
    static std::mutex gInitMutex_;
    UbsmemLogLevel minLogLevel_ = UbsmemLogLevel::INFO;
    bool syslogOpen_ = false;
    uint32_t syslogType_ = LOG_USER;
    static std::atomic<bool> threadRunning_;
    bool syncMode_ = false;
    std::unique_ptr<UbsmemLoggerDoubleBuffer> logBuffer_;
    std::thread loggingThread_;
    std::shared_ptr<UbsmemLoggerWriter> writer_;
    bool auditConfigured_ = false;
    std::function<void(int, const char *)> externLogCallback_;
};
} // namespace ubsmem::log

#endif // UBSMEM_LOGGER_MANAGER_H