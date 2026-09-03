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

#ifndef MEMORYFABRIC_RACK_MEM_LIB_COMMON_H
#define MEMORYFABRIC_RACK_MEM_LIB_COMMON_H

#include <fcntl.h>
#include <cstddef>
#include "RmLibObmmExecutor.h"
#include "log.h"
#include "rack_mem_libobmm.h"

namespace ock::mxmd {

constexpr size_t RACK_MEM_PAGE_SIZE = 4 * 1024 * 1024;

constexpr int INVALID_MEM_FD = -1;
constexpr uint64_t INVALID_MEM_SIZE = 0;

inline bool CheckRackMemSize(const size_t shmSize)
{
    if (shmSize <= 0) {
        DBG_LOGERROR("The size " << shmSize << " less than 0.");
        return false;
    }
    if (shmSize % RACK_MEM_PAGE_SIZE != 0) {
        DBG_LOGERROR("The size " << shmSize << " does not align with " << RACK_MEM_PAGE_SIZE);
        return false;
    }
    return true;
}

/**
 * 根据提供的内存ID，使用OBMM打开内存，并返回文件描述符。
 * @param id 要打开的内存ID。
 * @return 成功时返回文件描述符，失败时返回负值。
 */
inline int ObmmOpenInternal(const mem_id id, int flags, int oflag)
{
    DBG_LOGINFO("ObmmOpenInternal oflag:" << oflag << ", openflag " << flags);
    int openFlag = 0;
    if (((flags & UBSM_FLAG_NONCACHE) == UBSM_FLAG_NONCACHE) ||
        ((flags & UBSM_FLAG_ONLY_IMPORT_NONCACHE) == UBSM_FLAG_ONLY_IMPORT_NONCACHE)) {
        openFlag = O_SYNC;
    }
    char path[MAX_OBMM_SHMDEV_PATH_LEN]{'\0'};
    ObmmGetDevPath(id, path, sizeof(path));
    const auto fd = open(path, openFlag | oflag);
    if (fd < 0) {
        const char *errorMsg = strerror(errno);
        DBG_LOGERROR("Obmm_open error! memid:" << id << " fd:" << fd << " error: " << errorMsg);
    } else {
        DBG_LOGINFO("Obmm_open ok memid:" << id << " fd:" << fd);
    }
    return fd;
}
} // namespace ock::mxmd
#endif // MEMORYFABRIC_RACK_MEM_LIB_COMMON_H