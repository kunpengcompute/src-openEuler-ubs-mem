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
#ifndef LOG_H
#define LOG_H

#include <chrono>

#include "logger/ubsmem_logger_entry.h"
#include "logger/ubsmem_logger_manager.h"

#define DBG_LOG_TRACE 0L
#define DBG_LOG_DEBUG 1L
#define DBG_LOG_INFO 2L
#define DBG_LOG_WARN 3L
#define DBG_LOG_ERROR 4L
#define DBG_LOG_CRITICAL 5L
#define DBG_LOG_LEVEL_MAX 6L

// ---- 通用日志宏 ----
#ifdef DEBUG_MEM_UT
#define DBG_LOGCRITICAL(fmt, ...) (void)0
#define DBG_LOGERROR(fmt, ...) (void)0
#define DBG_LOGWARN(fmt, ...) (void)0
#define DBG_LOGINFO(fmt, ...) (void)0
#define DBG_LOGDEBUG(fmt, ...) (void)0
#define DBG_AUDITERROR(fmt, ...) (void)0
#define DBG_AUDITWARN(fmt, ...) (void)0
#define DBG_AUDITINFO(fmt, ...) (void)0
#define DBG_AUDITCRITICAL(fmt, ...) (void)0
#else
#define DBG_LOGCRITICAL(fmt, ...) UBSMEM_LOG_CRIT << fmt
#define DBG_LOGERROR(fmt, ...) UBSMEM_LOG_ERROR << fmt
#define DBG_LOGWARN(fmt, ...) UBSMEM_LOG_WARN << fmt
#define DBG_LOGINFO(fmt, ...) UBSMEM_LOG_INFO << fmt
#define DBG_LOGDEBUG(fmt, ...) UBSMEM_LOG_DEBUG << fmt
#define DBG_AUDITERROR(fmt, ...) UBSMEM_AUDIT_ERROR << fmt
#define DBG_AUDITWARN(fmt, ...) UBSMEM_AUDIT_WARN << fmt
#define DBG_AUDITINFO(fmt, ...) UBSMEM_AUDIT_INFO << fmt
#define DBG_AUDITCRITICAL(fmt, ...) UBSMEM_AUDIT_CRIT << fmt
#endif
#endif
