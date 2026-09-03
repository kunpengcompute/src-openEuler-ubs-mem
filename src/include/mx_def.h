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

#ifndef MX_DEF_H
#define MX_DEF_H

#ifdef __cplusplus
extern "C" {
#endif
typedef enum RackMemPerfLevel {
    L0, /* *L0对应直连节点 */
    L1, /* *L1对应通过1跳节点，暂不支持 */
    L2  /* *L2对应过超过1跳节点 ，暂不支持 */
} PerfLevel;

struct NodeBorrowMemInfo {
    uint64_t availBorrowMemSize; // 当前可借入内存的大小，单位字节
    uint64_t borrowedMemSize;    // 已经借入的内存大小，单位字节
    uint64_t availLendMemSize;   // 当前可借出内存的大小，单位字节
    uint64_t lentMemSize;        // 已经借出的内存大小，单位字节
};

typedef struct NodeBorrowMemInfo QueryLocalBorrowMemInfo;

#define MEM_TOPOLOGY_MAX_HOSTS 16
#define MEM_MAX_ID_LENGTH 48 // 节点id，共享内存name，最大限制1024

#ifdef __cplusplus
}
#endif
#endif // MX_DEF_H
