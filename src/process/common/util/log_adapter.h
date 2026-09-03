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
#ifndef OCK_COMMON_COMMON_LOG_H
#define OCK_COMMON_COMMON_LOG_H

#include <string>
#include "defines.h"
#include "lock.h"
#include "log.h"

namespace ock {
namespace common {

class LogAdapter {
public:
    static HRESULT LogServerInit(int minLogLevel, const std::string &path, int rotationFileSize, int rotationFileCount);
    static HRESULT AuditLogInit(const std::string &path, int rotationFileSize, int rotationFileCount);
    static int32_t StringToLogLevel(const std::string &level);

    static Lock gLock;
    static Lock gAuditLock;
    static uint32_t gLevel;

    LogAdapter(const LogAdapter &) = delete;
    LogAdapter &operator=(const LogAdapter &) = delete;
    LogAdapter(const LogAdapter &&) = delete;
    LogAdapter &operator=(const LogAdapter &&) = delete;

private:
    LogAdapter() {}
    ~LogAdapter() {}
};

} // namespace common
} // namespace ock

#endif // OCK_COMMON_LOG_H
