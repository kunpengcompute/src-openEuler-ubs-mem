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

#include "dlock_executor.h"
#include <dlfcn.h>
#include "ubs_mem_def.h"
#include "system_adapter.h"

using namespace ock::dlock_utils;
using namespace ock::common;
using namespace ock::ubsm;

int32_t DLockExecutor::InitDLockServerDlopenLib()
{
    serverHandle = SystemAdapter::DlOpen(DLOCKS_PATH, RTLD_LAZY | RTLD_GLOBAL);
    const char *dlError = dlerror();
    if (serverHandle == nullptr) {
        DBG_LOGERROR("Open dlock server lib error is " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockServerLibInitFunc =
        reinterpret_cast<DLockSeverLibInitPtr>(SystemAdapter::DlSym(serverHandle, "dserver_lib_init"));
    if (DLockServerLibInitFunc == nullptr) {
        dlError = dlerror();
        DBG_LOGERROR("Not find function dserver_lib_init: " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockSeverLibDeInitFunc =
        reinterpret_cast<DLockSeverLibDeInitPtr>(SystemAdapter::DlSym(serverHandle, "dserver_lib_deinit"));
    if (DLockSeverLibDeInitFunc == nullptr) {
        dlError = dlerror();
        DBG_LOGERROR("Not find function dserver_lib_deinit: " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockServerStartFunc = reinterpret_cast<DLockServerStartPtr>(SystemAdapter::DlSym(serverHandle, "server_start"));
    if (DLockServerStartFunc == nullptr) {
        dlError = dlerror();
        DBG_LOGERROR("Not find function server_start: " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockServerStopFunc = reinterpret_cast<DLockServerStopPtr>(SystemAdapter::DlSym(serverHandle, "server_stop"));
    if (DLockServerStopFunc == nullptr) {
        dlError = dlerror();
        DBG_LOGERROR("Not find function server_stop: " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("DLock server library initialization completed successfully");
    return UBSM_OK;
}

int32_t DLockExecutor::InitDLockClientLockInitLib()
{
    DLockClientLibInitFunc =
        reinterpret_cast<DLockClientLibInitPtr>(SystemAdapter::DlSym(clientHandle, "dclient_lib_init"));
    const char *dlsymError = dlerror();
    if (DLockClientLibInitFunc == nullptr) {
        DBG_LOGERROR("Not find function dclient_lib_init: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientLibDeInitFunc =
        reinterpret_cast<DLockClientLibDeinitPtr>(SystemAdapter::DlSym(clientHandle, "dclient_lib_deinit"));
    if (DLockClientLibDeInitFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function dclient_lib_deinit: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientInitFunc = reinterpret_cast<DLockClientInitPtr>(SystemAdapter::DlSym(clientHandle, "client_init"));
    if (DLockClientInitFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function client_init: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientReinitFunc = reinterpret_cast<DLockClientReinitPtr>(SystemAdapter::DlSym(clientHandle, "client_reinit"));
    if (DLockClientReinitFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function client_reinit: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientDeInitFunc = reinterpret_cast<DLockClientDeInitPtr>(SystemAdapter::DlSym(clientHandle, "client_deinit"));
    if (DLockClientDeInitFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function client_deinit: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientReinitDoneFunc =
        reinterpret_cast<DLockClientReinitDonePtr>(SystemAdapter::DlSym(clientHandle, "client_reinit_done"));
    if (DLockClientReinitDoneFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function client_reinit_done: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockClientHeartbeatFunc =
        reinterpret_cast<DLockClientHeartbeatPtr>(SystemAdapter::DlSym(clientHandle, "client_heartbeat"));
    if (DLockClientHeartbeatFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function client_heartbeat: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("DLock client library init functions loaded successfully");
    return UBSM_OK;
}

int32_t DLockExecutor::InitDLockClientLockGetLib()
{
    DLockClientGetLockFunc = reinterpret_cast<DLockClientGetLockPtr>(SystemAdapter::DlSym(clientHandle, "get_lock"));
    const char *dlsymError = dlerror();
    if (DLockClientGetLockFunc == nullptr) {
        DBG_LOGERROR("Not find function get_lock: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockReleaseLockFunc = reinterpret_cast<DLockReleaseLockPtr>(SystemAdapter::DlSym(clientHandle, "release_lock"));
    if (DLockReleaseLockFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function release_lock: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockTryLockFunc = reinterpret_cast<DLockTryLockPtr>(SystemAdapter::DlSym(clientHandle, "trylock"));
    if (DLockTryLockFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function trylock: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockLockFunc = reinterpret_cast<DLockLockPtr>(SystemAdapter::DlSym(clientHandle, "lock"));
    if (DLockLockFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function lock: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockUnlockFunc = reinterpret_cast<DLockUnlockPtr>(SystemAdapter::DlSym(clientHandle, "unlock"));
    if (DLockUnlockFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function unlock: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockLockExtendFunc = reinterpret_cast<DLockLockExtendPtr>(SystemAdapter::DlSym(clientHandle, "lock_extend"));
    if (DLockLockExtendFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function lock_extend: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("DLock client lock get functions loaded successfully");
    return UBSM_OK;
}

int32_t DLockExecutor::InitDLockClientLockMgmtLib()
{
    DLockUpdateAllLocksFunc =
        reinterpret_cast<DLockUpdateAllLocksPtr>(SystemAdapter::DlSym(clientHandle, "update_all_locks"));
    const char *dlsymError = dlerror();
    if (DLockUpdateAllLocksFunc == nullptr) {
        DBG_LOGERROR("Not find function update_all_locks: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockLockRequestAsyncFunc =
        reinterpret_cast<DLockLockRequestAsyncPtr>(SystemAdapter::DlSym(clientHandle, "lock_request_async"));
    if (DLockLockRequestAsyncFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function lock_request_async: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockLockResultCheckFunc =
        reinterpret_cast<DLockLockResultCheckPtr>(SystemAdapter::DlSym(clientHandle, "lock_result_check"));
    if (DLockLockResultCheckFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function lock_result_check: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }

    DLockGetClientDebugStatsFunc =
        reinterpret_cast<DLockGetClientDebugStatsPtr>(SystemAdapter::DlSym(clientHandle, "get_client_debug_stats"));
    if (DLockGetClientDebugStatsFunc == nullptr) {
        dlsymError = dlerror();
        DBG_LOGERROR("Not find function get_client_debug_stats: " << dlsymError);
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("DLock client lock management functions loaded successfully");
    return UBSM_OK;
}

int32_t DLockExecutor::InitDLockClientDlopenLib()
{
    clientHandle = SystemAdapter::DlOpen(DLOCKC_PATH, RTLD_LAZY | RTLD_GLOBAL);
    if (clientHandle == nullptr) {
        const char *dlError = dlerror();
        DBG_LOGERROR("Open dlock client lib error: " << dlError);
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("Successfully opened dlock client lib: " << DLOCKC_PATH);
    if (InitDLockClientLockInitLib() != UBSM_OK) {
        return MXM_ERR_DLOCK_LIB;
    }
    if (InitDLockClientLockGetLib() != UBSM_OK) {
        return MXM_ERR_DLOCK_LIB;
    }
    if (InitDLockClientLockMgmtLib() != UBSM_OK) {
        return MXM_ERR_DLOCK_LIB;
    }
    DBG_LOGINFO("DLock client library initialization completed successfully");
    return UBSM_OK;
}

int32_t DLockExecutor::InitDLockDlopenLib()
{
    if (isInitialized) {
        return UBSM_OK;
    }
    if (InitDLockServerDlopenLib() != UBSM_OK) {
        return MXM_ERR_DLOCK_LIB;
    }
    if (InitDLockClientDlopenLib() != UBSM_OK) {
        return MXM_ERR_DLOCK_LIB;
    }
    isInitialized = true;
    return UBSM_OK;
}

void DLockExecutor::DestroyDLockDlopenLib()
{
    if (serverHandle != nullptr) {
        SystemAdapter::DlClose(serverHandle);
        serverHandle = nullptr;
        DBG_LOGINFO("Closed dlock server library");
    }
    if (clientHandle != nullptr) {
        SystemAdapter::DlClose(clientHandle);
        clientHandle = nullptr;
        DBG_LOGINFO("Closed dlock client library");
    }
    isInitialized = false;
}

int DLockExecutor::ClientInitWrapper(int *clientId, const char *serverIp)
{
    return GetInstance().DLockClientInitFunc(clientId, serverIp);
}

int DLockExecutor::ClientReinitWrapper(int clientId, const char *serverIp)
{
    return GetInstance().DLockClientReinitFunc(clientId, serverIp);
}

int DLockExecutor::ServerStartWrapper(const struct dlock::server_cfg &cfg, int &serverId)
{
    return GetInstance().DLockServerStartFunc(cfg, serverId);
}