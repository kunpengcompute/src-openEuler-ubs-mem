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
#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>

#include "log.h"
#include "ubsm_thread_pool.h"

const static int DEFAULT_THREAD_NUM = 6;

using namespace ock::ubsm;

UBSMThreadPool::UBSMThreadPool(int thCount, std::string name) noexcept
    : running(false),
      threadNum{thCount},
      poolName{std::move(name)}
{
}

UBSMThreadPool::~UBSMThreadPool() noexcept
{
    if (running) {
        Stop();
    }
}

int UBSMThreadPool::Start() noexcept
{
    int result = 0;
    running.store(true);
    for (int i = 0; i < threadNum; ++i) {
        try {
            threads.push_back(std::make_shared<std::thread>(&UBSMThreadPool::ThreadWork, this, i));
        } catch (std::exception &ex) {
            result = -1;
            DBG_LOGERROR("Exception:" << ex.what());
        } catch (...) {
            result = -1;
            DBG_LOGERROR("Uncatch Exception");
        }
    }
    return result;
}

void UBSMThreadPool::Push(const std::function<void()> &task) noexcept
{
    std::unique_lock<std::mutex> lock(mtx);
    tasks.push_back(task);
    lock.unlock();
    cond.notify_one();
}

void UBSMThreadPool::Stop() noexcept
{
    running.store(false);
    cond.notify_all();
    for (auto &th : threads) {
        th->join();
    }
}

UBSMThreadPool &UBSMThreadPool::GetInstance() noexcept
{
    static UBSMThreadPool instance(DEFAULT_THREAD_NUM, "ubsm_thread");
    return instance;
}

void UBSMThreadPool::ThreadWork(int threadId) noexcept
{
    std::string name = poolName + "_" + std::to_string(threadId);
    std::chrono::seconds waitTime(2);
    int ret = 0;
    if ((ret = pthread_setname_np(pthread_self(), name.c_str())) != 0) {
        DBG_LOGWARN("Failed to set name for thread, name=" << name << "error=" << strerror(ret));
    }
    DBG_LOGINFO("New worker, name=" << name);
    while (running.load()) {
        std::function<void()> task;
        std::unique_lock<std::mutex> lock(mtx);
        while (tasks.empty() && running) {
            cond.wait_for(lock, waitTime);
        }

        if (!tasks.empty()) {
            task = tasks.front();
            tasks.pop_front();
        }
        lock.unlock();

        if (task) {
            task();
        }
    }
}