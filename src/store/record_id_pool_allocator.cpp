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
#include "record_id_pool_allocator.h"
#include "log.h"

namespace ock {
namespace ubsm {
union IdHead {
    uint64_t id;
    struct un {
        uint64_t used : 1; // 是否被占用（分配出去）
        uint64_t head : 1; // 分配出去的第一个line
        uint64_t tail : 1; // 分配出去的最后一个line
        uint64_t reserved : 29;
        uint64_t nextIndex : 32; // 非最后一个，表示后一个line的index；最后一个，表示line有中有效个数
    } u;
};

RecordIdPoolAllocator::RecordIdPoolAllocator() noexcept : memIdRecordPool_{nullptr} {}

RecordIdPoolAllocator::~RecordIdPoolAllocator() noexcept
{
    Destroy();
}

int RecordIdPoolAllocator::Initialize(MemIdRecordPool *address) noexcept
{
    if (address == nullptr) {
        DBG_LOGERROR("mem id record pool address is null.");
        return -1;
    }
    memIdRecordPool_ = address;

    IdHead idHead;
    idleIdIndexes_.reserve(RECORD_MEM_ID_POOL_LINE_COUNT);
    for (auto line = 0U; line < RECORD_MEM_ID_POOL_LINE_COUNT; line++) {
        idHead.id = memIdRecordPool_->memIds[line][0];
        if (idHead.u.used == 0UL) {
            idleIdIndexes_.emplace_back(line);
        }
    }

    return 0;
}

void RecordIdPoolAllocator::Destroy() noexcept
{
    idleIdIndexes_.clear();
    memIdRecordPool_ = nullptr;
}

int RecordIdPoolAllocator::Allocate(const std::vector<uint64_t> &ids, uint32_t &headIndex) noexcept
{
    constexpr auto lineSize = RECORD_MEM_ID_POOL_LINE_SIZE - 1U;
    auto idCount = ids.size();
    auto linesNeed = (idCount + lineSize - 1U) / lineSize;
    auto lastLineItems = idCount - (linesNeed - 1U) * lineSize;

    if (memIdRecordPool_ == nullptr) {
        DBG_LOGERROR("Not Initialized!");
        return -1;
    }

    if (linesNeed == 0) {
        DBG_LOGERROR("no id need to allocate.");
        return -1;
    }

    std::vector<uint32_t> allocatedLines(linesNeed);
    std::unique_lock<std::mutex> uniqueLock{indexMutex_};
    if (idleIdIndexes_.size() < linesNeed) {
        uniqueLock.unlock();
        DBG_LOGERROR("allocate need id count: " << idCount << ", not enough, failed.");
        return -1;
    }

    for (auto i = 0U; i < linesNeed; i++) {
        allocatedLines[i] = idleIdIndexes_[idleIdIndexes_.size() - 1 - i];
    }
    idleIdIndexes_.resize(idleIdIndexes_.size() - linesNeed);
    uniqueLock.unlock();

    for (auto i = 0U; i < linesNeed; i++) {
        IdHead idHead;
        idHead.u.used = 1U;
        idHead.u.head = (i == 0U) ? 1U : 0U;
        idHead.u.tail = (i == linesNeed - 1U) ? 1U : 0U;
        idHead.u.reserved = 0U;
        idHead.u.nextIndex = (i == linesNeed - 1U) ? lastLineItems : allocatedLines[i + 1];
        memIdRecordPool_->memIds[allocatedLines[i]][0] = idHead.id;
    }

    for (auto i = 0U; i < ids.size(); i++) {
        auto row = i / lineSize;
        auto col = i % lineSize + 1U;
        memIdRecordPool_->memIds[allocatedLines[row]][col] = ids[i];
    }

    headIndex = allocatedLines[0];
    return 0;
}

int RecordIdPoolAllocator::FillAllocated(uint32_t headIndex, std::vector<uint64_t> &ids) noexcept
{
    ids.clear();
    if (memIdRecordPool_ == nullptr) {
        DBG_LOGERROR("Not Initialized!");
        return -1;
    }

    if (headIndex >= RECORD_MEM_ID_POOL_LINE_COUNT) {
        DBG_LOGERROR("input head index(" << headIndex << ") invalid capacity is " << RECORD_MEM_ID_POOL_LINE_COUNT);
        return -1;
    }

    IdHead idHead;
    idHead.id = memIdRecordPool_->memIds[headIndex][0];
    if (idHead.u.used == 0U) {
        DBG_LOGERROR("input head index(" << headIndex << ") is now idle.");
        return -1;
    }

    if (idHead.u.head == 0U) {
        DBG_LOGERROR("input head index(" << headIndex << ") is not head flag.");
        return -1;
    }

    auto currentIndex = headIndex;
    while (idHead.u.tail == 0U) {
        for (auto i = 1U; i < RECORD_MEM_ID_POOL_LINE_SIZE; i++) {
            ids.emplace_back(memIdRecordPool_->memIds[currentIndex][i]);
        }

        currentIndex = idHead.u.nextIndex;
        idHead.id = memIdRecordPool_->memIds[currentIndex][0];
    }

    for (auto i = 0U; i < idHead.u.nextIndex && i < RECORD_MEM_ID_POOL_LINE_SIZE; i++) {
        ids.emplace_back(memIdRecordPool_->memIds[currentIndex][i + 1]);
    }

    DBG_LOGDEBUG("fill allocated memory ids from head index(" << headIndex << ") count=" << ids.size());
    return 0;
}

int RecordIdPoolAllocator::Release(uint32_t headIndex) noexcept
{
    if (memIdRecordPool_ == nullptr) {
        DBG_LOGERROR("Not Initialized!");
        return -1;
    }

    if (headIndex >= RECORD_MEM_ID_POOL_LINE_COUNT) {
        DBG_LOGERROR("input head index(" << headIndex << ") invalid capacity is " << RECORD_MEM_ID_POOL_LINE_COUNT);
        return -1;
    }

    IdHead idHead;
    idHead.id = memIdRecordPool_->memIds[headIndex][0];
    if (idHead.u.used == 0U) {
        DBG_LOGERROR("input head index(" << headIndex << ") is now idle.");
        return -1;
    }

    if (idHead.u.head == 0U) {
        DBG_LOGERROR("input head index(" << headIndex << ") is not head flag.");
        return -1;
    }

    std::vector<uint32_t> releasedIndexes;
    auto currentIndex = headIndex;
    while (idHead.u.tail == 0U) {
        memIdRecordPool_->memIds[currentIndex][0] = 0UL;
        releasedIndexes.emplace_back(currentIndex);
        currentIndex = idHead.u.nextIndex;
        idHead.id = memIdRecordPool_->memIds[currentIndex][0];
    }
    memIdRecordPool_->memIds[currentIndex][0] = 0UL;
    releasedIndexes.emplace_back(currentIndex);

    std::unique_lock<std::mutex> uniqueLock{indexMutex_};
    idleIdIndexes_.insert(idleIdIndexes_.end(), releasedIndexes.begin(), releasedIndexes.end());
    uniqueLock.unlock();

    DBG_LOGINFO("fill allocated memory ids from head index(" << headIndex << ")");
    return 0;
}
} // namespace ubsm
} // namespace ock