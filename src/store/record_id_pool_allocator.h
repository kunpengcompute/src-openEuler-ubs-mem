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

#ifndef SIMPLE_SAMPLES_RECORD_ID_POOL_ALLOCATOR_H
#define SIMPLE_SAMPLES_RECORD_ID_POOL_ALLOCATOR_H

#include <mutex>
#include <vector>
#include "record_store_def.h"

namespace ock {
namespace ubsm {
class RecordIdPoolAllocator {
public:
    RecordIdPoolAllocator() noexcept;
    ~RecordIdPoolAllocator() noexcept;
    int Initialize(MemIdRecordPool *address) noexcept;
    void Destroy() noexcept;
    int Allocate(const std::vector<uint64_t> &ids, uint32_t &headIndex) noexcept;
    int FillAllocated(uint32_t headIndex, std::vector<uint64_t> &ids) noexcept;
    int Release(uint32_t headIndex) noexcept;

private:
    MemIdRecordPool *memIdRecordPool_;
    std::vector<uint32_t> idleIdIndexes_;
    std::mutex indexMutex_;
};
} // namespace ubsm
} // namespace ock

#endif // SIMPLE_SAMPLES_RECORD_ID_POOL_ALLOCATOR_H
