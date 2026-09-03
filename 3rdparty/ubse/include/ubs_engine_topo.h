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

#ifndef UBS_ENGINE_TOPO_H
#define UBS_ENGINE_TOPO_H

#include <bits/local_lim.h>
#include <netinet/in.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UBS_TOPO_SOCKET_NUM 2
#define UBS_TOPO_IPADDR_NUM 50
#define UBS_TOPO_NUMA_NUM 4
#define UBS_TOPO_MAX_NODE_NUM 512
#define UBS_TOPO_MAX_CPU_LINK_NUM 1024

typedef struct {
    int32_t af;           // 地址族，ipv4为AF_INET，ipv6为AF_INET6
    struct in_addr ipv4;  // ipv4地址
    struct in6_addr ipv6; // IPv6地址
} ubs_topo_ip_address_t;

typedef struct {
    uint32_t slot_id;
    uint32_t socket_id[UBS_TOPO_SOCKET_NUM];
    uint32_t numa_ids[UBS_TOPO_SOCKET_NUM][UBS_TOPO_NUMA_NUM];
    ubs_topo_ip_address_t ips[UBS_TOPO_IPADDR_NUM];
    char host_name[HOST_NAME_MAX]; // 主机名
} ubs_topo_node_t;

/**
 * @brief 查询全量节点信息
 *
 * @param node_list [OUT] 节点信息数组, 调用方需要使用free主动释放内存
 * @param cnt [OUT] 节点信息个数, 范围 [0, UBS_TOPO_MAX_NODE_NUM]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_topo_node_list(ubs_topo_node_t **node_list, uint32_t *node_cnt);

/**
 * @brief 查询本节点信息
 *
 * @param node [OUT] 节点信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_topo_node_local_get(ubs_topo_node_t *node);

typedef struct {
    uint32_t slot_id;        // 节点id
    uint32_t socket_id;      // socket id, 0xFFFFFFFF表示无效值
    uint32_t port_id;        // 端口id
    uint32_t peer_slot_id;   // 对端节点id
    uint32_t peer_socket_id; // 对端socket id, 0xFFFFFFFF表示无效值
    uint32_t peer_port_id;   // 对端端口id
} ubs_topo_link_t;

/**
 * @brief 查询所有CPU类型节点的组网拓扑信息，粒度为硬件连线
 *
 * @param cpu_links [OUT] cpu连接信息数组, 调用方需要使用free接口主动释放内存
 * @param cpu_link_cnt [OUT] cpu连接信息个数, 范围 [0, UBS_TOPO_MAX_CPU_LINK_NUM]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_topo_link_list(ubs_topo_link_t **cpu_links, uint32_t *cpu_link_cnt);
#ifdef __cplusplus
}
#endif
#endif // UBS_ENGINE_TOPO_H
