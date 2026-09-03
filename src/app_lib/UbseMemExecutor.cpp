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
#include "UbseMemExecutor.h"
#include "defines.h"
#include "dlfcn.h"
#include "rack_mem_err.h"
#include "system_adapter.h"
namespace ock::mxmd {
using namespace ock::common;
using namespace ock::ubsm;
ShmFaultFuncPtr UbseMemExecutor::ShmFaultFunc = nullptr;
uint32_t UbseMemExecutor::Initialize()
{
    if (initialized_) {
        return HOK;
    }
    static constexpr auto soPath = ARCH_LIB_DIR "/libubse-client.so.1";
    handle = SystemAdapter::DlOpen(soPath, RTLD_NOW);
    if (handle == nullptr) {
        DBG_LOGERROR("Dlopen libubse-client.so.1 failed, error is " << dlerror());
        return MXM_ERR_UBSE_LIB;
    }
    UbseLogCallBackRegisterFunc =
        (UbseLogCallBackRegisterPtr)SystemAdapter::DlSym(handle, "ubs_engine_log_callback_register");
    if (UbseLogCallBackRegisterFunc == nullptr) {
        DBG_LOGWARN("Symbol ubse_log_callback_register is not found , reason is" << dlerror());
    } else {
        auto logFunc = ubsmem::log::UbsmemLoggerManager::Instance()->GetExternLogCallback();
        if (logFunc == nullptr) {
            DBG_LOGWARN("The user has not set an external log function");
        } else {
            UbseLogCallBackRegisterFunc(reinterpret_cast<ubs_engine_log_handler>(logFunc));
        }
    }

    UbseFaultEventRegisterFunc = (UbseFaultEventRegisterPtr)SystemAdapter::DlSym(handle, "ubs_mem_shm_fault_register");
    if (UbseFaultEventRegisterFunc == nullptr) {
        DBG_LOGERROR("Failed to call dlsym to load ubs_mem_shm_fault_register, error is " << dlerror());
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }
    initialized_ = true;
    return HOK;
}

uint32_t UbseMemExecutor::Exit()
{
    if (handle != nullptr) {
        SystemAdapter::DlClose(handle);
        handle = nullptr;
        UbseFaultEventRegisterFunc = nullptr;
    }
    initialized_ = false;
    return HOK;
}

int32_t UbseMemExecutor::UbseShmFaultHandlerFunc(const char *name, uint64_t memid, ubs_mem_fault_type_t type)
{
    if (name == nullptr || ShmFaultFunc == nullptr) {
        DBG_LOGERROR("name or registerFunc is nullptr");
        return MXM_ERR_PARAM_INVALID;
    }
    return ShmFaultFunc(name, static_cast<ubsmem_fault_type_t>(type));
}

int32_t UbseMemExecutor::RegisterShmFaultEvent(ShmFaultFuncPtr registerFunc)
{
    if (!initialized_) {
        DBG_LOGERROR("UbseMemExecutor not initialized");
        return -1;
    }
    ShmFaultFunc = registerFunc;
    auto ret = UbseFaultEventRegisterFunc(UbseShmFaultHandlerFunc);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("register shm fault event failed ret is " << ret);
        return MXM_ERR_UBSE_INNER;
    }
    return HOK;
}
} // namespace ock::mxmd