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
#include "RmLibObmmExecutor.h"
#include <cstddef>
#include "dlfcn.h"
#include "log.h"
#include "system_adapter.h"

namespace ock::mxmd {
using namespace ock::common;
using namespace ock::ubsm;
uint32_t RmLibObmmExecutor::Initialize()
{
    static constexpr auto obmmPath = SYS_LIB_DIR "/libobmm.so.1";
    handle = SystemAdapter::DlOpen(obmmPath, RTLD_NOW);
    if (handle == nullptr) {
        DBG_LOGERROR("Failed to dlopen obmm.so, error info=" << dlerror());
        return MXM_ERR_PARAM_INVALID;
    }
    obmmOwnershipSwitch = reinterpret_cast<ObmmOwnershipSwitch>(SystemAdapter::DlSym(handle, "obmm_set_ownership"));
    if (obmmOwnershipSwitch == nullptr) {
        DBG_LOGERROR("Failed to get function obmm_set_ownership from libobmm.so, error=" << dlerror());
        SystemAdapter::DlClose(handle);
        handle = nullptr;
        return MXM_ERR_PARAM_INVALID;
    }
    return HOK;
}

uint32_t RmLibObmmExecutor::Exit()
{
    if (handle != nullptr) {
        SystemAdapter::DlClose(handle);
        handle = nullptr;
    }
    return HOK;
};
} // namespace ock::mxmd