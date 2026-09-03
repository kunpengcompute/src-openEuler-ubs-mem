/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * ubs-engine is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef UBS_ENGINE_LOG_H
#define UBS_ENGINE_LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 日志回调函数
 */
typedef void (*ubs_engine_log_handler)(uint32_t level, const char *message);

/**
 * @brief  注册日志回调函数，没有注册日志函数或空指针，采用标准输出
 *
 * @param handler [IN] 日志回调函数
 */
void ubs_engine_log_callback_register(ubs_engine_log_handler handler);

#ifdef __cplusplus
}
#endif
#endif // UBS_ENGINE_LOG_H
