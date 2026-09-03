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
#ifndef UBSM_THREAD_POOL_H
#define UBSM_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace ock {
namespace ubsm {
class UBSMThreadPool {
public:
    UBSMThreadPool(int thCount, std::string name) noexcept;
    ~UBSMThreadPool() noexcept;

    int Start() noexcept;
    void Push(const std::function<void()> &task) noexcept;
    void Stop() noexcept;
    static UBSMThreadPool &GetInstance() noexcept;

private:
    void ThreadWork(int threadId) noexcept;

private:
    std::atomic<bool> running;
    int threadNum;
    std::vector<std::shared_ptr<std::thread>> threads;
    std::list<std::function<void()>> tasks;
    std::string poolName;
    std::mutex mtx;
    std::condition_variable cond;
};
} // namespace ubsm
} // namespace ock

#endif // UBSM_THREAD_POOL_H
