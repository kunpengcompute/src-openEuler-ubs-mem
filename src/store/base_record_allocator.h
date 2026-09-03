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
#ifndef SIMPLE_SAMPLES_BASE_RECORD_ALLOCATOR_H
#define SIMPLE_SAMPLES_BASE_RECORD_ALLOCATOR_H

#include <functional>
#include <mutex>
#include <vector>
#include "record_store_def.h"

namespace ock {
namespace ubsm {

template <class RecordType>
class BaseRecordAllocator {
public:
    BaseRecordAllocator(RecordType *address, uint32_t count, const std::function<bool(const RecordType &)> &idleFun,
                        const std::function<void(RecordType &)> &clearFun) noexcept
        : recordAddress_{address},
          recordCount_{count},
          checkIdleFunc_{idleFun},
          clearRecordFun_{clearFun}
    {
    }

    std::vector<RecordType *> Restore()
    {
        std::vector<RecordType *> allocatedRecords;
        allocatedRecords.reserve(recordCount_);
        idleIndexes_.reserve(recordCount_);

        std::unique_lock<std::mutex> uniqueLock{recordMutex_};
        for (auto i = 0U; i < recordCount_; i++) {
            if (checkIdleFunc_(recordAddress_[i])) {
                idleIndexes_.emplace_back(i);
            } else {
                allocatedRecords.emplace_back(recordAddress_ + i);
            }
        }

        return allocatedRecords;
    }

    RecordType *Allocate() noexcept
    {
        std::unique_lock<std::mutex> uniqueLock{recordMutex_};
        if (idleIndexes_.empty()) {
            return nullptr;
        }

        auto index = idleIndexes_[idleIndexes_.size() - 1U];
        idleIndexes_.pop_back();
        uniqueLock.unlock();

        return recordAddress_ + index;
    }

    void Release(RecordType *record) noexcept
    {
        auto index = record - recordAddress_;
        if (index >= recordCount_) {
            return;
        }

        clearRecordFun_(*record);
        std::unique_lock<std::mutex> uniqueLock{recordMutex_};
        idleIndexes_.emplace_back(index);
    }

private:
    RecordType *const recordAddress_;
    const uint32_t recordCount_;
    const std::function<bool(const RecordType &)> checkIdleFunc_;
    const std::function<void(RecordType &)> clearRecordFun_;
    std::mutex recordMutex_;
    std::vector<uint32_t> idleIndexes_;
};
} // namespace ubsm
} // namespace ock

#endif // SIMPLE_SAMPLES_BASE_RECORD_ALLOCATOR_H
