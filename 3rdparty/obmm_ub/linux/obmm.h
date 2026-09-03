// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 */

#ifndef UAPI_OBMM_H
#define UAPI_OBMM_H

#include <linux/types.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct obmm_mem_desc {
    uint64_t addr;
    uint64_t length;
    uint32_t tokenid;
    uint16_t scna;
    uint16_t dcna;
    uint8_t cacheable;
    uint16_t priv_len;
    uint8_t priv[];
} __attribute__((aligned(8)));

#define OBMM_MAX_LOCAL_NUMA_NODES 16
#define MAX_NUMA_DIST 254
#define OBMM_MAX_PRIV_LEN 512

#define OBMM_MMAP_FLAG_HUGETLB_PMD 1UL << 63

#define OBMM_EXPORT_FLAG_ALLOW_MMAP 0x1UL
#define OBMM_EXPORT_FLAG_FAST 0x2UL
#define OBMM_EXPORT_FLAG_MASK (OBMM_EXPORT_FLAG_ALLOW_MMAP | OBMM_EXPORT_FLAG_FAST)

struct obmm_cmd_export_pid {
    void* va;  // in
    uint64_t length;  // in
    uint64_t flags;  // in
    int32_t pid;  // in
    uint32_t tokenid;  // out
    uint64_t uba;  // out
    uint64_t mem_id;  // out
    uint32_t priv_len;  // in
    void* priv;  // in
} __attribute__((aligned(8)));

/* For ordinary register requests, @length and @flags are input arguments while
 * @tokenid, @uba and @mem_id are values set by obmm kernel module. For
 * register request, @length, @flags, @tokenid and @uba are input to obmm
 * kernel module. @mem_id is the only output. */
struct obmm_cmd_export {
    uint64_t size[OBMM_MAX_LOCAL_NUMA_NODES];
    uint64_t length;
    uint64_t flags;
    uint32_t cacheable : 1;
    uint32_t priv_len;
    const void* priv;
    uint32_t tokenid;
    uint64_t uba;
    uint64_t mem_id;
} __attribute__((aligned(8)));

#define OBMM_UNEXPORT_FLAG_MASK (0UL)

struct obmm_cmd_unexport {
    uint64_t mem_id;
    uint64_t flags;
} __attribute__((aligned(8)));

enum obmm_query_key_type { OBMM_QUERY_BY_PA, OBMM_QUERY_BY_ID_OFFSET };

struct obmm_cmd_addr_query {
    /* key type decides the input and output */
    enum obmm_query_key_type key_type;
    uint64_t mem_id;
    uint64_t offset;
    uint64_t pa;
} __attribute__((aligned(8)));

#define OBMM_IMPORT_FLAG_ALLOW_MMAP 0x1UL
#define OBMM_IMPORT_FLAG_PREIMPORT 0x2UL
#define OBMM_IMPORT_FLAG_NUMA_REMOTE 0x4UL
#define OBMM_IMPORT_FLAG_MASK (OBMM_IMPORT_FLAG_ALLOW_MMAP | OBMM_IMPORT_FLAG_PREIMPORT | OBMM_IMPORT_FLAG_NUMA_REMOTE)

struct obmm_cmd_import {
    const void* priv; /* Input */
    uint64_t flags; /* Input */
    uint8_t base_dist; /* Input */
    uint64_t mem_id; /* Output */
    int32_t numa_id; /* Input/Output */
    struct obmm_mem_desc desc; /* Input */
} __attribute__((aligned(8)));

#define OBMM_UNIMPORT_FLAG_MASK (0UL)

struct obmm_cmd_unimport {
    uint64_t mem_id;
    uint64_t flags;
} __attribute__((aligned(8)));

#define OBMM_CMD_EXPORT _IO('x', 0)
#define OBMM_CMD_IMPORT _IO('x', 1)
#define OBMM_CMD_UNEXPORT _IO('x', 2)
#define OBMM_CMD_UNIMPORT _IO('x', 3)
#define OBMM_CMD_ADDR_QUERY _IO('x', 4)
#define OBMM_CMD_EXPORT_PID _IO('x', 5)
#define OBMM_CMD_DECLARE_PREIMPORT _IO('x', 6)
#define OBMM_CMD_UNDECLARE_PREIMPORT _IO('x', 7)

/* 2bits */
#define OBMM_SHM_MEM_CACHE_RESV     0x0
#define OBMM_SHM_MEM_NORMAL 0x1
#define OBMM_SHM_MEM_NORMAL_NC 0x2
#define OBMM_SHM_MEM_DEVICE 0x3
#define OBMM_SHM_MEM_CACHE_MASK 0b11
/* 2bits */
#define OBMM_SHM_MEM_READONLY 0x0
#define OBMM_SHM_MEM_READEXEC 0x4
#define OBMM_SHM_MEM_READWRITE 0x8
#define OBMM_SHM_MEM_NO_ACCESS 0xc
#define OBMM_SHM_MEM_ACCESS_MASK 0b1100

/* cache maintainence operations (not states) */
/* no cache maintainence (nops) */
#define OBMM_SHM_CACHE_NONE 0x0
/* invalidate only (in-cache modifications may not be written back to DRAM) */
#define OBMM_SHM_CACHE_INVAL 0x1
/* write back and invalidate */
#define OBMM_SHM_CACHE_WB_INVAL 0x2
/* Automatically choose the cache maintainence action depending on the memory
 * state. The resulting choice always make sure no data would be lost, and might
 * be more conservative than necessary. */
#define OBMM_SHM_CACHE_INFER 0x3

struct obmm_cmd_update_range {
    /* address range to manipulate: [start, end) */
    uint64_t start;
    uint64_t end;
    uint8_t mem_state;
    uint8_t cache_ops;
} __attribute__((aligned(8)));

#define OBMM_SHMDEV_UPDATE_RANGE _IO('X', 0)

struct obmm_cmd_preimport {
    uint64_t pa;
    uint64_t length;
    uint8_t base_dist;
    int numa_id;
    uint16_t scna;
    uint16_t dcna;
    /* mar_id, etc */
    uint16_t priv_len;
    const void* priv;
    uint64_t flags;
} __attribute__((aligned(16), packed));

#define OBMM_PREIMPORT_FLAG_MASK (0UL)
#define OBMM_UNPREIMPORT_FLAG_MASK (0UL)

#if defined(__cplusplus)
}
#endif

#endif /* UAPI_OBMM_H */
