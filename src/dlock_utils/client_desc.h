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

#ifndef UBSM_CLIENT_DESC_H
#define UBSM_CLIENT_DESC_H

#include <pthread.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>
#include "comm_def.h"
#include "time/dg_monotonic.h"

namespace ock {
namespace dlock_utils {
constexpr auto INVALID = -1;

struct LockUdsInfo {
    uint32_t pid;
    uint32_t uid;
    uint32_t gid;
    unsigned long validTime;

    // 重载 < 运算符，用于 std::set 的唯一性校验
    bool operator<(const LockUdsInfo &other) const
    {
        if (pid != other.pid) {
            return pid < other.pid;
        }
        if (uid != other.uid) {
            return uid < other.uid;
        }
        return gid < other.gid;
    }
};

class ClientDesc {
public:
    explicit ClientDesc(int32_t clientId) : clientId_(clientId)
    {
        pthread_rwlock_init(&clientLock, nullptr);
    }

    ~ClientDesc() = default;

    void Lock()
    {
        pthread_rwlock_wrlock(&clientLock);
    }

    void Unlock()
    {
        pthread_rwlock_unlock(&clientLock);
    }

    inline int32_t GetClientId() const
    {
        return clientId_;
    }

    inline void SetClientId(int32_t clientId)
    {
        clientId_ = clientId;
    }

    bool Contains(const std::string &name) const
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        return name2LockId.find(name) != name2LockId.end();
    }

    inline std::pair<bool, int32_t> GetLockId(const std::string &name) const
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        auto it = name2LockId.find(name);
        if (it != name2LockId.end()) {
            return {true, it->second};
        }
        return {false, INVALID};
    }

    inline void RemoveLockId(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        name2LockId.erase(name);
    }

    inline void SetLockId(const std::string &name, int32_t lockId)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        name2LockId[name] = lockId;
    }

    inline void SetLockUdsInfo(const std::string &name, const LockUdsInfo &udsInfo)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        name2UdsInfo[name].insert(udsInfo);
    }

    inline void RemoveLockUdsInfoByName(const std::string &name, const LockUdsInfo &udsInfo)
    {
        std::lock_guard<std::mutex> guard(dataMutex);

        auto it = name2UdsInfo.find(name);
        if (it == name2UdsInfo.end()) {
            DBG_LOGERROR("Failed to find udsInfo by name: " << name);
            return;
        }

        auto &udsSet = it->second;
        auto found = udsSet.find(udsInfo);
        if (found != udsSet.end()) {
            DBG_LOGINFO("RemoveLockUdsInfoByName udsInfo by name: " << name << " pid: " << udsInfo.pid << " uid: "
                                                                    << udsInfo.uid << " gid: " << udsInfo.gid);
            udsSet.erase(found);
            return;
        }
    }

    inline void RemoveLockUdsInfo(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        name2UdsInfo.erase(name);
    }

    inline bool IsLockUdsValid(const std::string &name, const LockUdsInfo &udsInfo)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        auto it = name2UdsInfo.find(name);
        if (it == name2UdsInfo.end()) {
            DBG_LOGERROR("Failed to find udsInfo by name: " << name);
            return false;
        }

        auto found = it->second.find(udsInfo);
        if (found == it->second.end()) {
            DBG_LOGERROR("Failed to find udsInfo by name: " << name << " pid: " << udsInfo.pid
                                                            << " uid: " << udsInfo.uid << " gid: " << udsInfo.gid);
            return false;
        }
        return true;
    }

    inline bool IsLockInValidTime(const std::string &name, const LockUdsInfo &udsInfo)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        auto it = name2UdsInfo.find(name);
        if (it == name2UdsInfo.end()) {
            DBG_LOGERROR("Failed to find udsInfo by name: " << name);
            return false;
        }

        auto found = it->second.find(udsInfo);
        if (found == it->second.end()) {
            DBG_LOGWARN("Failed to find udsInfo by name: " << name);
            return false;
        }
        auto lockExpiryTime = found->validTime;
        auto timeNow = ock::dagger::Monotonic::TimeUs();
        DBG_LOGINFO("IsLockInValidTime timeNow: " << timeNow << " validTime: " << lockExpiryTime);
        return lockExpiryTime > timeNow;
    }

    inline std::vector<LockUdsInfo> GetInValidUdsInfo(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        std::vector<LockUdsInfo> result;
        auto it = name2UdsInfo.find(name);
        if (it == name2UdsInfo.end()) {
            DBG_LOGDEBUG("Not to find udsInfo by name: " << name);
            return result;
        }
        auto timeNow = dagger::Monotonic::TimeUs();
        for (const auto &udsInfo : it->second) {
            if (udsInfo.validTime <= timeNow) {
                result.emplace_back(udsInfo);
            }
        }
        DBG_LOGINFO("name: " << name << " has num: " << result.size() << " invalid udsInfo.");
        return result;
    }

    inline bool IsLockInValidTime(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(dataMutex);
        auto it = name2UdsInfo.find(name);
        if (it == name2UdsInfo.end()) {
            DBG_LOGWARN("Failed to find udsInfo by name: " << name);
            return true; // 没有相关锁信息，返回 true
        }
        auto timeNow = dagger::Monotonic::TimeUs();
        for (const auto &udsInfo : it->second) {
            if (udsInfo.validTime > timeNow) {
                DBG_LOGDEBUG("Has validTime lock with name: " << name);
                return true; // 存在未过期的锁，返回 true
            }
        }
        DBG_LOGINFO("All lock is invalid with name: " << name);
        return false; // 所有锁都已过期，返回 false
    }

private:
    std::unordered_map<std::string, int32_t> name2LockId;
    std::unordered_map<std::string, std::set<LockUdsInfo>> name2UdsInfo;
    int32_t clientId_;
    pthread_rwlock_t clientLock{};
    mutable std::mutex dataMutex;
};
} // namespace dlock_utils
} // namespace ock
#endif