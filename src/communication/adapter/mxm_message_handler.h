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
#ifndef MXM_MESSAGE_HANDLE_H
#define MXM_MESSAGE_HANDLE_H
#include "comm_def.h"
#include "mxm_com_base.h"

namespace ock::com {
class MxmNetMessageHandler : public MxmComBaseMessageHandler {
public:
    MxmNetMessageHandler(uint16_t opCode, uint16_t moduleCode, MxmComServiceHandler handler)
        : opCode(opCode),
          moduleCode(moduleCode),
          handler(std::move(handler))
    {
    }
    MxmNetMessageHandler(uint16_t opCode, uint16_t moduleCode, MxmComIpcServiceHandler handler)
        : opCode(opCode),
          moduleCode(moduleCode),
          ipcHandler(std::move(handler))
    {
        isIpc = true;
    }
    HRESULT Handle(const MsgBase *req, MsgBase *rsp, MxmComBaseMessageHandlerCtxPtr handlerCtx) override
    {
        HRESULT ret = HOK;
        if (isIpc) {
            MxmComUdsInfo udsInfo;
            if (handlerCtx != nullptr) {
                auto info = handlerCtx->GetUdsIdInfo();
                udsInfo.pid = info.pid;
                udsInfo.uid = info.uid;
                udsInfo.gid = info.gid;
            }
            ipcHandler(udsInfo, req, rsp);
        } else {
            handler(req, rsp);
        }

        if (handlerCtx != nullptr && !handlerCtx->GetEngineName().empty()) {
            NetMsgPacker packer;
            rsp->Serialize(packer);
            auto respStr = packer.String();
            if (respStr.empty()) {
                DBG_LOGERROR("response is empty");
                return HFAIL;
            }
            uint32_t rspLen = respStr.size();
            auto rspData = new (std::nothrow) char[rspLen];
            if (!rspData) {
                DBG_LOGERROR("rspData is nullptr.");
                return HFAIL;
            }
            auto result = memcpy_s(rspData, rspLen, respStr.data(), rspLen);
            if (result != 0) {
                DBG_LOGERROR("copy response data failed, ret: " << result);
                delete[] rspData;
                return HFAIL;
            }
            MxmComDataDesc dataDesc;
            dataDesc.data = reinterpret_cast<uint8_t *>(rspData);
            dataDesc.len = static_cast<uint32_t>(rspLen);
            MxmComMessageCtx ctx;
            ctx.SetEngineName(handlerCtx->GetEngineName());
            ctx.SetRspCtx(handlerCtx->GetResponseCtx());
            ctx.SetChannelId(handlerCtx->GetChannelId());
            ret = MxmComBase::ReplyMsg(ctx, dataDesc);
            delete[] rspData;
        }
        SafeDelete(handlerCtx);
        return ret;
    }
    uint16_t GetModuleCode() override
    {
        return moduleCode;
    }
    uint16_t GetOpCode() override
    {
        return opCode;
    }

    bool NeedReply() override
    {
        return false;
    }

private:
    uint16_t opCode;
    uint16_t moduleCode;
    MxmComServiceHandler handler;
    MxmComIpcServiceHandler ipcHandler;
    bool isIpc = false;
};
} // namespace ock::com
#endif // MXM_MESSAGE_HANDLE_H