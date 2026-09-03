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
#include "ubs_mem_monitor.h"
#include <iosfwd>
#include <iostream>
#include "defines.h"
#include "log.h"
#include "mls_manager.h"
#include "mxm_ipc_server_interface.h"
#include "record_store.h"
#include "ubs_mem_leak_cleaner.h"
#include "ubse_mem_adapter.h"

namespace ock {
namespace ubsm {
using namespace lease::service;
DelayRemovedKey::DelayRemovedKey(std::string nm, uint64_t delay, bool isLease, const AppContext &appCtx, bool changed,
                                 bool isNumaLease, bool isTimeOutScene) noexcept
    : isLease(isLease),
      retryTimes{0},
      expiresTime{0},
      name{std::move(nm)},
      appCtx(appCtx),
      changed(changed),
      isNumaLease(isNumaLease),
      timeOutScene(isTimeOutScene)
{
    auto nowMilli = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    expiresTime = nowMilli.time_since_epoch().count() + delay;
}

bool DelayKeyComparator::operator()(const DelayRemovedKey &k1, const DelayRemovedKey &k2) const noexcept
{
    if (k1.expiresTime != k2.expiresTime) {
        return k1.expiresTime < k2.expiresTime;
    }

    return k1.name < k2.name;
}

bool DelayKeyIsBusyComparator::operator()(const DelayRemovedKey &k1, const DelayRemovedKey &k2) const noexcept
{
    return k1.name < k2.name;
}

UBSMemMonitor::UBSMemMonitor() noexcept : firstDelayMs{0UL}, running{false} {}
UBSMemMonitor &UBSMemMonitor::GetInstance() noexcept
{
    static UBSMemMonitor instance;
    return instance;
}

uint32_t UBSMemMonitor::RegisterHandler(pid_t pid) noexcept
{
    return UBSMemMonitor::GetInstance().ManagerEventNotified(pid);
}

int UBSMemMonitor::Initialize(uint64_t delayMs) noexcept
{
    std::unique_lock<std::recursive_mutex> lock{mutex};
    if (running) {
        DBG_LOGDEBUG("monitor already initialized.");
        return 0;
    }

    ock::com::ipc::MXMSetLinkEventHandler(RegisterHandler);

    auto ret = UBSMemLeakCleaner::GetInstance().Start();
    if (ret != 0) {
        DBG_LOGERROR("Start UBSMemLeakCleaner fail. ret=" << ret);
        return ret;
    }

    running = true;
    lock.unlock();

    firstDelayMs = delayMs == 0UL ? ONE_MINUTE_MS : delayMs;
    bgThread = std::thread{[this]() {
        BackgroundProcess();
    }};
    return 0;
}

void UBSMemMonitor::Destroy() noexcept
{
    DBG_LOGINFO("borrow memory monitor has been exiting.");
    std::unique_lock<std::recursive_mutex> lock{mutex};
    if (!running) {
        DBG_LOGINFO("monitor not initialized.");
        return;
    }

    running = false;
    lock.unlock();
    cond.notify_one();
    DBG_LOGINFO("wait for monitor thread.");
    if (bgThread.joinable()) {
        bgThread.join();
    }
    DBG_LOGINFO("borrow memory monitor has been exited.");
}

uint32_t UBSMemMonitor::ManagerEventNotified(pid_t pid) noexcept
{
    DBG_LOGINFO("ManagerEventNotified happened, exit pid=" << pid);
    std::unique_lock<std::recursive_mutex> lock{mutex};
    exitedPids.push_back(pid);
    lock.unlock();
    cond.notify_one();
    return 0U;
}

void UBSMemMonitor::BackgroundProcess() noexcept
{
    std::unique_lock<std::recursive_mutex> lock{mutex};

    while (running) {
        if (exitedPids.empty() && delayedRemoveKeys.empty()) {
            DBG_LOGDEBUG("exitedPids.empty() && delayedRemoveKeys.empty().");
            cond.wait_for(lock, std::chrono::milliseconds(firstDelayMs));
        }

        auto nowMilli = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
        auto nowTime = nowMilli.time_since_epoch().count();
        if (!delayedRemoveKeys.empty() && delayedRemoveKeys.begin()->expiresTime > nowTime) {
            auto waitTime = delayedRemoveKeys.begin()->expiresTime - nowTime;
            DBG_LOGDEBUG("BackgroundProcess check for name(" << delayedRemoveKeys.begin()->name << ", expires time("
                                                             << delayedRemoveKeys.begin()->expiresTime << ") nowtime("
                                                             << nowTime << ", wait for(" << waitTime << ").");
            cond.wait_for(lock, std::chrono::milliseconds(waitTime));
        }

        if (!running) {
            break;
        }
        lock.unlock();

        RemoveImmediatelyBorrows();
        RemoveDelayBorrows();

        lock.lock();
    }
}

void UBSMemMonitor::RemoveImmediatelyBorrows() noexcept
{
    std::list<uint32_t> pids;
    std::unique_lock<std::recursive_mutex> lock{mutex};
    if (!running) {
        return;
    }

    pids.swap(exitedPids);
    lock.unlock();

    for (auto pid : pids) {
        ClientProcessExited(pid);
    }

    auto ret = UBSMemLeakCleaner::GetInstance().Start();
    if (ret != UBSM_OK) {
        DBG_LOGWARN("Back ground clear dead process memory failed. ret: " << ret);
    }
}

void UBSMemMonitor::ClientProcessExited(pid_t pid) noexcept
{
    DBG_LOGINFO("Start to clean leak memory allocated by " << pid);
    auto leaseRecord = MLSManager::GetInstance().ListAllMem();
    for (const auto &record : leaseRecord) {
        if (record.appContext.pid != pid) {
            continue;
        }
        DelayRemovedKey removedKey{record.name, 0, true, record.appContext, false, record.isNuma, false};
        if (!UBSMemMonitor::GetInstance().GetDelayRemoveRecord(removedKey)) {
            AddDelayRemoveRecord(removedKey);
        }
    }
    auto ret = UBSMemLeakCleaner::GetInstance().SHMProcessDeadProcess(pid);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("SHMProcessDeadProcess error. ret: " << ret);
    }
    DBG_LOGINFO("Clean leak memory allocated by (" << pid << ") end.");
}

static uint64_t LinearDelayMs(uint64_t retryTimes, uint64_t stepMs, uint64_t maxDelayMs)
{
    auto nowMilli = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    auto nowTime = nowMilli.time_since_epoch().count();
    uint64_t delay = nowTime;
    if (retryTimes > 0) {
        if (stepMs > (std::numeric_limits<uint64_t>::max() / retryTimes)) {
            delay = std::numeric_limits<uint64_t>::max();
        } else {
            uint64_t stepProduct = stepMs * retryTimes;
            if (delay > (std::numeric_limits<uint64_t>::max() - stepProduct)) {
                delay = std::numeric_limits<uint64_t>::max();
            } else {
                delay += stepProduct;
            }
        }
    }
    return std::min(delay, maxDelayMs + nowTime);
}

void UBSMemMonitor::RemoveDelayBorrows() noexcept
{
    auto nowMilli = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    auto nowTime = nowMilli.time_since_epoch().count();
    std::list<DelayRemovedKey> expiresRemovedKeys;

    mutex.lock();
    if (!running || delayedRemoveKeys.empty()) {
        mutex.unlock();
        return;
    }

    for (auto item : delayedRemoveKeys) {
        if (item.expiresTime <= nowTime) {
            DBG_LOGDEBUG("Find expires name(" << item.name << ", expires time(" << item.expiresTime << ") nowtime("
                                              << nowTime << ").");
            expiresRemovedKeys.push_back(item);
        }
    }
    mutex.unlock();

    std::list<DelayRemovedKey> failedRetryKeys;
    for (auto &removedKey : expiresRemovedKeys) {
        removedKey.retryTimes++;
        HRESULT ret = HOK;
        DBG_LOGINFO("RemoveDelayBorrows for name(" << removedKey.name << ") retry(" << removedKey.retryTimes
                                                   << "), expires time(" << removedKey.expiresTime << "), nowtime("
                                                   << nowTime << ")");
        if (removedKey.isLease) {
            ret = UBSMemLeakCleaner::GetInstance().CleanLeaseMemoryLeakInner(
                removedKey.name, removedKey.appCtx, removedKey.changed, removedKey.timeOutScene,
                removedKey.isNumaLease);
        } else {
            ret = UBSMemLeakCleaner::GetInstance().CleanShareMemoryLeakInner(
                removedKey.name, removedKey.appCtx, removedKey.timeOutScene, removedKey.isAttach,
                removedKey.createSeqNo);
        }

        mutex.lock();
        auto waitToErase = delayedRemoveKeysByName.find(removedKey);
        if (waitToErase != delayedRemoveKeysByName.end()) {
            delayedRemoveKeysByName.erase(waitToErase);
        }
        delayedRemoveKeys.erase(removedKey);
        mutex.unlock();

        if (ret != 0) {
            removedKey.expiresTime = LinearDelayMs(removedKey.retryTimes, ONE_MINUTE_MS, MAX_DELAY_TIME_MS);
            DBG_LOGWARN("RemoveDelayBorrows for name(" << removedKey.name << ") retry(" << removedKey.retryTimes
                                                       << "), expires time(" << removedKey.expiresTime << "), nowtime("
                                                       << nowTime << "), failed: errCode=" << ret);
            failedRetryKeys.push_back(std::move(removedKey));
        }
    }

    for (auto &failedKey : failedRetryKeys) {
        AddDelayRemoveRecord(failedKey);
    }
}

void UBSMemMonitor::AddDelayRemoveRecord(const DelayRemovedKey &key)
{
    std::unique_lock<std::recursive_mutex> lock{mutex};
    delayedRemoveKeys.insert(key);
    delayedRemoveKeysByName.insert(key);
}

bool UBSMemMonitor::GetDelayRemoveRecord(const DelayRemovedKey &key)
{
    std::unique_lock<std::recursive_mutex> lock{mutex};
    auto checkKey = delayedRemoveKeysByName.find(key);
    if (checkKey != delayedRemoveKeysByName.end()) {
        return true;
    }
    return false;
}

} // namespace ubsm
} // namespace ock
