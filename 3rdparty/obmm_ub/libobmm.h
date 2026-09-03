/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2025. All rights reserved.
 */

#ifndef _OBMM_API_H
#define _OBMM_API_H

#include <stddef.h>
#include <stdint.h>
#include <linux/obmm.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_NUMA_NODES 16

typedef uint64_t mem_id;
#define OBMM_INVALID_MEMID 0

mem_id obmm_export(const size_t length[OBMM_MAX_LOCAL_NUMA_NODES], unsigned long flags, struct obmm_mem_desc* desc);
int obmm_unexport(mem_id id, unsigned long flags);

struct obmm_preimport_info {
    uint64_t pa;
    uint64_t length;
    int base_dist;
    int numa_id;
    uint16_t scna;
    uint16_t dcna;
    /* mar_id, etc */
    uint16_t priv_len;
    uint8_t priv[];
};

int obmm_preimport(struct obmm_preimport_info* preimport_info, unsigned long flags);
int obmm_unpreimport(const struct obmm_preimport_info* preimport_info, unsigned long flags);

/* Export the specified va range of the process pid out of localhost.
 * Due to hardware limitations, during the export process, the corresponding
 * physical memory for the VA (virtual address) range will be allocated and
 * pinned, and the related pages will be checked to see if they are 2M pages.
 *
 * pid: the ID of the process in which va ranged are to exported. If pid is 0,
 * export va range of the calling process.
 **/
mem_id obmm_export_useraddr(int pid, void* va, size_t length, unsigned long flags, struct obmm_mem_desc* desc);

/* TODO @base_dist to uint8_t type*/
mem_id obmm_import(const struct obmm_mem_desc* desc, unsigned long flags, int base_dist, int* numa);
int obmm_unimport(mem_id id, unsigned long flags);

/*
 * Set the ownership (reader, writer, none) of a range of OBMM virtual address space.
 * @fd: The file descriptor of an OBMM memory device.
 * @start: The start virutal address.
 * @end: The end virtual addreses.
 * @prot: The ownership expressed as memory protection bits (PROT_NONE, PROT_READ, PROT_WRITE).
 *        NOTE: PROT_WRITE implies PROT_READ.
 */
int obmm_set_ownership(int fd, void *start, void *end, int prot);

/* debug interface */
int obmm_query_memid_by_pa(unsigned long pa, mem_id* id, unsigned long* offset);
int obmm_query_pa_by_memid(mem_id id, unsigned long offset, unsigned long* pa);

#if defined(__cplusplus)
}
#endif

#endif
