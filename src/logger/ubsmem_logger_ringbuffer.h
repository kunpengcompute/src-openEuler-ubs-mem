/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef UBSMEM_LOGGER_BUFFER_H
#define UBSMEM_LOGGER_BUFFER_H

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "ubsmem_logger_entry.h"

namespace ubsmem::log {
class UbsmemLoggerRingBuffer {
public:
    explicit UbsmemLoggerRingBuffer(uint32_t size);

    ~UbsmemLoggerRingBuffer();

    bool IsEmpty() const;

    void Push(UbsmemLoggerEntry &&loggerEntry, bool &dropped);

    void Pop(UbsmemLoggerEntry &loggerEntry);

    UbsmemLoggerRingBuffer(UbsmemLoggerRingBuffer const &) = delete;
    UbsmemLoggerRingBuffer &operator=(UbsmemLoggerRingBuffer const &) = delete;

public:
    uint32_t size_;
    std::vector<UbsmemLoggerEntry> buffer_{};
    std::atomic<uint32_t> left_{};
    std::atomic<uint32_t> right_{};
    std::atomic<bool> bufferFullWarned_{false};
};

class UbsmemLoggerDoubleBuffer {
public:
    explicit UbsmemLoggerDoubleBuffer(uint32_t size) : readBuffer_(size), writeBuffer_(size) {}

    void Push(UbsmemLoggerEntry &&loggerEntry, bool &dropped);
    bool Pop(UbsmemLoggerEntry &loggerEntry);
    void Swap();

public:
    std::shared_mutex mtx_{};
    bool stop_ = false;

private:
    UbsmemLoggerRingBuffer readBuffer_;
    UbsmemLoggerRingBuffer writeBuffer_;
};
} // namespace ubsmem::log
#endif // UBSMEM_LOGGER_BUFFER_H
