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

#include "system_adapter.h"
#include <numaif.h>
namespace ock::ubsm {
// mmap
void *SystemAdapter::MemoryMap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    return mmap(addr, len, prot, flags, fd, offset);
}

// munmap
int SystemAdapter::MemoryUnMap(void *addr, size_t len)
{
    return munmap(addr, len);
}

// mbind
long SystemAdapter::MemoryBind(void *addr, unsigned long size, int mode, const unsigned long nodemask[],
                               unsigned long maxnode, unsigned int flags)
{
    return mbind(addr, size, mode, nodemask, maxnode, flags);
}

// open
int SystemAdapter::Open(const char *file, int oflag)
{
    return open(file, oflag);
}

// close
int SystemAdapter::Close(int fd)
{
    return close(fd);
}

// dlopen
void *SystemAdapter::DlOpen(const char *file, int mode)
{
    return dlopen(file, mode);
}

// dlsym
void *SystemAdapter::DlSym(void *handle, const char *name)
{
    return dlsym(handle, name);
}

// dlclose
int SystemAdapter::DlClose(void *handle)
{
    return dlclose(handle);
}
} // namespace ock::ubsm
