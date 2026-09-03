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

#ifndef SYSTEM_ADAPTER_H
#define SYSTEM_ADAPTER_H

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
namespace ock::ubsm {
class SystemAdapter {
public:
    SystemAdapter() = delete;

    // mmap
    static void *MemoryMap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

    // munmap
    static int MemoryUnMap(void *addr, size_t len);

    // mbind
    static long MemoryBind(void *addr, unsigned long size, int mode, const unsigned long nodemask[],
                           unsigned long maxnode, unsigned int flags);

    // open
    static int Open(const char *file, int oflag);

    // close
    static int Close(int fd);

    // dlopen
    static void *DlOpen(const char *file, int mode);

    // dlsym
    static void *DlSym(void *handle, const char *name);

    // dlclose
    static int DlClose(void *handle);
};
} // namespace ock::ubsm

#endif // SYSTEM_ADAPTER_H
