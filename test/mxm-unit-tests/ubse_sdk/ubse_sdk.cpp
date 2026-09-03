/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "ubs_engine.h"
#include "ubs_engine_log.h"
#include "ubs_engine_mem.h"
#include "ubs_engine_topo.h"
#include "ubs_error.h"
#ifdef __cplusplus
extern "C" {
#endif
namespace UT {

#define MXE_DESCS_NUM 1024
#define MXE_SOCKET_PATH_LEN 107

// 模拟全局共享内存描述信息列表（静态变量）
typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH + 1];
    mode_t mode;
    ubs_mem_shm_desc_t desc;
} shm_desc_entry_t;

static shm_desc_entry_t global_shm_descs[MXE_DESCS_NUM]; // 全局内存描述信息
static uint32_t global_shm_desc_count = 0;

// 检查name是否存在
int is_name_exists(const char *name)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

// 添加描述信息到全局列表
void add_shm_desc_to_list(ubs_mem_shm_desc_t *desc)
{
    printf("in add_shm_desc_to_list\n");
    strncpy(global_shm_descs[global_shm_desc_count].name, desc->name, UBS_MEM_MAX_NAME_LENGTH);
    global_shm_descs[global_shm_desc_count].desc = *desc;
    global_shm_desc_count++;
}

// 获取shm_desc数量
uint32_t get_shm_desc_count()
{
    return global_shm_desc_count;
}

// 根据name获取shm_desc
void get_shm_desc_by_name(const char *name, ubs_mem_shm_desc_t *desc)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            *desc = global_shm_descs[i].desc;
            return;
        }
    }
}

// 删除shm_desc
void delete_shm_desc_by_name(const char *name)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            // 找到匹配的条目，先释放其import_desc占用的内存
            printf("delete_shm_desc_by_name: %s\n", name);
            if (global_shm_descs[i].desc.import_desc) {
                // 释放import_desc占用的内存
                free(global_shm_descs[i].desc.import_desc);
                global_shm_descs[i].desc.import_desc = NULL;
            }
            // 移除该条目
            for (uint32_t j = i; j < global_shm_desc_count - 1; j++) {
                global_shm_descs[j] = global_shm_descs[j + 1];
            }
            global_shm_desc_count--;
            return;
        }
    }
}

// 获取所有shm_desc
void get_all_shm_descs(ubs_mem_shm_desc_t **descs, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&(*descs)[i], &global_shm_descs[i].desc, sizeof(ubs_mem_shm_desc_t));
    }
}

int32_t ubs_mem_shm_list(ubs_mem_shm_desc_t **shm_descs, uint32_t *shm_desc_cnt)
{
    if (!shm_descs || !shm_desc_cnt) {
        return UBS_ERR_NULL_POINTER;
    }
    *shm_desc_cnt = global_shm_desc_count;
    *shm_descs = static_cast<ubs_mem_shm_desc_t *>(malloc(*shm_desc_cnt * sizeof(ubs_mem_shm_desc_t)));
    if (!*shm_descs) {
        return UBS_ERR_NULL_POINTER;
    }
    get_all_shm_descs(shm_descs, *shm_desc_cnt);
    return UBS_SUCCESS;
}

int32_t ubs_engine_client_initialize(const char *ubse_uds_path)
{
    if (ubse_uds_path != nullptr && strlen(ubse_uds_path) > MXE_SOCKET_PATH_LEN) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    return UBS_SUCCESS;
}

void ubs_engine_client_finalize() {}

static constexpr const char *NOT_SUPPORTED_TRIGGER = "_NOT_SUPPORTED";

static bool IsNotSupportedName(const char *name)
{
    return name != nullptr && strstr(name, NOT_SUPPORTED_TRIGGER) != nullptr;
}

// 预设节点列表
static ubs_topo_node_t mock_nodes[2];
// 初始化函数
void init_mock_nodes()
{
    // 初始化第一个节点
    mock_nodes[0].slot_id = 1UL;
    mock_nodes[0].socket_id[0] = 0;
    mock_nodes[0].socket_id[1UL] = 1UL;
    strcpy(mock_nodes[0].host_name, "node1");
    // 初始化第二个节点
    mock_nodes[1UL].slot_id = 2UL;
    mock_nodes[1UL].socket_id[0] = 0;
    mock_nodes[1UL].socket_id[1UL] = 2UL;
    strcpy(mock_nodes[1UL].host_name, "node2");
}

int32_t ubs_topo_node_list(ubs_topo_node_t **node_list, uint32_t *node_cnt)
{
    if (!node_list || !node_cnt) {
        return UBS_ERR_NULL_POINTER;
    }
    init_mock_nodes();
    *node_cnt = sizeof(mock_nodes) / sizeof(mock_nodes[0]);
    *node_list = static_cast<ubs_topo_node_t *>(malloc(*node_cnt * sizeof(ubs_topo_node_t)));
    if (!*node_list) {
        return UBS_ERR_NULL_POINTER;
    }
    memcpy(*node_list, mock_nodes, *node_cnt * sizeof(ubs_topo_node_t));
    return UBS_SUCCESS;
}

int32_t ubs_topo_node_local_get(ubs_topo_node_t *node)
{
    if (!node) {
        return UBS_ERR_NULL_POINTER;
    }
    // 模拟当前节点信息
    node->slot_id = 1;
    strncpy(node->host_name, "node1", sizeof(node->host_name));
    node->socket_id[0] = 0;
    node->numa_ids[0][0] = 0;
    node->numa_ids[0][1] = 1;
    node->socket_id[1] = 1;
    node->numa_ids[1][0] = 0;
    node->numa_ids[1][1] = 1;
    return UBS_SUCCESS;
}

// 预设链接列表
static ubs_topo_link_t mock_links[] = {{1, 0, 1, 2, 0, 2}, {2, 0, 2, 1, 0, 1}};

int32_t ubs_topo_link_list(ubs_topo_link_t **node_cpu_links, uint32_t *node_cpu_link_cnt)
{
    if (!node_cpu_links || !node_cpu_link_cnt) {
        return UBS_ERR_NULL_POINTER;
    }

    *node_cpu_link_cnt = sizeof(mock_links) / sizeof(mock_links[0]);
    *node_cpu_links = static_cast<ubs_topo_link_t *>(malloc(*node_cpu_link_cnt * sizeof(ubs_topo_link_t)));
    if (!*node_cpu_links) {
        return UBS_ERR_NULL_POINTER;
    }
    memcpy(*node_cpu_links, mock_links, *node_cpu_link_cnt * sizeof(ubs_topo_link_t));
    return UBS_SUCCESS;
}

// 模拟NUMA内存查询
int32_t ubs_mem_numastat_get(uint32_t slot_id, ubs_mem_numastat_t **numa_mems, uint32_t *numa_mem_cnt)
{
    if (!numa_mems || !numa_mem_cnt)
        return UBS_ERR_NULL_POINTER;
    if (slot_id != 1 && slot_id != 2) {
        return UBS_ENGINE_ERR_CONNECTION_FAILED; // 模拟节点不存在
    }

    *numa_mem_cnt = 1;
    *numa_mems = static_cast<ubs_mem_numastat_t *>(malloc(*numa_mem_cnt * sizeof(ubs_mem_numastat_t)));
    if (!*numa_mems)
        return UBS_ERR_NULL_POINTER;

    (*numa_mems)[0].slot_id = 1UL;
    (*numa_mems)[0].socket_id = 0;
    (*numa_mems)[0].numa_id = 0;
    (*numa_mems)[0].numa_type = NUMA_LOCAL;
    (*numa_mems)[0].mem_lend_ratio = 50UL;
    (*numa_mems)[0].mem_total = 1024UL * 1024UL * 1024UL; // 1GB
    (*numa_mems)[0].mem_free = 512UL * 1024UL * 1024UL;
    (*numa_mems)[0].huge_pages_2M = 1024UL;
    (*numa_mems)[0].free_huge_pages_2M = 512UL;
    (*numa_mems)[0].huge_pages_1G = 1UL;
    (*numa_mems)[0].free_huge_pages_1G = 0;
    (*numa_mems)[0].mem_borrow = 256UL * 1024UL * 1024UL;
    (*numa_mems)[0].mem_lend = 128UL * 1024UL * 1024UL;
    return UBS_SUCCESS;
}

// 获取内存描述信息数量
uint32_t get_mem_desc_count()
{
    return global_shm_desc_count;
}

// 获取所有内存描述信息
void get_all_mem_descs(ubs_mem_fd_desc_t **descs, uint32_t count)
{
    memcpy(*descs, &(global_shm_descs[0]), count * sizeof(ubs_mem_fd_desc_t));
}

// 删除内存描述信息
void delete_mem_desc_by_name(const char *name)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            // 移除该条目
            for (uint32_t j = i; j < global_shm_desc_count - 1; j++) {
                global_shm_descs[j] = global_shm_descs[j + 1];
            }
            global_shm_desc_count--;
            return;
        }
    }
}

// 全局fd内存描述信息列表
typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH + 1];
    mode_t mode;
    ubs_mem_fd_desc_t desc;
} fd_desc_entry_t;

static fd_desc_entry_t global_fd_descs[MXE_DESCS_NUM];
static uint32_t global_fd_desc_count = 0;

// 添加fd_desc到全局列表
void add_fd_desc_to_list(ubs_mem_fd_desc_t *desc)
{
    strncpy(global_fd_descs[global_fd_desc_count].name, desc->name, UBS_MEM_MAX_NAME_LENGTH);
    global_fd_descs[global_fd_desc_count].desc = *desc;
    global_fd_desc_count++;
}

// 检查name是否存在
int is_fd_name_exists(const char *name)
{
    for (uint32_t i = 0; i < global_fd_desc_count; i++) {
        if (strcmp(global_fd_descs[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

// 更新内存描述信息的权限
void update_fd_desc_permission(const char *name, mode_t mode)
{
    for (uint32_t i = 0; i < global_fd_desc_count; i++) {
        if (strcmp(global_fd_descs[i].name, name) == 0) {
            global_fd_descs[i].mode = mode;
            return;
        }
    }
}

// 删除shm_desc
void delete_fd_desc_by_name(const char *name)
{
    for (uint32_t i = 0; i < global_fd_desc_count; i++) {
        if (strcmp(global_fd_descs[i].name, name) == 0) {
            for (uint32_t j = i; j < global_fd_desc_count - 1; j++) {
                global_fd_descs[j] = global_fd_descs[j + 1];
            }
            global_fd_desc_count--;
            return;
        }
    }
}

// 根据name获取内存描述信息
void get_fd_desc_by_name(const char *name, ubs_mem_fd_desc_t *desc)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_fd_descs[i].name, name) == 0) {
            *desc = global_fd_descs[i].desc;
            return;
        }
    }
}

uint32_t get_fd_desc_count()
{
    return global_shm_desc_count;
}

// 获取所有fd_desc
void get_all_fd_descs(ubs_mem_fd_desc_t **descs, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&(*descs)[i], &global_fd_descs[i].desc, sizeof(ubs_mem_fd_desc_t));
    }
}

int32_t ubs_mem_fd_create_with_candidate(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                         const uint32_t *slot_ids, uint32_t slot_cnt, ubs_mem_fd_desc_t *fd_desc)
{
    if (!name || !fd_desc || !slot_ids || slot_cnt == 0) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    // 检查name是否已存在
    if (is_fd_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }

    // 模拟生成fd_desc
    strncpy(fd_desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    fd_desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';

    fd_desc->memid_cnt = 1UL;
    fd_desc->memids[0] = 1003UL; // 假设唯一memid
    fd_desc->mem_size = size;
    fd_desc->unit_size = 1024UL * 1024UL;

    // 设置 export_node
    uint32_t export_slot_id = slot_ids[0];
    const ubs_topo_node_t *export_node = NULL;
    for (size_t i = 0; i < sizeof(mock_nodes) / sizeof(mock_nodes[0]); i++) {
        if (mock_nodes[i].slot_id == export_slot_id) {
            export_node = &mock_nodes[i];
            break;
        }
    }

    if (!export_node) {
        return UBS_ERR_OUT_OF_RANGE; // 模拟无法找到候选节点
    }

    // 设置 export_node 的信息
    fd_desc->export_node.slot_id = export_node->slot_id;
    fd_desc->export_node.socket_id[0] = export_node->socket_id[0];
    fd_desc->export_node.socket_id[1] = export_node->socket_id[1];
    strncpy(fd_desc->export_node.host_name, export_node->host_name, sizeof(fd_desc->export_node.host_name));

    // 设置 import_node
    uint32_t import_slot_id = slot_ids[0]; // 假设 import_node 也是在候选节点里
    const ubs_topo_node_t *import_node = NULL;
    for (size_t i = 0; i < sizeof(mock_nodes) / sizeof(mock_nodes[0]); i++) {
        if (mock_nodes[i].slot_id == import_slot_id) {
            import_node = &mock_nodes[i];
            break;
        }
    }

    if (!import_node) {
        return UBS_ERR_OUT_OF_RANGE; // 模拟无法找到候选节点
    } else {
        fd_desc->import_node.slot_id = import_node->slot_id;
        fd_desc->import_node.socket_id[0] = import_node->socket_id[0];
        fd_desc->import_node.socket_id[1] = import_node->socket_id[1];
        strncpy(fd_desc->import_node.host_name, import_node->host_name, sizeof(fd_desc->import_node.host_name));
    }
    add_fd_desc_to_list(fd_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_fd_create_with_lender(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                      const ubs_mem_lender_t *lender, uint32_t lender_cnt, ubs_mem_fd_desc_t *fd_desc)
{
    if (!name || !lender || !fd_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (lender->lender_size == 0) {
        return UBS_ERR_TIMED_OUT; // size 为0，构造超时场景
    }

    if ((strlen(name) > UBS_MEM_MAX_NAME_LENGTH) || (lender->lender_size < UBS_MEM_MIN_SIZE)) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (is_fd_name_exists(name)) { // 检查name是否已存在
        return UBS_ENGINE_ERR_EXISTED;
    }

    // 模拟生成fd_desc
    strncpy(fd_desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    fd_desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';

    fd_desc->memid_cnt = 1UL;
    fd_desc->memids[0] = 1003UL; // 假设唯一memid
    fd_desc->mem_size = lender->lender_size;
    fd_desc->unit_size = 1024UL * 1024UL;

    // 设置 export_node
    uint32_t export_slot_id = lender->slot_id;
    const ubs_topo_node_t *export_node = NULL;
    for (size_t i = 0; i < sizeof(mock_nodes) / sizeof(mock_nodes[0]); i++) {
        if (mock_nodes[i].slot_id == export_slot_id) {
            export_node = &mock_nodes[i];
            break;
        }
    }

    // 设置 export_node 的信息
    fd_desc->export_node.slot_id = export_node->slot_id;
    fd_desc->export_node.socket_id[0] = export_node->socket_id[0];
    fd_desc->export_node.socket_id[1] = export_node->socket_id[1];
    strncpy(fd_desc->export_node.host_name, export_node->host_name, sizeof(fd_desc->export_node.host_name));

    // 设置 import_node
    uint32_t import_slot_id = lender->slot_id + 1; // 假设 import_node 是export的下个节点
    const ubs_topo_node_t *import_node = NULL;
    for (size_t i = 0; i < sizeof(mock_nodes) / sizeof(mock_nodes[0]); i++) {
        if (mock_nodes[i].slot_id == import_slot_id) {
            import_node = &mock_nodes[i];
            break;
        }
    }

    if (!import_node) {
        return UBS_ERR_OUT_OF_RANGE; // 模拟无法找到候选节点
    } else {
        fd_desc->import_node.slot_id = import_node->slot_id;
        fd_desc->import_node.socket_id[0] = import_node->socket_id[0];
        fd_desc->import_node.socket_id[1] = import_node->socket_id[1];
        strncpy(fd_desc->import_node.host_name, import_node->host_name, sizeof(fd_desc->import_node.host_name));
    }
    add_fd_desc_to_list(fd_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_fd_permission(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode)
{
    if (!name || !owner || mode == 0) {
        return UBS_ERR_NULL_POINTER;
    }
    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (!is_fd_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }
    // 假设权限修改成功
    update_fd_desc_permission(name, mode);
    return UBS_SUCCESS;
}

int32_t ubs_mem_fd_delete(const char *name)
{
    if (!name) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (!is_fd_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }

    // 模拟删除内存描述信息
    delete_fd_desc_by_name(name);
    return UBS_SUCCESS;
}

// 全局numa内存描述信息列表
typedef struct {
    char name[UBS_MEM_MAX_NAME_LENGTH + 1];
    ubs_mem_numa_desc_t desc;
} numa_desc_entry_t;

static numa_desc_entry_t global_numa_descs[MXE_DESCS_NUM];
static uint32_t global_numa_desc_count = 0;

// 添加numa_desc到全局列表
void add_numa_desc_to_list(ubs_mem_numa_desc_t *desc)
{
    strncpy(global_numa_descs[global_numa_desc_count].name, desc->name, UBS_MEM_MAX_NAME_LENGTH);
    global_numa_descs[global_numa_desc_count].desc = *desc;
    global_numa_desc_count++;
}

// 获取numa_desc数量
uint32_t get_numa_desc_count()
{
    return global_numa_desc_count;
}

// 根据name获取numa_desc
void get_numa_desc_by_name(const char *name, ubs_mem_numa_desc_t *desc)
{
    for (uint32_t i = 0; i < global_numa_desc_count; i++) {
        if (strcmp(global_numa_descs[i].name, name) == 0) {
            *desc = global_numa_descs[i].desc;
            return;
        }
    }
}

int is_numa_name_exists(const char *name)
{
    for (uint32_t i = 0; i < global_numa_desc_count; i++) {
        if (strcmp(global_numa_descs[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

// 删除numa_desc
void delete_numa_desc_by_name(const char *name)
{
    for (uint32_t i = 0; i < global_numa_desc_count; i++) {
        if (strcmp(global_numa_descs[i].name, name) == 0) {
            // 移除该条目
            for (uint32_t j = i; j < global_numa_desc_count - 1; j++) {
                global_numa_descs[j] = global_numa_descs[j + 1];
            }
            global_numa_desc_count--;
            return;
        }
    }
}

int32_t ubs_mem_numa_create(const char *name, uint64_t size, ubs_mem_distance_t distance,
                            ubs_mem_numa_desc_t *numa_desc)
{
    if (!name || !numa_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    if (is_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }

    // 模拟numaid生成
    static int64_t next_numaid = 1000L;
    numa_desc->numaid = next_numaid++;

    // 模拟export_node和import_node
    numa_desc->export_node.slot_id = 1UL;
    numa_desc->export_node.socket_id[0] = 0;
    numa_desc->export_node.socket_id[1UL] = 1UL;
    strcpy(numa_desc->export_node.host_name, "export-node");

    numa_desc->import_node.slot_id = 2;
    numa_desc->import_node.socket_id[0] = 0;
    numa_desc->import_node.socket_id[1UL] = 1UL;
    strcpy(numa_desc->import_node.host_name, "import-node");

    // 模拟填充name
    strncpy(numa_desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    numa_desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';

    // 添加到全局列表
    add_numa_desc_to_list(numa_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_numa_create_with_candidate(const char *name, uint64_t size, const uint32_t *slot_ids, uint32_t slot_cnt,
                                           ubs_mem_numa_desc_t *numa_desc)
{
    if (!name || !slot_ids || slot_cnt == 0 || !numa_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (is_numa_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }

    // 模拟numaid生成
    static int64_t next_numaid = 1000L;
    numa_desc->numaid = next_numaid++;

    // 模拟export_node和import_node
    numa_desc->export_node.slot_id = slot_ids[0]; // 选择第一个候选节点
    numa_desc->export_node.socket_id[0] = 0;
    numa_desc->export_node.socket_id[1UL] = 1UL;
    strcpy(numa_desc->export_node.host_name, "export-node");

    numa_desc->import_node.slot_id = 4UL;
    numa_desc->import_node.socket_id[0] = 0;
    strcpy(numa_desc->import_node.host_name, "import-node");

    strncpy(numa_desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    numa_desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';

    // 添加到全局列表
    add_numa_desc_to_list(numa_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_numa_create_with_lender(const char *name, const ubs_mem_lender_t *lender, uint32_t lender_cnt,
                                        ubs_mem_numa_desc_t *numa_desc)
{
    if (!name || !lender || !numa_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (lender->lender_size == 0) {
        // size 为0，构造超时场景
        return UBS_ERR_TIMED_OUT;
    }

    if (lender->lender_size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (is_numa_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }

    // 模拟numaid生成
    static int64_t next_numaid = 1000L;
    numa_desc->numaid = next_numaid++;

    // 模拟export_node和import_node
    numa_desc->export_node.slot_id = lender->slot_id;
    numa_desc->export_node.socket_id[0] = 0;
    numa_desc->export_node.socket_id[1UL] = 1UL;
    strcpy(numa_desc->export_node.host_name, "export-node");

    numa_desc->import_node.slot_id = 2UL;
    numa_desc->import_node.socket_id[0] = 0;
    strcpy(numa_desc->import_node.host_name, "import-node");

    strncpy(numa_desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    numa_desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';

    // 添加到全局列表
    add_numa_desc_to_list(numa_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_numa_get(const char *name, ubs_mem_numa_desc_t *numa_desc)
{
    if (!name || !numa_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (!is_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }

    get_numa_desc_by_name(name, numa_desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_numa_delete(const char *name)
{
    if (!name) {
        return UBS_ERR_NULL_POINTER;
    }
    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }
    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    if (!is_numa_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }
    delete_numa_desc_by_name(name);
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_create(const char *name, uint64_t size, uint8_t usr_info[32], uint64_t flag,
                           const ubs_mem_nodes_t *region, const ubs_mem_nodes_t *provider)
{
    printf("in ubse_mem_shm_create1\n");
    if (!name || !region || !usr_info) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (is_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }
    printf("in ubse_mem_shm_create2\n");
    // 模拟生成共享内存描述信息
    ubs_mem_shm_desc_t *desc = &global_shm_descs[global_shm_desc_count].desc;
    strncpy(desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';
    desc->mem_size = size;
    desc->unit_size = 1024UL * 1024UL;
    desc->export_node.slot_id = 2UL;
    desc->export_node.socket_id[0] = 0;
    desc->export_node.socket_id[1UL] = 1UL;
    strcpy(desc->export_node.host_name, "export-node");

    desc->import_desc_cnt = 1;
    // 分配内存给import_desc
    desc->import_desc = (ubs_mem_shm_import_desc_t *)malloc(sizeof(ubs_mem_shm_import_desc_t) * desc->import_desc_cnt);
    if (!desc->import_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    desc->import_desc[0].memid_cnt = 1UL;
    desc->import_desc[0].memids[0] = 1000UL + global_shm_desc_count;
    desc->import_desc[0].import_node.slot_id = 1UL;
    desc->import_desc[0].import_node.socket_id[0] = 0;
    desc->import_desc[0].import_node.socket_id[1UL] = 1UL;
    strcpy(desc->import_desc[0].import_node.host_name, "import-node");

    memcpy(desc->usr_info, usr_info, UBS_MEM_MAX_USR_INFO_LEN);
    printf("in ubse_mem_shm_create5\n");
    // 添加到全局列表
    add_shm_desc_to_list(desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_create_with_lender(const char *name, uint8_t usr_info[UBS_MEM_MAX_USR_INFO_LEN], uint64_t flag,
                                       const ubs_mem_nodes_t *region, const ubs_mem_lender_t *lender)
{
    if (!name || !usr_info || !lender) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (lender->lender_size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (lender->slot_id > 4) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (lender->socket_id == UINT32_MAX) {
        printf("ubs_mem_shm_create_with_lender, socket_id is UINT32_MAX.\n");
    }

    if (lender->numa_id == UINT32_MAX) {
        printf("ubs_mem_shm_create_with_lender, numa_id is UINT32_MAX.\n");
    }

    if (lender->port_id == UINT32_MAX) {
        printf("ubs_mem_shm_create_with_lender, port_id is UINT32_MAX.\n");
    }

    if (is_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }
    // 模拟生成共享内存描述信息
    ubs_mem_shm_desc_t *desc = &global_shm_descs[global_shm_desc_count].desc;
    strncpy(desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';
    desc->mem_size = lender->lender_size;
    desc->unit_size = 1024UL * 1024UL;
    desc->export_node.slot_id = 2UL;
    desc->export_node.socket_id[0] = 0;
    desc->export_node.socket_id[1UL] = 1UL;
    strcpy(desc->export_node.host_name, "export-node");

    desc->import_desc_cnt = 1;
    // 分配内存给import_desc
    desc->import_desc = (ubs_mem_shm_import_desc_t *)malloc(sizeof(ubs_mem_shm_import_desc_t) * desc->import_desc_cnt);
    if (!desc->import_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    desc->import_desc[0].memid_cnt = 1UL;
    desc->import_desc[0].memids[0] = 1000UL + global_shm_desc_count;
    desc->import_desc[0].import_node.slot_id = 1UL;
    desc->import_desc[0].import_node.socket_id[0] = 0;
    desc->import_desc[0].import_node.socket_id[1UL] = 1UL;
    strcpy(desc->import_desc[0].import_node.host_name, "import-node");

    memcpy(desc->usr_info, usr_info, UBS_MEM_MAX_USR_INFO_LEN);
    printf("in ubs_mem_shm_create_with_lender3\n");
    // 添加到全局列表
    add_shm_desc_to_list(desc);
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_create_with_affinity(const char *name, uint64_t size, uint32_t affinity_socket_id,
                                         uint8_t usr_info[32], uint64_t flag, const ubs_mem_nodes_t *region,
                                         const ubs_mem_nodes_t *provider)
{
    printf("in ubs_mem_shm_create_with_affinity\n");
    if (!name || !region || !usr_info) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (size == 0) { // size 为0，构造超时场景
        return UBS_ERR_TIMED_OUT;
    }

    if (size < UBS_MEM_MIN_SIZE) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (is_name_exists(name)) {
        return UBS_ENGINE_ERR_EXISTED;
    }
    printf("in ubse_mem_shm_create2\n");
    // 模拟生成共享内存描述信息
    ubs_mem_shm_desc_t *desc = &global_shm_descs[global_shm_desc_count].desc;
    strncpy(desc->name, name, UBS_MEM_MAX_NAME_LENGTH);
    desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';
    desc->mem_size = size;
    desc->unit_size = 1024 * 1024;
    desc->export_node.slot_id = 2;
    desc->export_node.socket_id[0] = 0;
    desc->export_node.socket_id[1] = 1;
    strcpy(desc->export_node.host_name, "export-node");

    desc->import_desc_cnt = 1;
    // 分配内存给import_desc
    desc->import_desc = (ubs_mem_shm_import_desc_t *)malloc(sizeof(ubs_mem_shm_import_desc_t) * desc->import_desc_cnt);
    if (!desc->import_desc) {
        return UBS_ERR_NULL_POINTER;
    }

    desc->import_desc[0].memid_cnt = 1;
    desc->import_desc[0].memids[0] = 1000 + global_shm_desc_count;
    desc->import_desc[0].import_node.slot_id = 1;
    desc->import_desc[0].import_node.socket_id[0] = 0;
    desc->import_desc[0].import_node.socket_id[1] = 1;
    strcpy(desc->import_desc[0].import_node.host_name, "import-node");

    memcpy(desc->usr_info, usr_info, UBS_MEM_MAX_USR_INFO_LEN);
    printf("in ubse_mem_shm_create5\n");
    // 添加到全局列表
    add_shm_desc_to_list(desc);
    return UBS_SUCCESS;
}

// 根据name获取shm_desc（只复制基本字段，不包括import_desc）
void get_shm_desc_basic(const char *name, ubs_mem_shm_desc_t *desc)
{
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            // 复制基本字段
            strncpy(desc->name, global_shm_descs[i].desc.name, UBS_MEM_MAX_NAME_LENGTH);
            desc->name[UBS_MEM_MAX_NAME_LENGTH] = '\0';
            desc->mem_size = global_shm_descs[i].desc.mem_size;
            desc->unit_size = global_shm_descs[i].desc.unit_size;
            desc->export_node = global_shm_descs[i].desc.export_node;
            memcpy(desc->usr_info, global_shm_descs[i].desc.usr_info, UBS_MEM_MAX_USR_INFO_LEN);
            return;
        }
    }
}

int32_t ubs_mem_shm_attach(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                           ubs_mem_shm_desc_t **shm_desc)
{
    if (!name || !shm_desc) {
        return UBS_ERR_NULL_POINTER;
    }
    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }
    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    if (!is_name_exists(name)) {
        return UBS_ENGINE_ERR_SHM_NO_CREATE;
    }
    printf("in ubse_mem_shm_attach1\n");
    // 获取匹配的索引和 import_desc_cnt（已知为 1）
    uint32_t found_index = 0;
    bool found = false;
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            found_index = i;
            break;
        }
    }

    // 3. import_desc_cnt 为 1（后续根据控制面的attach接口变化，需要适配）
    const uint32_t import_desc_cnt = 1;
    // 4. 动态扩展内存（固定为 1 个 import_desc）
    size_t total_size = sizeof(ubs_mem_shm_desc_t) + import_desc_cnt * sizeof(ubs_mem_shm_import_desc_t);
    *shm_desc = (ubs_mem_shm_desc_t *)malloc(total_size);
    if (!*shm_desc) {
        free(*shm_desc);
        *shm_desc = NULL;
        return UBS_ERR_NULL_POINTER;
    }
    // 5. 设置 import_desc 指针指向结构体之后的内存区域
    (*shm_desc)->import_desc = (ubs_mem_shm_import_desc_t *)((char *)*shm_desc + sizeof(ubs_mem_shm_desc_t));
    (*shm_desc)->import_desc_cnt = import_desc_cnt;
    // 6. 填充结构体字段
    get_shm_desc_basic(name, *shm_desc);
    // 7. 复制 import_desc 数据（仅 1 个元素）
    if (import_desc_cnt > 0) {
        (*shm_desc)->import_desc[0] = global_shm_descs[found_index].desc.import_desc[0];
    }
    printf("in ubse_mem_shm_attach4\n");
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_get(const char *name, ubs_mem_shm_desc_t **shm_desc)
{
    if (!name || !shm_desc) {
        return UBS_ERR_NULL_POINTER;
    }
    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    if (!is_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }
    printf("in ubse_mem_shm_get\n");
    // 2. 获取 import_desc_cnt 和匹配的索引 i
    uint32_t import_desc_cnt = 0;
    uint32_t found_index = 0; // 保存匹配的索引
    for (uint32_t i = 0; i < global_shm_desc_count; i++) {
        if (strcmp(global_shm_descs[i].name, name) == 0) {
            import_desc_cnt = global_shm_descs[i].desc.import_desc_cnt;
            found_index = i; // 保存匹配的索引
            break;
        }
    }
    // 3. 如果需要分配 import_desc 数组，动态扩展内存
    if (import_desc_cnt > 0) {
        // 计算总内存大小（结构体 + import_desc 数组）
        size_t total_size = sizeof(ubs_mem_shm_desc_t) + import_desc_cnt * sizeof(ubs_mem_shm_import_desc_t);
        *shm_desc = (ubs_mem_shm_desc_t *)malloc(total_size);
        if (!*shm_desc) {
            // 内存分配失败，清理并返回错误
            free(*shm_desc);
            *shm_desc = NULL;
            return UBS_ERR_NULL_POINTER;
        }
        // 设置 import_desc 指针指向结构体之后的内存区域
        (*shm_desc)->import_desc = (ubs_mem_shm_import_desc_t *)((char *)*shm_desc + sizeof(ubs_mem_shm_desc_t));
    }
    // 4. 填充结构体字段
    get_shm_desc_basic(name, *shm_desc);
    // 5. 如果有 import_desc_cnt，复制数据到数组
    if (import_desc_cnt > 0) {
        for (uint32_t j = 0; j < import_desc_cnt; j++) {
            (*shm_desc)->import_desc[j] = global_shm_descs[found_index].desc.import_desc[j];
        }
    }
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_detach(const char *name)
{
    if (!name) {
        return UBS_ERR_NULL_POINTER;
    }
    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }
    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }
    if (!is_name_exists(name)) {
        return UBS_ENGINE_ERR_SHM_NO_ATTACH;
    }
    // 模拟删除标记 应该增加引用计数来模拟attach？
    return UBS_SUCCESS;
}

int32_t ubs_mem_shm_delete(const char *name)
{
    if (!name) {
        return UBS_ERR_NULL_POINTER;
    }

    if (IsNotSupportedName(name)) {
        return UBS_ERR_NOT_SUPPORTED;
    }

    if (strlen(name) > UBS_MEM_MAX_NAME_LENGTH) {
        return UBS_ENGINE_ERR_OUT_OF_RANGE;
    }

    if (!is_name_exists(name)) {
        return UBS_ENGINE_ERR_NOT_EXIST;
    }

    delete_shm_desc_by_name(name);
    return UBS_SUCCESS;
}

// 暂未调用的接口
int32_t ubs_mem_fd_create(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                          ubs_mem_distance_t distance, ubs_mem_fd_desc_t *fd_desc)
{
    return UBS_SUCCESS;
}

int32_t ubs_mem_fd_get(const char *name, ubs_mem_fd_desc_t *fd_desc)
{
    return UBS_SUCCESS;
}

int32_t ubs_mem_fd_list(ubs_mem_fd_desc_t **fd_descs, uint32_t *fd_desc_cnt)
{
    return UBS_SUCCESS;
}

int32_t ubs_mem_numa_list(ubs_mem_numa_desc_t **numa_descs, uint32_t *numa_desc_cnt)
{
    return UBS_SUCCESS;
}

void ubs_engine_log_callback_register(ubs_engine_log_handler handler) {}

int32_t ubs_mem_shm_fault_register(ubs_mem_shm_fault_handler handler)
{
    return UBS_SUCCESS;
}

} // namespace UT
#ifdef __cplusplus
}
#endif