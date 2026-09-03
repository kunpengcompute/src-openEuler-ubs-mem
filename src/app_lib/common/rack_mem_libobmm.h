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
#ifndef RACK_MEMORYFABRIC_LIBOBMM_H
#define RACK_MEMORYFABRIC_LIBOBMM_H

#include <fcntl.h>
#include <securec.h>
#include <cstdio>
#include <iostream>
#include "ubs_mem_def.h"
#include "libobmm.h"

#define INVALID_MEM_ID 0

inline void ObmmGetDevPath(mem_id id, char *path, size_t pathLen)
{
    int code = sprintf_s(path, pathLen, "/dev/obmm_shmdev%lu", id);
    if (code == -1) {
        path[0] = '\0';
    }
}
#endif