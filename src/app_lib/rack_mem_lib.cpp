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

#include "rack_mem_lib.h"

#include <iostream>

#include "ubs_mem.h"
#include "RmLibObmmExecutor.h"
#include "ShmMetaDataMgr.h"
#include "ipc_proxy.h"
#include "log.h"
#include "mx_def.h"
#include "rack_mem_functions.h"
#include "ubsm_ptracer.h"

void __attribute__((constructor)) InitRackMemLib()
{
    static bool inited = false;
    if (inited) {
        return;
    }
    // 应用使能库Release包不能在控制台打日志，错误日志也不能打，Debug调试可以打日志，已确认
    inited = true;
}

namespace ock::mxmd {
static const auto UBSM_SDK_TRACE_ENABLE = "UBSM_SDK_TRACE_ENABLE";
void RackMemLib::InitModules()
{
    pModules = {{"IPC", [this] { return InitIpc(); }, [this] { return ShutdownIpc(); }, false},
                {"shmShmMetaMgr", [] { return InitShmMetaMgr(); }, [this] { return HOK; }, false},
                {"MemPerfStat", [this] { return InitHtrace(); }, [this] { return ExitHtrace(); }, false},
                {"Obmm", [this] { return RmLibObmmExecutor::GetInstance().Initialize(); },
                 [this] { return RmLibObmmExecutor::GetInstance().Exit(); }, false}};
}

uint32_t RackMemLib::Initialize()
{
    std::lock_guard<std::mutex> lockGuard(lock);
    if (inited) {
        DBG_LOGINFO("The rack_mem has been initialized");
        return UBSM_OK;
    }
    InitModules();
    for (auto &desc : pModules) {
        if (desc.init == nullptr) {
            return MXM_ERR_MEMLIB;
        }
        uint32_t hr = desc.init();
        if (hr == 0) {
            DBG_LOGINFO("Module " << desc.name << " is been initialized successfully");
            desc.isInitialized = true;
        } else {
            DBG_LOGERROR("Failed to initialize module " << desc.name << ", ret=" << hr);
            return MXM_ERR_MEMLIB;
        }
    }
    DBG_LOGINFO("SDK of ubsm has been initialized");
    inited = true;
    return UBSM_OK;
}

void RackMemLib::Destroy()
{
    std::lock_guard<std::mutex> lockGuard(lock);
    for (int index = static_cast<int>(pModules.size()) - 1; index >= 0; index--) {
        RackMemModule &desc = pModules.at(index);
        if (!desc.isInitialized || desc.shutdown == nullptr) {
            continue;
        }
        auto hr = desc.shutdown();
        if (BresultFail(hr)) {
            DBG_LOGERROR("Module " << desc.name << " module shutdown failure");
        }
        desc.isInitialized = false;
    }
    inited = false;
    ubsmem::log::UbsmemLoggerManager::Destroy();
}

uint32_t RackMemLib::InitHtrace() const
{
    auto envValue = std::getenv(UBSM_SDK_TRACE_ENABLE);
    if (envValue == nullptr || strlen(envValue) == 0) {
        return HOK;
    }
    return tracer::UbsmPtracer::Init();
}

uint32_t RackMemLib::ExitHtrace() const
{
    auto envValue = std::getenv(UBSM_SDK_TRACE_ENABLE);
    if (envValue == nullptr || strlen(envValue) == 0) {
        return HOK;
    }
    tracer::UbsmPtracer::UnInit();
    return UBSM_OK;
}

uint32_t RackMemLib::InitIpc()
{
    auto ret = IpcProxy::GetInstance().Initialize();
    return ret;
}

uint32_t RackMemLib::InitShmMetaMgr()
{
    auto ret = ock::mxmd::ShmMetaDataMgr::GetInstance().Init();
    return ret;
}

uint32_t RackMemLib::ShutdownIpc()
{
    DBG_LOGINFO("Start to shutdown ipc");
    IpcProxy::Destroy();
    return UBSM_OK;
}
} // namespace ock::mxmd

#ifdef __cplusplus
extern "C" {
#endif

int ubsmem_init_attributes(ubsmem_options_t *ubsm_shmem_opts)
{
    if (ubsm_shmem_opts == NULL) {
        DBG_LOGERROR("param is null.");
        return UBSM_ERR_PARAM_INVALID;
    }
    return UBSM_OK;
}

int ubsmem_initialize(const ubsmem_options_t *ubsm_shmem_opts)
{
    if (ubsm_shmem_opts == NULL) {
        DBG_LOGERROR("param is null.");
        return UBSM_ERR_PARAM_INVALID;
    }
    auto ret = ock::mxmd::RackMemLib::GetInstance().Initialize();
    return ock::mxmd::GetErrCode(ret);
}

int ubsmem_finalize(void)
{
    ock::mxmd::RackMemLib::GetInstance().Destroy();
    return UBSM_OK;
}

int ubsmem_set_logger_level(int level)
{
    if (level < static_cast<int>(ubsmem::log::UbsmemLogLevel::DEBUG) ||
        level >= static_cast<int>(ubsmem::log::UbsmemLogLevel::COUNT)) {
        DBG_LOGERROR("Level error, level=" << level);
        return UBSM_ERR_PARAM_INVALID;
    }
    ubsmem::log::UbsmemLoggerManager::Instance()->SetLogLevel(static_cast<ubsmem::log::UbsmemLogLevel>(level));
    DBG_LOGINFO("Set log level successfully, level=" << level);
    return UBSM_OK;
}

int ubsmem_set_extern_logger(void (*func)(int level, const char *msg))
{
    if (func == nullptr) {
        DBG_LOGERROR("The function is empty");
        return UBSM_ERR_PARAM_INVALID;
    }
    ubsmem::log::UbsmemLoggerManager::Instance()->SetExternLogCallback(func);
    DBG_LOGINFO("Set extern log successfully");
    return UBSM_OK;
}

#ifdef __cplusplus
} // end of extern "C"
#endif