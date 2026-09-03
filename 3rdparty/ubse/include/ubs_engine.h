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

#ifndef UBS_ENGINE_H
#define UBS_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化ubse客户端
 *
 * @param ubs_engine_uds_path [IN]
 * ubse服务端的uds文件路径；传入空指针或空路径，则采用ubse默认地址/var/run/ubse/ubse.sock ；路径长度限制：遵从linux
 * `sun_path` 的大小是 108 字节 (`#define UNIX_PATH_MAX 108`)，所以路径不能超过 107 个字符（不含结尾的空字符 `\0`）
 * @return UBS_SUCCESS:操作成功;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:参数数据长度超108字节;
 * UBS_ENGINE_ERR_RESOURCE:资源创建失败;
 */
int32_t ubs_engine_client_initialize(const char *ubs_engine_uds_path);

/**
 * @brief   销毁ubse客户端
 */
void ubs_engine_client_finalize(void);

#ifdef __cplusplus
}
#endif
#endif // UBS_ENGINE_H
