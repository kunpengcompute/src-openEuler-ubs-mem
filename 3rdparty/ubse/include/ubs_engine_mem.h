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

#ifndef UBS_ENGINE_MEM_H
#define UBS_ENGINE_MEM_H

#include <stdint.h>
#include <sys/types.h>

#include "ubs_engine_topo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UBS_MEM_MAX_SLOT_NUM 16
#define UBS_MEM_MAX_MEMID_NUM 2048
#define UBS_MEM_MAX_NAME_LENGTH 48
#define UBSE_MAX_USR_INFO_LENGTH 32
#define UBS_MEM_MIN_SIZE (4ULL * 1024ULL * 1024ULL)
#define UBS_MEM_MAX_LENDER_CNT 4
#define UBS_MEM_FLAG_NO_WR_DELAY 0x1   // 非写接力
#define UBS_MEM_FLAG_SHM_ANONYMOUS 0x2 // 共享内存没有使用方时, 后台对账时自动清理提供方
#define UBS_MEM_FLAG_CACHEABLE 0x4     // 设置cacheableFlag为1
#define UBS_MEM_MAX_DESC_LIST 2000

typedef enum {
    NUMA_LOCAL, // 本地numa
    NUMA_REMOTE // 远端numa, 当前不支持
} ubs_mem_numa_type_t;

typedef enum {
    UBSE_NOT_EXIST = 0,         // 借用关系不存在
    UBSE_CREATING = 1,          // 正在创建中
    UBSE_DELETING = 2,          // 正在删除中
    UBSE_EXIST = 3,             // 创建成功
    UBSE_ERR_ONLY_IMPORT = 4,   // 只存在借入
    UBSE_ERR_WAIT_UNEXPORT = 5, // 等待unexport执行，对账会执行，可以手动删除
    UBSE_END = 6                // 类型转换边界值,不表示任何内存状态
} ubs_mem_stage;

typedef struct {
    uint32_t slot_id;              // 节点唯一标识, 采用slotid, 与lcne保持一致
    uint32_t socket_id;            // socket id
    uint32_t numa_id;              // 节点中的numa id
    ubs_mem_numa_type_t numa_type; // numa类型
    uint32_t mem_lend_ratio;       // 池化内存借出比例上限
    uint64_t mem_total;            // 内存总量, 单位字节
    uint64_t mem_free;             // 内存空闲量, 单位字节
    uint32_t huge_pages_2M;        // 2M大页数量
    uint32_t free_huge_pages_2M;   // 2M大页空闲数量
    uint32_t huge_pages_1G;        // 1G大页数量
    uint32_t free_huge_pages_1G;   // 1G大页空闲数量
    uint64_t mem_borrow;           // 借用的内存，单位字节
    uint64_t mem_lend;             // 借出的内存，单位字节
} ubs_mem_numastat_t;

/**
 * @brief 查询指定节点numa信息，当前只支持本地内存，后续会增加远端numa
 *
 * @param slot_id [IN] 节点id
 * @param node_numa_mem_list [OUT] 节点numa信息数组, 调用方需要使用free接口主动释放内存
 * @param node_numa_mem_cnt [OUT] 节点numa信息个数, 范围 [0, UBS_TOPO_NUMA_NUM]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numastat_get(uint32_t slot_id, ubs_mem_numastat_t **numa_mems, uint32_t *numa_mem_cnt);

// 使用方进程信息
typedef struct {
    uid_t uid; // 属主进程的运行用户的uid
    gid_t gid; // 属主进程的的运行用户的groupid
    pid_t pid; // 属主进程的的运行用户的pid, 指定pid，pid消失时, ubs自动释放借用内存(暂不提供)
} ubs_mem_fd_owner_t;

typedef enum {
    MEM_DISTANCE_L0, // L0对应直连节点
    MEM_DISTANCE_L1, // L1对应通过1跳节点, 暂不支持
    MEM_DISTANCE_L2  // L2对应过超过1跳节点 , 暂不支持
} ubs_mem_distance_t;

typedef struct {
    uint64_t lender_size; // 借出内存大小, 单位Byte, 取值范围大于等于4*1024*1024
    uint32_t slot_id;     // 节点唯一标识, 采用slotid, 与lcne保持一致
    uint32_t socket_id;   // socket id
    uint32_t numa_id;     // 节点中的numa id
    uint32_t port_id;     // 指定链路借用
} ubs_mem_lender_t;

#define UBS_MEM_DEV_NAME_PREFIX "obmm_shmdev"
#define UBS_MEM_DEV_PATH "/dev/" UBS_MEM_DEV_NAME_PREFIX
typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH];     // 借用标识
    uint32_t memid_cnt;                     // 导出的内存块数量
    uint64_t memids[UBS_MEM_MAX_MEMID_NUM]; // 内存块标识信息，FD的文件形成规则：UBS_MEM_DEV_PATH + memid
    uint64_t mem_size;                      // 导出内存的总大小, 取值为拆分粒度的整数倍, 单位Byte
    size_t unit_size;                       // 芯片表项拆分粒度, 单位Byte
    ubs_topo_node_t export_node;            // 借出节点, 其中ips字段无效, 需通过topo接口获取
    ubs_topo_node_t import_node;            // 借入节点, 其中ips字段无效, 需通过topo接口获取
    ubs_mem_stage mem_stage;                // 内存状态
} ubs_mem_fd_desc_t;

/**
 * @brief  在本节点创建fd形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name在节点内保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param owner [IN] 内存资源使用者属主信息, 可选参数, Null不关注该字段
 * @param mode [IN] 内存资源使用者访问权限, 可选参数, 0则不关注该字段
 * @param distance [IN] 内存访问距离
 * @param fd_desc [OUT] 内存描述信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name或者size参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_create(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                          ubs_mem_distance_t distance, ubs_mem_fd_desc_t *fd_desc);

/**
 * @brief  指定借出信息，在本节点创建fd形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_";  name在节点内保持唯一性
 * @param owner [IN] 内存资源属主信息, 可选参数, Null不关注该字段
 * @param mode [IN] 内存资源的访问权限, 可选参数, 0则不关注该字段
 * @param lender [IN] 借出信息
 * @param lender_cnt [IN] 借出信息个数, 最大为UBS_MEM_MAX_LENDER_CNT
 * @param fd_desc [OUT] 内存描述信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */

int32_t ubs_mem_fd_create_with_lender(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                      const ubs_mem_lender_t *lender, uint32_t lender_cnt, ubs_mem_fd_desc_t *fd_desc);

/**
 * @brief  指定候选借出节点范围，在本节点创建fd形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 候选节点与当前借入节点，必须满足直连要求
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param owner [IN] 内存资源属主信息, 可选参数, Null不关注该字段
 * @param mode [IN] 内存资源的访问权限, 可选参数, 0则不关注该字段
 * @param slot_ids [IN] 候选借出节点范围, 最大为16
 * @param slot_cnt [IN] 候选借出节点个数
 * @param fd_desc [OUT] 内存描述信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_create_with_candidate(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                         const uint32_t *slot_ids, uint32_t slot_cnt, ubs_mem_fd_desc_t *fd_desc);

/**
 * @brief 改变本节点fd形态远端内存的permission信息; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param owner [IN] 内存资源属主信息, 必选参数, 不允许为Null
 * @param mode [IN] 内存资源的访问权限, 必选参数, 不允许为0
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_permission(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode);

/**
 * @brief 查询本节点fd形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param fd_desc [OUT] fd内存信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_get(const char *name, ubs_mem_fd_desc_t *fd_desc);

/**
 * @brief 查询本节点所有fd形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param fd_descs [OUT] fd内存描述信息数组, 调用成功时，调用方需要使用free接口主动释放内存
 * @param fd_desc_cnt [OUT] fd内存描述信息数组中的元素个数, 范围 [0, UBS_MEM_MAX_DESC_LIST]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name前缀参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_list(ubs_mem_fd_desc_t **fd_descs, uint32_t *fd_desc_cnt);

/**
 * @brief 删除本节点指定fd远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name  [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_IMPORT_ABSENT:IMPORT不在位, 无法删除;
 * UBS_ENGINE_ERR_CREATING:正在创建过程中;
 * UBS_ENGINE_ERR_DELETING:正在删除过程中;
 * UBS_ENGINE_ERR_UNIMPORT_SUCCESS:UNIMPORT已经成功, unexport失败, 资源没有释放完全, 后续对账能自动回收;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_fd_delete(const char *name);

typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH];        // 借用标识
    int64_t numaid;                            // 形成远端numa对应的numaid
    ubs_topo_node_t export_node;               // 借出节点, 其中ips字段无效, 需通过topo接口获取
    ubs_topo_node_t import_node;               // 借入节点, 其中ips字段无效, 需通过topo接口获取
    uint64_t size;                             // 借用大小
    ubs_mem_stage mem_stage;                   // 内存状态
    uint8_t usrInfo[UBSE_MAX_USR_INFO_LENGTH]; // 调用方私有数据，UBSE只负责保存，get时原样返回
} ubs_mem_numa_desc_t;

/**
 * @brief 在本节点创建numa形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param distance [IN] 内存访问跳数
 * @param numa_desc [OUT] 借用形成的远端numa信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name或者size参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_create(const char *name, uint64_t size, ubs_mem_distance_t distance,
                            ubs_mem_numa_desc_t *numa_desc);

/**
 * @brief  指定借出信息，在本节点创建numa形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param lender [IN] 借出信息
 * @param lender_cnt [IN] 借出信息个数, 最大为UBS_MEM_MAX_LENDER_CNT
 * @param numa_desc [OUT] 借用形成的远端numa信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_create_with_lender(const char *name, const ubs_mem_lender_t *lender, uint32_t lender_cnt,
                                        ubs_mem_numa_desc_t *numa_desc);

/**
 * @brief  指定候选借出节点，在本节点创建numa形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 候选节点与当前借入节点，必须满足直连要求
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param slot_ids [IN] 候选借出节点范围
 * @param slot_cnt [IN] 候选借出节点个数, 最大为16
 * @param numa_desc [OUT] 借用形成的远端numa信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_create_with_candidate(const char *name, uint64_t size, const uint32_t *slot_ids, uint32_t slot_cnt,
                                           ubs_mem_numa_desc_t *numa_desc);

/**
 * @brief 查询本节点numa形态远端内存信息; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @param numa_desc [OUT] 借用形成的远端numa信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_get(const char *name, ubs_mem_numa_desc_t *numa_desc);

/**
 * @brief 查询本节点所有numa形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param numa_desc [OUT] numa内存描述信息数组调用成功时, 调用成功时, 调用方需要使用free接口主动释放内存
 * @param numa_desc_cnt [OUT] numa内存描述信息数组中的元素个数
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_list(ubs_mem_numa_desc_t **numa_descs, uint32_t *numa_desc_cnt);

/**
 * @brief 删除本节点指定numa远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name  [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name在节点内保持唯一性
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_IMPORT_ABSENT:IMPORT不在位, 无法删除;
 * UBS_ENGINE_ERR_CREATING:正在创建过程中;
 * UBS_ENGINE_ERR_DELETING:正在删除过程中;
 * UBS_ENGINE_ERR_UNIMPORT_SUCCESS:UNIMPORT已经成功, unexport失败, 资源没有释放完全, 后续对账能自动回收;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_numa_delete(const char *name);

typedef struct {
    uint32_t node_cnt; // 实际有效的节点数量, 取值范围[1-16],
                       // 若超过slot_ids中已初始化的数据范围, 可能导致越界访问未定义数据
    uint32_t slot_ids[UBS_MEM_MAX_SLOT_NUM]; // 节点ID数组，实际有效长度为 node_cnt, 超出部分无效
} ubs_mem_nodes_t;

#define UBS_MEM_MAX_USR_INFO_LEN 32

/**
 * @brief
 * 创建共享形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 算法决策借出节点，指定资源提供方，则从中选择，没有指定，则从region中选择；确保所有节点均能使用共享内存
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param usr_info [IN] 调用方私有数据(当前有NC/CC信息), UBSE只负责保存，get时原样返回
 * @param flag [IN] 额外的内存借用属性，目前支持写接力
 * @param region [IN] 后续使用共享内存的节点范围, region内的节点必须能通过公共节点直连, 必选参数
 * @param provider [IN] 资源提供方节点范围, null表示不指定, 非null表示指定
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name或者size参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_create(const char *name, uint64_t size, uint8_t usr_info[32], uint64_t flag,
                           const ubs_mem_nodes_t *region, const ubs_mem_nodes_t *provider);

/**
 * @brief
 * 创建共享形态的远端内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 算法决策借出节点，指定资源提供方，则从中选择，没有指定，则从region中选择；确保所有节点均能使用共享内存
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param size [IN] 借用大小, 单位Byte, 取值范围大于等于4*1024*1024
 * @param affinity_socket_id [IN] 亲和的cpu socket_id
 * @param usr_info [IN] 调用方私有数据(当前有NC/CC信息), UBSE只负责保存，get时原样返回
 * @param flag [IN] 额外的内存借用属性，目前支持写接力
 * 传递给OBMM的内存访问属性；全0表示无效，UBSE自行组装访问属性；非全零，直接使用该字段传递给OBMM
 * @param region [IN] 后续使用共享内存的节点范围, region内的节点必须能通过公共节点直连, 必选参数
 * @param provider [IN] 资源提供方节点范围, null表示不指定, 非null表示指定
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name或者size参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_create_with_affinity(const char *name, uint64_t size, uint32_t affinity_socket_id,
                                         uint8_t usr_info[32], uint64_t flag, const ubs_mem_nodes_t *region,
                                         const ubs_mem_nodes_t *provider);

/**
 * @brief
 * 指定numa创建共享内存，该资源的管理权限属于调用者
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 算法决策借出节点，指定资源提供方，则从中选择，没有指定，则从region中选择；确保所有节点均能使用共享内存
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param usr_info [IN] 调用方私有数据(当前有NC/CC信息), UBSE只负责保存，get时原样返回
 * @param flag [IN] 额外的内存借用属性，目前支持写接力
 * 传递给OBMM的内存访问属性；全0表示无效，UBSE自行组装访问属性；非全零，直接使用该字段传递给OBMM
 * @param region [IN] 后续使用共享内存的节点范围, region内的节点必须能通过公共节点直连, 必选参数
 * @param lender [IN] 借出方信息
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name或者size参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_EXISTED:借用关系已存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_create_with_lender(const char *name, uint8_t usr_info[UBS_MEM_MAX_USR_INFO_LEN], uint64_t flag,
                                       const ubs_mem_nodes_t *region, const ubs_mem_lender_t *lender);

typedef struct {
    uint32_t memid_cnt;                     // 导出的内存块数量
    uint64_t memids[UBS_MEM_MAX_MEMID_NUM]; // 内存块标识信息，FD的文件形成规则：UBSE_MEM_DEV_PATH + memid
    ubs_topo_node_t import_node;            // 借入节点, 其中ips字段无效, 需通过topo接口获取
    ubs_mem_stage mem_stage;                // 内存状态
} ubs_mem_shm_import_desc_t;
typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH]; // 借用标识
    uint64_t mem_size;                  // 导出内存的总大小, 取值为拆分粒度的整数倍, 单位Byte
    size_t unit_size;                   // 芯片表项拆分粒度, 单位Byte
    ubs_topo_node_t export_node;        // 借出节点, 其中ips字段无效, 需通过topo接口获取
    uint8_t usr_info[UBS_MEM_MAX_USR_INFO_LEN];
    uint32_t import_desc_cnt;               // 导入内存描述符信息的数量, 范围 [0, UBS_TOPO_MAX_NODE_NUM]
    ubs_mem_stage mem_stage;                // 内存状态
    ubs_mem_shm_import_desc_t *import_desc; // 导入内存描述符信息
} ubs_mem_shm_desc_t;

/**
 * @brief 导入共享形态的远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param owner [IN] 内存资源属主信息, 可选参数, Null不关注该字段
 * @param mode [IN] 内存资源的访问权限, 可选参数, 0则不关注该字段
 * @param shm_desc [OUT] 内存描述信息, 调用方需要使用free接口主动释放内存
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_SHM_NO_CREATE:共享内存未创建;
 * UBS_ENGINE_ERR_CREATING:正在创建过程中;
 * UBS_ENGINE_ERR_DELETING:正在删除过程中;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_attach(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                           ubs_mem_shm_desc_t **shm_desc);

/**
 * @brief 查询指定共享形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识前缀, 最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param shm_desc [OUT] 借用形成的远端fd信息, 调用方需要使用free接口主动释放内存
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_get(const char *name, ubs_mem_shm_desc_t **shm_desc);

/**
 * @brief 查询指定借用标识前缀的共享形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param shm_descs [OUT] numa内存描述信息数组调用成功时, 调用成功时, 调用方需要使用free接口主动释放内存
 * @param shm_desc_cnt [OUT] numa内存描述信息数组中的元素个数, 范围 [0, UBS_MEM_MAX_DESC_LIST]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_list(ubs_mem_shm_desc_t **shm_descs, uint32_t *shm_desc_cnt);

/**
 * @brief 查询指定借用标识前缀的共享形态远端内存; 最多查询到2000条借用内存;
 * 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同;
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid;
 *
 * @param name_prefix [IN] 指定借用标识前缀
 * @param shm_descs [OUT] 共享内存描述信息数组, 调用成功时, 调用方需要使用free接口主动释放内存
 * @param shm_desc_cnt [OUT] 共享内存描述信息数组中的元素个数, 范围 [0, UBS_MEM_MAX_DESC_LIST]
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name_prefix参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_list_with_prefix(const char *name_prefix, ubs_mem_shm_desc_t **shm_descs, uint32_t *shm_desc_cnt);

/**
 * @brief 删除导入共享形态的远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_SHM_NO_ATTACH:共享内存未导入;
 * UBS_ENGINE_ERR_SHM_ATTACHING:正在导入共享内存过程中;
 * UBS_ENGINE_ERR_SHM_DETACHING:正在删除导入共享内存过程中;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_detach(const char *name);

/**
 * @brief 删除指定共享形态远端内存; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 * 该接口只设置删除标记，等到所有attach都解除之后才删除；
 * 标记删除后，原有attach能继续使用，新的attach返回失败；
 * 标记删除后，借出节点重启，即使attach还存在，也会完成真正删除
 *
 * @param name  [IN] 借用标识, name最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在;
 * UBS_ENGINE_ERR_CREATING:正在创建过程中;
 * UBS_ENGINE_ERR_DELETING:正在删除过程中;
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_delete(const char *name);

typedef enum {
    UB_MEM_ATOMIC_DATA_ERR = 0,
    UB_MEM_READ_DATA_ERR,
    UB_MEM_FLOW_POISON,
    UB_MEM_FLOW_READ_AUTH_POISON,
    UB_MEM_FLOW_READ_AUTH_RESPERR,
    UB_MEM_TIMEOUT_POISON,
    UB_MEM_TIMEOUT_RESPERR,
    UB_MEM_READ_DATA_POISON,
    UB_MEM_READ_DATA_RESPERR,
    MAR_NOPORT_VLD_INT_ERR, // 无物理地址
    MAR_FLUX_INT_ERR,
    MAR_WITHOUT_CXT_ERR,
    RSP_BKPRE_OVER_TIMEOUT_ERR, // 无物理地址
    MAR_NEAR_AUTH_FAIL_ERR,
    MAR_FAR_AUTH_FAIL_ERR,
    MAR_TIMEOUT_ERR,
    MAR_ILLEGAL_ACCESS_ERR,
    REMOTE_READ_DATA_ERR_OR_WRITE_RESPONSE_ERR,
    UB_MEM_HEALTHY = 1000, // 无故障
} ubs_mem_fault_type_t;

typedef struct {
    uint32_t memid_cnt;                                       // 导出的内存块数量
    uint64_t memids[UBS_MEM_MAX_MEMID_NUM];                   // 导出内存块标识信息
    ubs_mem_fault_type_t memid_status[UBS_MEM_MAX_MEMID_NUM]; // 内存块的健康状态，与memids一一对应
} ubs_mem_memids_fault_t;

typedef int32_t (*ubs_mem_shm_fault_handler)(const char *name, uint64_t memid, ubs_mem_fault_type_t type);

/**
 * @brief 查询指定共享远端内存的状态; 调用该接口能操控的资源：创建资源时的标识与本次调用方的标识相同；
 * 调用者的标识处理原则：能够获取username，则使用username；无法获取username，则使用uid；
 *
 * @param name [IN] 借用标识, 最大长度48字节, 含结尾字符\0; name仅可包括大小写字母、数字、"."、":"、"-"以及"_"; name全局保持唯一性
 * @param shm_status [OUT] 内存块的健康状态
 * @return UBS_SUCCESS:操作成功;
 * UBS_ERR_NULL_POINTER:空指针;
 * UBS_ENGINE_ERR_OUT_OF_RANGE:name参数超出范围;
 * UBS_ENGINE_ERR_CONNECTION_FAILED:连接UBSE服务端失败;
 * UBS_ENGINE_ERR_AUTH_FAILED:UBSE服务端鉴权不通过;
 * UBS_ENGINE_ERR_NOT_EXIST:借用关系不存在
 * UBS_ENGINE_ERR_TIMEOUT:UBSE服务端处理超时;
 * UBS_ENGINE_ERR_INTERNAL:UBSE服务端内部错误
 */
int32_t ubs_mem_shm_fault_get(const char *name, ubs_mem_memids_fault_t *fault);

/**
 * @brief 客户端订阅共享内存UB Event事件
 * @param[in] registerFunc: 共享内存UB Event事件响应处理函数
 * @return 成功返回0, 失败返回非0
 */
int32_t ubs_mem_shm_fault_register(ubs_mem_shm_fault_handler handler);
#ifdef __cplusplus
}
#endif
#endif // UBS_ENGINE_MEM_H
