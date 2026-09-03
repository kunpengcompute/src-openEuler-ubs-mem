/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef MXM_IPC_INTERFACE_H
#define MXM_IPC_INTERFACE_H
#include <cstdint>
#include "comm_def.h"
#include "mxm_msg.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
     * @brief 消息结构体定义
     */
typedef struct {
    uint8_t *data; // 数据指针
    uint32_t len;  // 数据长度
} MxmComByteBuffer;

/**
     * @brief 异步发送回调函数定义
     *
     * @param[in] ctx: 上下文指针
     * @param[in] recv: 收到的消息指针
     * @param[in] len: 收到的消息长度
     * @param[in] result：消息发送结果
     */
typedef void (*MxmComCallbackFunc)(void *ctx, void *recv, uint32_t len, int32_t result);

/**
     * @brief 异步发送回调结构体定义
     */
typedef struct {
    MxmComCallbackFunc cb; // 回调函数指针
    void *cbCtx;           // 上下文指针
} MxmComCallbackDef;

/**
     * @brief 启动Ipc客户端
     * @return 成功返回0, 失败返回错误码
     */
int MxmComStartIpcClient(void);

/**
     * @brief 停止Ipc客户端
     */
void MxmComStopIpcClient(void);

/**
     * @brief 同步发送消息
     * response内存由框架申请，释放由调用方释放
     * @param[in] opCode: 操作码
     * @param[in] request: 请求体指针
     * @param[out] response: 返回消息指针
     */
int MxmComIpcClientSend(uint16_t opCode, ock::mxmd::MsgBase *request, ock::mxmd::MsgBase *response);
/* 与Server端重新建链后的处理函数 */
int MxmSetPostReconnectHandler(int (*handler)());
#ifdef __cplusplus
}
#endif
#endif // MXM_IPC_INTERFACE_H
