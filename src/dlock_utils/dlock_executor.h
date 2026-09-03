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
#ifndef UBSM_DLOCK_DLOCK_EXECUTOR_H
#define UBSM_DLOCK_DLOCK_EXECUTOR_H

#include "dlock_common.h"
#include "dlock_types.h"
#include "log.h"
#include "rack_mem_err.h"

namespace ock {
namespace dlock_utils {

constexpr auto DLOCKS_PATH = SYS_LIB_DIR "/libdlocks.so.0";
constexpr auto DLOCKC_PATH = SYS_LIB_DIR "/libdlockc.so.0";

using DLockSeverLibInitPtr = int (*)(unsigned int max_server_num);
using DLockSeverLibDeInitPtr = void (*)();
using DLockServerStartPtr = int (*)(const struct dlock::server_cfg &cfg, int &serverId);
using DLockServerStopPtr = int (*)(int server_id);

using DLockClientLibInitPtr = int (*)(const struct dlock::client_cfg *pClientCfg);
using DLockClientLibDeinitPtr = void (*)();
using DLockClientInitPtr = int (*)(int *pClientId, const char *ipStr);
using DLockClientReinitPtr = int (*)(int clientId, const char *ipStr);
using DLockClientDeInitPtr = int (*)(int clientId);
using DLockClientReinitDonePtr = int (*)(int clientId);
using DLockClientHeartbeatPtr = int (*)(int clientId, unsigned int timeout);
using DLockClientGetLockPtr = int (*)(int clientId, const struct dlock::lock_desc *desc, int *lockId);
using DLockReleaseLockPtr = int (*)(int clientId, int lockId);
using DLockUpdateAllLocksPtr = int (*)(int clientId);
using DLockGetClientDebugStatsPtr = int (*)(int clientId, struct dlock::debug_stats *stats);
using DLockTryLockPtr = int (*)(int clientId, const struct dlock::lock_request *req, void *result);
using DLockLockPtr = int (*)(int clientId, const struct dlock::lock_request *req, void *result);
using DLockUnlockPtr = int (*)(int clientId, int lockId, void *result);
using DLockLockExtendPtr = int (*)(int clientId, const struct dlock::lock_request *req, void *result);
using DLockLockRequestAsyncPtr = int (*)(int clientId, const struct dlock::lock_request *req);
using DLockLockResultCheckPtr = int (*)(int clientId, void *result);

class DLockExecutor {
public:
    DLockSeverLibInitPtr DLockServerLibInitFunc;
    DLockSeverLibDeInitPtr DLockSeverLibDeInitFunc;
    DLockServerStartPtr DLockServerStartFunc;
    DLockServerStopPtr DLockServerStopFunc;
    DLockClientLibInitPtr DLockClientLibInitFunc;
    DLockClientLibDeinitPtr DLockClientLibDeInitFunc;
    DLockClientInitPtr DLockClientInitFunc;
    DLockClientReinitPtr DLockClientReinitFunc;
    DLockClientDeInitPtr DLockClientDeInitFunc;
    DLockClientReinitDonePtr DLockClientReinitDoneFunc;
    DLockClientHeartbeatPtr DLockClientHeartbeatFunc;
    DLockClientGetLockPtr DLockClientGetLockFunc;
    DLockReleaseLockPtr DLockReleaseLockFunc;
    DLockUpdateAllLocksPtr DLockUpdateAllLocksFunc;
    DLockGetClientDebugStatsPtr DLockGetClientDebugStatsFunc;
    DLockTryLockPtr DLockTryLockFunc;
    DLockLockPtr DLockLockFunc;
    DLockUnlockPtr DLockUnlockFunc;
    DLockLockExtendPtr DLockLockExtendFunc;
    DLockLockRequestAsyncPtr DLockLockRequestAsyncFunc;
    DLockLockResultCheckPtr DLockLockResultCheckFunc;

    static int ClientInitWrapper(int *clientId, const char *serverIp);

    static int ClientReinitWrapper(int clientId, const char *serverIp);

    static int ServerStartWrapper(const struct dlock::server_cfg &cfg, int &serverId);

    static DLockExecutor &GetInstance()
    {
        static DLockExecutor instance;
        return instance;
    }

    int32_t InitDLockDlopenLib();

    int32_t InitDLockClientDlopenLib();

    int32_t InitDLockClientLockInitLib();

    int32_t InitDLockClientLockGetLib();

    int32_t InitDLockClientLockMgmtLib();

    int32_t InitDLockServerDlopenLib();

    void DestroyDLockDlopenLib();

private:
    bool isInitialized = false;
    void *serverHandle = nullptr;
    void *clientHandle = nullptr;
};
} // namespace dlock_utils
} // namespace ock
#endif // UBSM_DLOCK_DLOCK_EXECUTOR_H
