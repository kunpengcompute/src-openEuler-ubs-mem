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
#ifndef UBSM_COM_CONSTANTS_H
#define UBSM_COM_CONSTANTS_H
#include <cstdint>

constexpr auto MXM_AGENT_IPC_SERVER_ENGINE_NAME = "MxmAgentIpcServerEngine";
constexpr auto MXM_AGENT_IPC_CLIENT_ENGINE_NAME = "MxmAgentIpcClientEngine";
constexpr auto MXM_AGENT_IPC_SERVER_NAME = "MxmAgentIpcServer";
constexpr auto FAKE_CUR_NODE_ID = "FakeCurNodeId";
constexpr auto MXM_IPC_UDS_PATH_PREFIX_DEFAULT = "/run/matrix/memory/";
constexpr auto REMOTE_NODE_ID_PREFIX = "RpcNodeId_";
constexpr auto RACK_HCOM_FILE_PATH_PREFIX = "HCOM_FILE_PATH_PREFIX";
constexpr auto KEY_SEP = "-";
// 大数据场景下，默认单次最大发送数据量大小，单位MB
constexpr uint32_t DEFAULT_MAX_SENDRECEIVE_SIZE = 5;
constexpr uint32_t DEFAULT_SEND_RECEIVE_SEG_COUNT = 128;

constexpr uint16_t MODULES_SIZE = 1000; // 最大模块数
constexpr uint16_t OP_CODE_SIZE = 1000; // 最大操作码

constexpr uint16_t MIN_UDS_PERM = 0660;  // 最小权限
constexpr uint16_t UDS_PATH_PERM = 0750; // 目录权限
#endif                                   // UBSM_COM_CONSTANTS_H
