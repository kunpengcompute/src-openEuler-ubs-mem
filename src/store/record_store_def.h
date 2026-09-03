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
#ifndef SIMPLE_SAMPLES_RECORD_STORE_DEF_H
#define SIMPLE_SAMPLES_RECORD_STORE_DEF_H

#include <cstdint>
#include "ubs_common_types.h"
namespace ock {
namespace ubsm {
constexpr uint32_t RECORD_NAME_SIZE = 64;            ///< 各种记录名称的长度，包含'/0'共64
constexpr uint32_t REGION_NODE_BITMAP_COUNT = 1024U; ///< 用于表示region中节点的bitmap位数，可表示0~1023的nodeId
constexpr uint32_t U64_BITS_COUNT = 64U;
constexpr uint32_t REGION_NODE_BITMAP_U64SIZE = REGION_NODE_BITMAP_COUNT / U64_BITS_COUNT;

constexpr uint32_t RECORD_MEM_ID_POOL_LINE_SIZE = 64U; ///< memId pool的一行长度64，有效数据长度为63
constexpr uint32_t RECORD_MEM_ID_POOL_LINE_COUNT = 32U * 1024U; ///< memId pool的行数

constexpr uint32_t CPU_NUM_PER_HOST = 2U;              ///< 每个host两个Cpu
constexpr uint32_t AVAILABLE_MAR = 2U;                 ///< MatrixServer使用2个MAR
constexpr uint32_t DECODER_ENTRY_SIZE = 1024U;         ///< 单个MAR的两张decoder表，合计1024条entry
constexpr uint32_t BLOCK_PER_ENTRY = 4U;               ///< 每个Entry可分为4个block
constexpr uint32_t UMMU_PROTECTION_TABLE_SIZE = 1024U; ///< 每个PROTECTION有1024个表项，对应1024次导出

// 支持最大的导入个数 Cpu * Mar * Decoder * Item
constexpr uint32_t MAX_IMPORT_COUNT = CPU_NUM_PER_HOST * AVAILABLE_MAR * DECODER_ENTRY_SIZE * BLOCK_PER_ENTRY;
// 支持最大的导入个数 Cpu * Protection
constexpr uint32_t MAX_EXPORT_COUNT = CPU_NUM_PER_HOST * UMMU_PROTECTION_TABLE_SIZE;

// 支持最大region数
constexpr uint32_t REGION_MAX_RECORD = 256U;
// 支持最大借用数量，为最大导入次数
constexpr uint32_t MEM_LEASE_MAX_RECORD = MAX_IMPORT_COUNT;
// 本地缓存的借用内存记录个数
constexpr uint32_t MEM_LEASE_MAX_LOCAL_CACHE = 5U;
// 支持共享内存挂载的最大个数 MaxImport + MaxExport
constexpr uint32_t SHM_MAX_ATTACH_RECORD = MAX_IMPORT_COUNT + MAX_EXPORT_COUNT;
// 支持共享内存引用的最大个数，极端场景每个共享内存都有64个进程映射
constexpr uint32_t SHM_MAX_REFERENCE_RECORD = 64U * SHM_MAX_ATTACH_RECORD;

/**
 * 共享内存文件的大小，如果调整上面各个个数常量，此size不足，需要相就增加
 */
constexpr uint32_t SHARE_MEM_SIZE = 40U * 1024U * 1024U;
constexpr uint32_t SHM_PAGE_SIZE = 4096U;
constexpr uint32_t SHM_PAGE_COUNT = SHARE_MEM_SIZE / SHM_PAGE_SIZE;

struct RegionRecord {
    char name[RECORD_NAME_SIZE];
    uint64_t size;
    uint64_t bitmap[REGION_NODE_BITMAP_U64SIZE];
    uint64_t bitmapAffinity[REGION_NODE_BITMAP_U64SIZE];
};

struct MemLeaseRecord {
    char name[RECORD_NAME_SIZE];
    uint64_t size;
    uint32_t pid;
    uint32_t uid;
    uint32_t gid;
    uint8_t fdMode;
    uint8_t distance;
    uint8_t reserved0[2];
    int64_t numaId;
    uint64_t unitSize;
    uint64_t memIdCount;
    uint32_t memIdBuffer;
    uint32_t reserved1;
    uint32_t slotId; // 记录借出内存节点
    RecordState recordState;
};

struct MemShareImportRecord {
    char name[RECORD_NAME_SIZE];
    uint64_t shmSize;
    uint32_t pid;
    uint32_t uid;
    uint32_t gid;
    uint32_t reserved0;
    uint64_t unitSize;
    uint64_t memIdCount;
    uint32_t memIdBuffer;
    uint32_t reserved1;
    uint64_t flag;
    int oflag;
    RecordState recordState;
};

struct MemShareRefRecord {
    uint32_t pid;
    uint32_t shmImportIndex;
};

struct MemIdRecordPool {
    uint64_t memIds[RECORD_MEM_ID_POOL_LINE_COUNT][RECORD_MEM_ID_POOL_LINE_SIZE];
};

} // namespace ubsm
} // namespace ock

#endif // SIMPLE_SAMPLES_RECORD_STORE_DEF_H
