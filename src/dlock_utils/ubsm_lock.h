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

#ifndef UBSM_DLOCK_LOCK_H
#define UBSM_DLOCK_LOCK_H

#include <mutex>
#include "client_desc.h"
#include "dlock_common.h"
#include "dlock_context.h"
#include "dlock_executor.h"
#include "ubs_cryptor_handler.h"
#include "zen_discovery.h"

namespace ock {
namespace dlock_utils {
class UbsmLock {
public:
    static auto Instance() -> UbsmLock &
    {
        static UbsmLock ubsmLock;
        return ubsmLock;
    };

    UbsmLock(const UbsmLock &other) = delete;
    UbsmLock(UbsmLock &&other) = delete;
    auto operator=(const UbsmLock &other) -> UbsmLock & = delete;
    auto operator=(UbsmLock &&other) -> UbsmLock & = delete;

    int32_t Init();
    int32_t DeInit();
    int32_t Reinit();
    int32_t DlockServerInit(struct dlock::ssl_cfg ssl);
    int32_t DlockClientLibInit(const struct dlock::ssl_cfg &ssl);
    int32_t Lock(const std::string &name, bool isExclusive, LockUdsInfo &udsInfo); // try lock
    int32_t Unlock(const std::string &name, const LockUdsInfo &udsInfo);
    int32_t UnlockWithDesc(const std::string &name, ClientDesc *clientDesc, const LockUdsInfo &udsInfo);
    int32_t Heartbeat();
    void DeinitTlsConfig();
    bool IsUbsmLockInit() const
    {
        std::lock_guard<std::mutex> lock(initMutex);
        return isUbsmLockInit;
    }

    void UbsmLockInitSet(bool value)
    {
        std::lock_guard<std::mutex> lock(initMutex);
        DBG_LOGINFO("Set UbsmLockInitSet from=" << isUbsmLockInit << ", to=" << value);
        isUbsmLockInit = value;
    }

    [[nodiscard]] bool IsUbsmLockInElection() const
    {
        ock::zendiscovery::ZenDiscovery *zenDiscovery = ock::zendiscovery::ZenDiscovery::GetInstance();
        if (zenDiscovery == nullptr) {
            DBG_LOGERROR("zenDiscovery is nullptr, init first.");
            return true;
        }
        auto electionProcess = zenDiscovery->GetElectionInProgress();
        DBG_LOGINFO("zenDiscovery in election process=" << electionProcess);
        if (electionProcess) {
            DBG_LOGWARN("zenDiscovery in election process, not ready for lock ops.");
            return true;
        }
        return false;
    }

private:
    static std::atomic<bool> isCleanupThreadRunning; // 标记是否已启动线程
    static std::thread cleanupThread;                // 后台清理线程
    static std::mutex cleanupMutex;                  // 保护线程状态的互斥锁
    static void CleanupExpiredLocksThread();
    static void CleanUpExpiredLocksForName(const std::string &name, ClientDesc *clientDesc);
    static std::mutex initMutex;

private:
    int32_t HandleLock(const std::string &name, bool isExclusive, LockUdsInfo &udsInfo); // try lock
    int32_t HandleUnlock(const std::string &name, const LockUdsInfo &udsInfo);
    ClientDesc *GetLock(const std::string &name, const LockUdsInfo &udsInfo);
    int32_t TryLock(const std::string &name, ClientDesc *clientDesc, bool isExclusive, LockUdsInfo &udsInfo);
    int32_t DoUnlock(int32_t lockId, int32_t clientId);

    int32_t TryReleaseLock(const std::string &name, int32_t lockId);
    int32_t DeInitDlockServer();
    UbsmLock() = default;
    ~UbsmLock();

    int32_t SetEid(dlock::dlock_eid_t *eid, std::string &devEid);
    int32_t DlockServerReinit(const std::string &serverIp, struct dlock::ssl_cfg ssl);
    int32_t DlockClientReinit(int32_t clientId, const std::string &serverIp);
    void DoClientReInitStagesClientReInit(int32_t &ret, bool &skipUpdate, int32_t clientId, REINIT_STAGES &stages);
    int32_t ClientReInitStagesClientReInit(int32_t clientId, const char *serverIp, uint32_t &retryCount);
    int32_t ClientReInitStagesUpdateLocks(int32_t clientId, int32_t &updateRetryTimes, REINIT_STAGES &stages);
    int32_t ClientReInitStagesClientReInitDone(int32_t clientId, REINIT_STAGES &stages);
    dlock::primary_cfg GetPrimCfg(const std::string &serverIp, DLockContext &ctx);
    int32_t GetServerCfg(const dlock::ssl_cfg &ssl, const dlock::primary_cfg &primCfg, dlock::server_cfg &conf);
    int32_t InitTlsConfig(struct dlock::ssl_cfg &conf);
    int32_t InitializeTlsPaths();
    int32_t GetClientNode(std::string &clientNodeId);
    bool isUbsmLockInit{false};
    struct dlock::ssl_cfg tlsConfig = {0};
};

} // namespace dlock_utils
} // namespace ock
#endif // UBSM_DLOCK_LOCK_H