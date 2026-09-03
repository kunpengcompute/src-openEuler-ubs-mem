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

#ifndef MXM_COM_ERROR_H
#define MXM_COM_ERROR_H

/* **************************************** */
/* 通信模块错误码定义                          */
/* **************************************** */

/* 0x10011000 Hcom channel 空指针 */
#define MXM_COM_ERROR_CHANNEL_NULL 0x10011000

/* 0x10011001 非法的消息 */
#define MXM_COM_ERROR_MESSAGE_INVALID 0x10011001

/* 0x10011002 消息长度校验失败 */
#define MXM_COM_ERROR_MESSAGE_CHECK_SIZE_FAIL 0x10011002

/* 0x10011003 非法的操作码 */
#define MXM_COM_ERROR_MESSAGE_INVALID_OP_CODE 0x10011003

/* 0x10011004 创建引擎失败 */
#define MXM_COM_ERROR_ENGINE_CREATE_FAIL 0x10011004

/* 0x10011005 启动引擎失败 */
#define MXM_COM_ERROR_ENGINE_START_FAIL 0x10011005

/* 0x10011006 channel不存在 */
#define MXM_COM_ERROR_CHANNEL_NOT_FOUND 0x10011006

/* 0x10011007 获取对端Ip端口失败 */
#define MXM_COM_ERROR_GET_PEER_IP_PORT_FAIL 0x10011007

/* 0x10011008 引擎未初始化 */
#define MXM_COM_ERROR_ENGINE_NOT_INIT 0x10011008

/* 0x10011009 与远端建连失败 */
#define MXM_COM_ERROR_CONNECT_TO_PEER_FAIL 0x10011009

/* 0x1001100A 同步发消息失败 */
#define MXM_COM_ERROR_SYNC_CALL_FAIL 0x1001100A

/* 0x1001100B 异步发消息失败 */
#define MXM_COM_ERROR_ASYNC_CALL_FAIL 0x1001100B

/* 0x1001100C 回复发消息失败 */
#define MXM_COM_ERROR_REPLY_FAIL 0x1001100C

/* 0x1001100D callback创建失败 */
#define MXM_COM_ERROR_NEW_NET_CALLBACK_FAIL 0x1001100D

/* 0x1001100E 引擎获取失败 */
#define MXM_COM_ERROR_GET_ENGINE_FAIL 0x1001100E

/* 0x1001100F 与远端建连失败 */
#define MXM_COM_ERROR_INVALID_CHANNEL_TYPE 0x1001100F

/* 0x10011010 与远端建连失败 */
#define MXM_COM_ERROR_NEW_IPC_CLIENT_FAIL 0x10011010
#endif // MXM_COM_ERROR_H
