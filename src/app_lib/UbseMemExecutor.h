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

#ifndef UBSEMEMEXECUTOR_H
#define UBSEMEMEXECUTOR_H
#include <cstdint>
#include "ubs_mem_def.h"
#include "log.h"
#include "ubs_engine_log.h"
#include "ubs_engine_mem.h"
#include "ubs_engine_topo.h"
#include "ubs_error.h"
namespace ock::mxmd {
using UbseFaultEventRegisterPtr = int32_t (*)(ubs_mem_shm_fault_handler registerFunc);
using ShmFaultFuncPtr = int32_t (*)(const char *name, ubsmem_fault_type_t type);
using UbseLogCallBackRegisterPtr = void (*)(ubs_engine_log_handler log_handler);

class UbseMemExecutor {
public:
    uint32_t Initialize();
    uint32_t Exit();
    int32_t RegisterShmFaultEvent(ShmFaultFuncPtr registerFunc);
    static UbseMemExecutor &GetInstance()
    {
        static UbseMemExecutor instance;
        return instance;
    }
    ~UbseMemExecutor()
    {
        try {
            Exit();
        } catch (const std::exception &e) {
            DBG_LOGERROR("Exception in UbseMemExecutor destructor: ", e.what());
        }
    }
    UbseMemExecutor(const UbseMemExecutor &other) = default;
    UbseMemExecutor(UbseMemExecutor &&other) = default;
    UbseMemExecutor &operator=(const UbseMemExecutor &other) = default;
    UbseMemExecutor &operator=(UbseMemExecutor &&other) noexcept = default;

private:
    static int32_t UbseShmFaultHandlerFunc(const char *name, uint64_t memid, ubs_mem_fault_type_t type);
    void *handle{nullptr};
    UbseMemExecutor() = default;
    bool initialized_ = false;
    UbseFaultEventRegisterPtr UbseFaultEventRegisterFunc{};
    UbseLogCallBackRegisterPtr UbseLogCallBackRegisterFunc{};
    static ShmFaultFuncPtr ShmFaultFunc;
};
} // namespace ock::mxmd
#endif // UBSEMEMEXECUTOR_H
