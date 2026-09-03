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

#ifndef UBSMEM_LOGGER_CONSTANTS_H
#define UBSMEM_LOGGER_CONSTANTS_H

#include <sys/syslog.h>
#include <cstdint>

namespace ubsmem {
namespace log {

// ---- 文件滚动参数 ----
constexpr uint32_t DEFAULT_LOG_FILE_SIZE_MB = 20;
constexpr uint32_t MIN_LOG_FILE_SIZE_MB = 2;
constexpr uint32_t MAX_LOG_FILE_SIZE_MB = 100;
constexpr uint32_t DEFAULT_LOG_FILE_COUNT = 10;
constexpr uint32_t MIN_LOG_FILE_COUNT = 1;
constexpr uint32_t MAX_LOG_FILE_COUNT = 50;

// ---- ring buffer ----
constexpr uint32_t DEFAULT_BUFFER_MAX_ITEM = 4096;
constexpr uint32_t MIN_BUFFER_ITEM = 64;
constexpr uint32_t MAX_BUFFER_ITEM = 4096;

// ---- 日志路径 / 文件名 ----
constexpr const char *DEFAULT_LOG_PATH = "/var/log/ubsm/";
constexpr const char *LOG_FILE_NAME = "ubsmd.log";
constexpr const char *AUDIT_FILE_NAME = "ubsmd.audit.log";

// ---- 默认日志级别 ----
constexpr const char *DEFAULT_LOG_LEVEL_STR = "INFO";

// ---- syslog（默认关闭）----
constexpr bool DEFAULT_SYSLOG_OPEN = false;
constexpr uint32_t DEFAULT_SYSLOG_TYPE = LOG_USER;

// ---- 文件权限 ----
constexpr int LOG_DIR_PERMISSION = 0750;

} // namespace log
} // namespace ubsmem

#endif // UBSMEM_LOGGER_CONSTANTS_H
