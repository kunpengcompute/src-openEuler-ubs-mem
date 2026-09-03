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

#ifndef MXM_COM_BASE_H
#define MXM_COM_BASE_H
#include <crc/dg_crc.h> // for ReadWriteLock
#include <cstdint>      // for uint16_t, uint32_t, uint64_t
#include <functional>   // for function

#include <sys/types.h> // for uint
#include <unistd.h>    // for sleep
#include <map>         // for map
#include <mutex>       // for mutex
#include <new>         // for nothrow
#include <string>      // for basic_string, string, operator<
#include <utility>     // for move
#include <vector>      // for vector

#include <referable/dg_ref.h> // for Ref, Referable
#include "log.h"
#include "mxm_com_def.h"    // for MxmComMessageCtx, MxmComMessage
#include "mxm_com_engine.h" // for MxmCommunication
#include "mxm_msg.h"
#include "ubsm_com_constants.h"
#include "util/defines.h"
#include "util/functions.h"

namespace ock::com {
using namespace ock::hcom;
using namespace ock::mxmd;

enum class MxmModuleCode {
    COLLECTOR = 0,
    MEM = 1,
    VM = 2,
    SSU = 3,
    HTTP = 4,
    RESOURCE_MGR = 5,
    NODE = 6,
    CONFIG = 7,
    VM_BORROW = 10,
    DEV = 11,
    REMOTE = 12,
    ELECTION = 13,
    DATA_SYNC = 14,
    STORAGE = 15,
    UBUS = 16,
    PSK = 666,
    PSK_AGENT = 667,
};

class MxmComBaseMessageHandlerCtx {
public:
    MxmComBaseMessageHandlerCtx(std::string engineName, uint64_t channelId, uintptr_t rspCtx);

    uint64_t GetChannelId() const;

    uintptr_t GetResponseCtx();

    const std::string &GetEngineName() const;

    const MxmUdsIdInfo &GetUdsIdInfo() const;

    void SetUdsIdInfo(const MxmUdsIdInfo &uds);

    uint32_t GetCrc() const;

    void SetCrc(uint32_t dataCrc);

private:
    std::string engineName;
    uint64_t channelId;
    uintptr_t rspCtx;
    uint32_t crc;
    MxmUdsIdInfo udsIdInfo;
};

using MxmComBaseMessageHandlerCtxPtr = MxmComBaseMessageHandlerCtx *;

class MxmComBaseMessageHandler : public Referable {
public:
    virtual HRESULT Handle(const MsgBase *req, MsgBase *rsp, MxmComBaseMessageHandlerCtxPtr ctx)
    {
        (void)req;
        (void)rsp;
        (void)ctx;
        return HOK;
    }
    virtual uint16_t GetOpCode() = 0;
    virtual uint16_t GetModuleCode() = 0;
    virtual bool NeedReply()
    {
        return true;
    };
};

using MxmComBaseMessageHandlerPtr = Ref<MxmComBaseMessageHandler>;

class MxmComBaseMessageHandlerManager {
public:
    static void AddHandler(MxmComBaseMessageHandlerPtr handler, const std::string &engineName);

    static void RemoveHandler(uint16_t moduleCode, uint16_t opCode, const std::string &engineName);

    static MxmComBaseMessageHandlerPtr GetHandler(uint16_t moduleCode, uint16_t opCode, const std::string &engineName);

private:
    static std::map<std::string, MxmComBaseMessageHandlerPtr> gHandlerMap;
    static std::mutex gLock;
};

class SendParam {
public:
    SendParam(std::string remoteId, uint16_t moduleCode, uint16_t opCode, MxmChannelType channelType)
        : remoteId(std::move(remoteId)),
          moduleCode(moduleCode),
          opCode(opCode),
          channelType(channelType){};

    SendParam(std::string remoteId, uint16_t moduleCode, uint16_t opCode)
        : remoteId(std::move(remoteId)),
          moduleCode(moduleCode),
          opCode(opCode){};

    const std::string &GetRemoteId() const;

    void SetRemoteId(const std::string &remoteIdSet);

    uint16_t GetModuleCode() const;

    void SetModuleCode(uint16_t moduleCodeSet);

    uint16_t GetOpCode() const;

    void SetOpCode(uint16_t opCodeSet);

    MxmChannelType GetChannelType() const;

    void SetChannelType(MxmChannelType chType);

private:
    std::string remoteId; // 远程节点ID
    uint16_t moduleCode;  // 模块Id
    uint16_t opCode;      // 操作码
    MxmChannelType channelType = MxmChannelType::NORMAL;
};

void Reply(MxmComMessageCtx &message, MsgBase *respPtr);

void ReplyCallback(void *ctx, void *recv, uint32_t len, int32_t result);

class MxmLinkInfo {
public:
    MxmLinkInfo(std::string nodeId, MxmLinkState state);

    MxmLinkInfo(std::string nodeId, MxmLinkState state, uint64_t timeStamp, uint32_t pid);

    const std::string &GetNodeId() const;
    const uint32_t &GetPID() const;

    MxmLinkState GetState() const;

    void SetTimeStamp(uint64_t nowTime);

    uint64_t GetTimeStamp() const;

    inline bool operator==(const MxmLinkInfo &other) const
    {
        return nodeId == other.nodeId && state == other.state && timeStamp == other.timeStamp && pid == other.pid;
    }

private:
    std::string nodeId;
    uint32_t pid;
    MxmLinkState state;
    uint32_t timeStamp{0};
};

using LinkStateMap = std::map<std::string, std::map<std::string, uint32_t>>;
using LinkNotifyFunction = std::function<void(const std::vector<MxmLinkInfo> &)>;
using LinkNotifyFunctionMap = std::map<std::string, std::vector<LinkNotifyFunction>>;

using HandlerExecutor = std::function<void(const std::function<void()> &task)>;
using LinkEventHandler = std::function<void(const std::vector<MxmLinkInfo> &linkInfoList)>;

void DefaultHandlerExecutor(const std::function<void()> &task);

void DefaultLinkEventHandler(const std::vector<MxmLinkInfo> &linkInfoList);

class MxmComBase : public Referable {
public:
    MxmComBase(std::string nodeId, std::string name) : nodeId(nodeId), name(name){};

    /* *
   * @brief 启动Server或Client
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    virtual HRESULT Start() = 0;

    /* *
   * @brief 停止Server或Client
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    virtual void Stop() = 0;

    /* *
   * @brief 向对端建连
   * @param remoteNodeId [in] 对端nodeId
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    virtual HRESULT Connect(const RpcNode &remoteNodeId)
    {
        return HOK;
    };

    /* *
   * @brief 向对端建连
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    virtual HRESULT Connect()
    {
        return HOK;
    };

    /* *
   * @brief 通过配置指定连接对端节点
   * @param option [in] 连接配置
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    virtual HRESULT ConnectWithOption(ConnectOption)
    {
        return HOK;
    };

    virtual void TlsOn();

    /* *
   * @brief 通过通道id移除通道
   * @param id [in] 通道id
   */
    void RemoveChannel(const std::string &remoteNodeId, MxmChannelType type)
    {
        MxmCommunication::RemoveChannel(name, remoteNodeId, type);
    }

    static void SetHandlerExecutor(const HandlerExecutor &handlerExecutor);

    static void SetIpcHandlerExecutor(const HandlerExecutor &handlerExecutor);

    static void SetLinkEventHandler(const LinkEventHandler &handler);

    /* *
   * @brief 注册消息处理函数
   * @param[in] handlerPtr: 消息处理函数
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT RegMessageHandler(MxmComBaseMessageHandlerPtr handlerPtr)
    {
        MxmComBaseMessageHandlerManager::AddHandler(handlerPtr, name);
        MxmComMsgHandler hdl{};
        hdl.opCode = handlerPtr->GetOpCode();
        hdl.moduleCode = handlerPtr->GetModuleCode();
        hdl.handler = HandleRequest;
        return MxmCommunication::RegMxmComMsgHandler(name, hdl);
    }

    /* *
   * @brief 同步发送消息
   * @param[in] param: 调用参数
   * @param[in] request: 请求体
   * @param[in] response: 返回体
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT Send(const SendParam &param, MsgBase *request, MsgBase *response)
    {
        NetMsgPacker packer;
        request->Serialize(packer);
        MxmComMessagePtr msg = TransRequestMsg(packer, param.GetOpCode(), param.GetModuleCode());
        if (msg == nullptr) {
            DBG_LOGERROR("node " << nodeId << " trans req msg failed");
            return HFAIL;
        }
        MxmChannelType type = param.GetChannelType();
        MxmComMessageCtx transMessage{msg, nodeId, param.GetRemoteId(), type};
        MxmComDataDesc retData(nullptr, 0);
        auto ret = MxmCommunication::MxmComMsgSend(name, transMessage, retData);
        if (ret != HOK) {
            DBG_LOGERROR("node " << nodeId << " call " << param.GetRemoteId() << " failed, " << ret);
            MxmComMessage::FreeMessage(msg);
            return ret;
        }
        std::string respStr(reinterpret_cast<char *>(retData.data), retData.len);
        NetMsgUnpacker unpacker(respStr);
        response->Deserialize(unpacker);
        MxmComMessage::FreeMessage(msg);
        SafeFree(retData.data);
        return ret;
    }

    /* *
   * @brief 回复消息接口
   * @param[in] message: 消息上下文
   * @param[in] response: 消息返回体
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT ReplyMsg(MxmComMessageCtx &message, const MxmComDataDesc &response);

    /* *
   * @brief 获取当前连接信息
   * @return std::vector<MxmLinkInfo>, 所有的连接信息
   */
    std::vector<MxmLinkInfo> GetAllLinkInfo();

    /* *
   * @brief 添加连接变更回调函数
   * @param[in] func: 回调函数定义
   */
    void AddLinkNotifyFunc(const LinkNotifyFunction &func);

    static void ClearStateMap()
    {
        g_linkStateMap.clear();
    }

protected:
    static void LinkNotify(const MxmComEngineInfo &info, const std::string &curNodeId, uint64_t pid,
                           MxmLinkState state);

    std::string nodeId;
    std::string name;
    std::string pskstr = "";

private:
    static std::vector<MxmLinkInfo> GetLinkInfoFromMap(const std::string &engineName, uint32_t pid = 0);

    static void HandleRequest(MxmComMessageCtx &message)
    {
        auto ucMsg = static_cast<MxmComMessage *>(static_cast<void *>(message.GetMessage()));
        uint16_t moduleCode = ucMsg->GetMessageHead().GetModuleCode();
        uint16_t opCode = ucMsg->GetMessageHead().GetOpCode();
        uint32_t crc = ucMsg->GetMessageHead().GetCrc();
        MxmComBaseMessageHandlerPtr handler =
            MxmComBaseMessageHandlerManager::GetHandler(moduleCode, opCode, message.GetEngineName());
        if (handler == nullptr) {
            DBG_LOGERROR("module " << moduleCode << " opCode " << opCode << " handler not exists");
            return;
        }
        auto reqPtr = CreateRequestByOpCode(opCode);
        if (reqPtr == nullptr) {
            DBG_LOGERROR("module " << moduleCode << " opCode " << opCode << " new req fail");
            return;
        }
        auto respPtr = CreateResponseByOpCode(opCode);
        if (respPtr == nullptr) {
            DBG_LOGERROR("module " << moduleCode << " opCode " << opCode << " new resp fail");
            delete reqPtr;
            return;
        }
        std::string reqStr = std::string(reinterpret_cast<char *>(ucMsg->GetMessageBody()), ucMsg->GetMessageBodyLen());
        NetMsgUnpacker unpacker(reqStr);
        reqPtr->Deserialize(unpacker);
        SubmitHandlerTask(crc, handler, message, reqPtr, respPtr);
        DBG_LOGDEBUG("module " << moduleCode << " , opCode " << opCode << " request end");
        delete reqPtr;
        delete respPtr;
    }

    static void SubmitHandlerTask(uint32_t crc, MxmComBaseMessageHandlerPtr &handler, MxmComMessageCtx &message,
                                  MsgBase *reqPtr, MsgBase *respPtr)
    {
        auto udsInfo = message.GetUdsInfo();
        const auto &engineName = message.GetEngineName();
        auto channelId = message.GetChannelId();
        auto respCtx = message.GetRspCtx();
        auto replyHook = message.GetReplyFuncHook();
        auto moduleCode = handler->GetModuleCode();
        auto opCode = handler->GetOpCode();
        HandlerExecutor executor;
        if (engineName == MXM_AGENT_IPC_SERVER_ENGINE_NAME) {
            executor = gIpcHandlerExecutor;
        } else {
            executor = gHandlerExecutor;
        }
        executor(
            [crc, engineName, channelId, respCtx, moduleCode, opCode, udsInfo, handler, reqPtr, respPtr, replyHook] {
                auto ctx = new (std::nothrow) MxmComBaseMessageHandlerCtx(engineName, channelId, respCtx);
                if (ctx == nullptr) {
                    DBG_LOGERROR("module " << moduleCode << " opCode " << opCode
                                           << " new MxmComBaseMessageHandlerCtx fail");
                    return;
                }
                ctx->SetCrc(crc);
                ctx->SetUdsIdInfo(udsInfo);
                auto handlerRet = handler->Handle(reqPtr, respPtr, ctx);
                if (handlerRet != HOK) {
                    DBG_LOGERROR("module " << moduleCode << " opCode " << opCode << " exec failed," << handlerRet);
                }

                MxmComMessageCtx msgCtx;
                msgCtx.SetEngineName(engineName);
                msgCtx.SetRspCtx(respCtx);
                msgCtx.SetReplyFuncHook(replyHook);
                msgCtx.SetChannelId(channelId);
                if (handler->NeedReply()) {
                    Reply(msgCtx, respPtr);
                }
            });
    }

private:
    static ReadWriteLock g_lock;
    static LinkStateMap g_linkStateMap;
    static LinkNotifyFunctionMap g_notifyFuncMap;
    static HandlerExecutor gHandlerExecutor;
    static HandlerExecutor gIpcHandlerExecutor;
    static LinkEventHandler gLinkEventHandler;
};

void Log(int level, const char *str);
} // namespace ock::com
#endif // MXM_COM_BASE_H
