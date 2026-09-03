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

#ifndef UBSM_DLOCK_DLOCK_CONTEXT_H
#define UBSM_DLOCK_DLOCK_CONTEXT_H

#include <time/dg_monotonic.h>
#include <atomic>
#include <cstdint>
#include "ubs_mem_def.h"
#include "client_desc.h"
#include "dlock_common.h"
#include "dlock_config.h"
#include "log.h"

namespace ock {
namespace dlock_utils {

class DLockContext {
public:
    static constexpr int32_t NOT_FOUND = -1;

public:
    DLockContext(const DLockContext &other) = delete;
    DLockContext(DLockContext &&other) = delete;
    auto operator=(const DLockContext &other) -> DLockContext & = delete;
    auto operator=(DLockContext &&other) -> DLockContext & = delete;
    ~DLockContext();

    static auto Instance() -> DLockContext &
    {
        static DLockContext ctx;
        return ctx;
    };

    auto GetConfig() -> DLockConfig &
    {
        return cfg;
    };

    auto GetConfig() const -> const DLockConfig &
    {
        return cfg;
    };

    int32_t InitDlockClient();
    void UnInitDlockClient();

    bool IsNeedServerDeinit()
    {
        return isNeedServerDeinit;
    };

    void SetServerDeinitFlag(bool flag)
    {
        isNeedServerDeinit = flag;
    };

    bool IsHeartBeatActive() const
    {
        return isHeartBeatActive;
    }

    void SetHeartBeatStatus(bool status)
    {
        isHeartBeatActive = status;
    }

    ClientDesc *GetDlockClient()
    {
        // client负载均衡
        size_t index = clientIndex.fetch_add(1, std::memory_order_relaxed);
        index %= cfg.dlockClientNum;
        if (index >= dlockClients.size()) {
            DBG_LOGERROR("Failed to get dlock client, an unexpected error, index: " << index);
            return nullptr;
        }
        return dlockClients.at(index);
    };

    const std::vector<ClientDesc *> &GetDlockClientList()
    {
        return dlockClients;
    };

    bool ContainClientDesc(const std::string &name) const
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        return metaMap.find(name) != metaMap.end();
    }

    ClientDesc *GetClientDesc(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        auto client = metaMap.find(name);
        if (client == metaMap.end()) {
            DBG_LOGINFO("GetClientId not find lockId of name: " << name);
            return nullptr;
        }
        return client->second;
    }

    void SetClientDesc(const std::string &name, ClientDesc *meta)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        metaMap[name] = meta;
    }

    void RemoveClientDesc(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        metaMap.erase(name);
    }

    void AddClientDescRef(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        ++refCount[name];
    }

    int32_t GetClientDescRef(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        auto it = refCount.find(name);
        if (it == refCount.end()) {
            return NOT_FOUND;
        }
        return refCount[name];
    }

    int32_t ReleaseClientDescRef(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        auto it = refCount.find(name);
        if (it == refCount.end()) {
            return NOT_FOUND;
        }
        if (--(it->second) == 0) {
            refCount.erase(it);
            return 0;
        }
        return it->second;
    }

    void RemoveClientDescRef(const std::string &name)
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        auto it = refCount.find(name);
        if (it != refCount.end()) {
            refCount.erase(it);
        }
    }

    std::vector<std::string> GetAllShmLockName()
    {
        std::lock_guard<std::mutex> guard(metaMutex);
        std::vector<std::string> result;
        for (const auto &entry : metaMap) {
            const auto &name = entry.first;
            result.emplace_back(name);
        }
        return result;
    }

private:
    DLockContext() = default;

private:
    std::atomic<size_t> clientIndex = {0};
    bool isNeedServerDeinit = false;
    bool isHeartBeatActive = true;
    DLockConfig cfg;
    std::vector<int32_t> index2ClientId;
    std::vector<ClientDesc *> dlockClients;
    mutable std::mutex metaMutex;
    std::unordered_map<std::string, ClientDesc *> metaMap;
    std::unordered_map<std::string, uint32_t> refCount;
};
} // namespace dlock_utils
} // namespace ock
#endif // UBSM_DLOCK_DLOCK_CONTEXT_H