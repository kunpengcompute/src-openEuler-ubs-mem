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
#ifndef UBS_MEM_MONITOR_H
#define UBS_MEM_MONITOR_H

#include <condition_variable>
#include <list>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "ubs_common_types.h"

namespace ock {
namespace ubsm {
constexpr uint64_t ONE_SECOND_MS = 1000;
constexpr uint64_t ONE_MINUTE_MS = 60UL * ONE_SECOND_MS;     // 1 minute
constexpr uint64_t MAX_DELAY_TIME_MS = 60UL * ONE_MINUTE_MS; // 1 hour
/**
 * @brief 一条需要延时执行的归还内存借用记录，执行失败时加入延时执行
 */
struct DelayRemovedKey {
    bool isLease;
    uint64_t retryTimes;
    uint64_t expiresTime;
    std::string name;
    AppContext appCtx;
    bool changed;
    bool isNumaLease;
    bool timeOutScene;
    bool isAttach = false;
    uint64_t createSeqNo = 0;
    DelayRemovedKey(std::string nm, uint64_t delay, bool isLease, const AppContext &appCtx, bool changed,
                    bool isNumaLease, bool timeScene) noexcept;
    explicit DelayRemovedKey(const std::string &name) : name(name){};
};

/**
 * @brief 用于管理延时还内存借用记录的比较器
 */
struct DelayKeyComparator {
    bool operator()(const DelayRemovedKey &k1, const DelayRemovedKey &k2) const noexcept;
};

struct DelayKeyIsBusyComparator {
    bool operator()(const DelayRemovedKey &k1, const DelayRemovedKey &k2) const noexcept;
};

class UBSMemMonitor {
public:
    static UBSMemMonitor &GetInstance() noexcept;

    ~UBSMemMonitor() = default;

    int Initialize(uint64_t delayMs = 0) noexcept;
    void Destroy() noexcept;
    void AddDelayRemoveRecord(const DelayRemovedKey &key);
    bool GetDelayRemoveRecord(const DelayRemovedKey &key);

private:
    static uint32_t RegisterHandler(pid_t pid) noexcept;
    UBSMemMonitor() noexcept;
    uint32_t ManagerEventNotified(pid_t pid) noexcept;
    void BackgroundProcess() noexcept;
    void RemoveImmediatelyBorrows() noexcept;
    void ClientProcessExited(pid_t pid) noexcept;
    void RemoveDelayBorrows() noexcept;

    uint64_t firstDelayMs;
    bool running;
    std::thread bgThread;
    std::recursive_mutex mutex;
    std::condition_variable_any cond;

    std::list<uint32_t> exitedPids;
    std::set<DelayRemovedKey, DelayKeyComparator> delayedRemoveKeys;
    std::set<DelayRemovedKey, DelayKeyIsBusyComparator> delayedRemoveKeysByName;
};
} // namespace ubsm
} // namespace ock
#endif // UBS_MEM_MONITOR_H
