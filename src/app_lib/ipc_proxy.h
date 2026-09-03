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

#ifndef MEMORYFABRIC_IPC_PROXY_H
#define MEMORYFABRIC_IPC_PROXY_H

#include <functional>
#include <string>
#include <utility>

#include "log.h"
#include "mxm_ipc_client_interface.h"
#include "mxm_msg.h"
#include "ptracer.h"
#include "rack_mem_err.h"
#include "ubsm_ptracer.h"

namespace ock::mxmd {
using namespace ock::common;
class IpcProxy {
public:
    static uint32_t Initialize();

    uint32_t SyncCall(int opcode, MsgBase *request, MsgBase *response);

    static uint32_t Destroy();

    uint32_t Ipc(MsgBase *request, MsgBase *response, const int opCode)
    {
        if (request == nullptr) {
            DBG_LOGINFO("Request is nullptr.");
            return MXM_ERR_PARAM_INVALID;
        }
        if (response == nullptr) {
            DBG_LOGINFO("Response is nullptr.");
            return MXM_ERR_PARAM_INVALID;
        };

        DBG_LOGINFO("IPC SyncCall begin, opCode=" << opCode);
        uint32_t hr;
        for (int i = 0; i < 3u; ++i) {
            TP_TRACE_BEGIN(TP_UBSM_IPC_CALL);
            hr = SyncCall(opCode, request, response);
            TP_TRACE_END(TP_UBSM_IPC_CALL, hr);
            if (hr != MXM_ERR_IPC_CRC_CHECK_ERROR && hr != MXM_ERR_IPC_SERIALIZE_DESERIALIZE_ERROR) {
                break;
            }
            DBG_LOGWARN("Failed to send message, ret=" << hr << ", opCode=" << opCode << ", retring");
            usleep(300U * (i + 1U) * (i + 1U));
        }
        if (BresultFail(hr)) {
            DBG_LOGERROR("Failed to send message, ret=" << hr << " ,opCode" << opCode);
            return hr;
        }
        DBG_LOGINFO("IPC SyncCall end, hr=" << hr);
        return hr;
    }

    static IpcProxy &GetInstance()
    {
        static IpcProxy instance;
        return instance;
    }
    IpcProxy(const IpcProxy &other) = default;
    IpcProxy(IpcProxy &&other) = default;
    IpcProxy &operator=(const IpcProxy &other) = default;
    IpcProxy &operator=(IpcProxy &&other) noexcept = default;

    DAGGER_DEFINE_REF_COUNT_FUNCTIONS
private:
    std::mutex mLock{};
    DAGGER_DEFINE_REF_COUNT_VARIABLE;
    IpcProxy() = default;
};
} // namespace ock::mxmd

#endif // MEMORYFABRIC_IPC_PROXY_H