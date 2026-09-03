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
#include "mxm_com_base.h"

#include <lock/dg_lock.h> // for ReadWriteLock, WriteLocker, ReadLocker
#include <log.h>
#include <securec.h> // for memcpy_s, EOK
#include <algorithm> // for max
#include <utility>   // for pair, move

namespace ock::com {
using namespace ock::common;
using namespace ock::mxmd;
std::map<std::string, MxmComBaseMessageHandlerPtr> MxmComBaseMessageHandlerManager::gHandlerMap;
std::mutex MxmComBaseMessageHandlerManager::gLock;
ReadWriteLock MxmComBase::g_lock;
LinkStateMap MxmComBase::g_linkStateMap{};
LinkNotifyFunctionMap MxmComBase::g_notifyFuncMap{};
HandlerExecutor MxmComBase::gHandlerExecutor = DefaultHandlerExecutor;
HandlerExecutor MxmComBase::gIpcHandlerExecutor = DefaultHandlerExecutor;
LinkEventHandler MxmComBase::gLinkEventHandler = DefaultLinkEventHandler;

const std::string &SendParam::GetRemoteId() const
{
    return remoteId;
}

void SendParam::SetRemoteId(const std::string &remoteIdSet)
{
    SendParam::remoteId = remoteIdSet;
}

uint16_t SendParam::GetModuleCode() const
{
    return moduleCode;
}

void SendParam::SetModuleCode(uint16_t moduleCodeSet)
{
    SendParam::moduleCode = moduleCodeSet;
}

uint16_t SendParam::GetOpCode() const
{
    return opCode;
}

void SendParam::SetOpCode(uint16_t opCodeSet)
{
    SendParam::opCode = opCodeSet;
}

MxmChannelType SendParam::GetChannelType() const
{
    return channelType;
}

void SendParam::SetChannelType(MxmChannelType chType)
{
    channelType = chType;
}

void MxmComBaseMessageHandlerManager::AddHandler(MxmComBaseMessageHandlerPtr handler, const std::string &engineName)
{
    std::lock_guard<std::mutex> lock(gLock);
    if (handler == nullptr) {
        DBG_LOGERROR("handler is nullptr.");
        return;
    }
    std::string key = engineName + KEY_SEP + std::to_string(handler->GetModuleCode()) + KEY_SEP +
                      std::to_string(handler->GetOpCode());
    gHandlerMap.emplace(key, handler);
}

void MxmComBaseMessageHandlerManager::RemoveHandler(uint16_t moduleCode, uint16_t opCode, const std::string &engineName)
{
    std::lock_guard<std::mutex> lock(gLock);
    std::string key = engineName + KEY_SEP + std::to_string(moduleCode) + KEY_SEP + std::to_string(opCode);
    auto iter = gHandlerMap.find(key);
    if (iter == gHandlerMap.end()) {
        return;
    }
    gHandlerMap.erase(key);
}

MxmComBaseMessageHandlerPtr MxmComBaseMessageHandlerManager::GetHandler(uint16_t moduleCode, uint16_t opCode,
                                                                        const std::string &engineName)
{
    std::lock_guard<std::mutex> lock(gLock);
    std::string key = engineName + KEY_SEP + std::to_string(moduleCode) + KEY_SEP + std::to_string(opCode);
    auto iter = gHandlerMap.find(key);
    if (iter == gHandlerMap.end()) {
        DBG_LOGERROR("Handler not register, module code " << moduleCode << ",op code " << opCode);
        return nullptr;
    }
    return iter->second;
}

std::vector<MxmLinkInfo> MxmComBase::GetLinkInfoFromMap(const std::string &engineName, uint32_t pid)
{
    std::vector<MxmLinkInfo> info;
    auto iter = g_linkStateMap.find(engineName);
    if (iter == g_linkStateMap.end()) {
        return info;
    }
    for (const auto &kv : iter->second) {
        auto timeStamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                                                   std::chrono::high_resolution_clock::now().time_since_epoch())
                                                   .count());
        if (kv.second > 0) {
            info.emplace_back(kv.first, MxmLinkState::LINK_UP, timeStamp, pid);
        } else {
            info.emplace_back(kv.first, MxmLinkState::LINK_DOWN, timeStamp, pid);
        }
    }
    return info;
}

void MxmComBase::TlsOn()
{
    return;
}

void Log(int level, const char *str)
{
    switch (level + 1) {
        case static_cast<int>(DBG_LOG_DEBUG):
            DBG_LOGDEBUG(str);
            break;
        case static_cast<int>(DBG_LOG_INFO):
            DBG_LOGINFO(str);
            break;
        case static_cast<int>(DBG_LOG_WARN):
            DBG_LOGWARN(str);
            break;
        case static_cast<int>(DBG_LOG_ERROR):
            DBG_LOGERROR(str);
            break;
        default:
            DBG_LOGERROR(str);
    }
}

void MxmComBase::LinkNotify(const MxmComEngineInfo &info, const std::string &curNodeId, uint64_t pid,
                            MxmLinkState state)
{
    WriteLocker<ReadWriteLock> lock(&g_lock);
    auto engineName = info.GetName();
    auto iter = g_linkStateMap.find(engineName);
    if (iter == g_linkStateMap.end()) {
        g_linkStateMap.emplace(engineName, std::map<std::string, uint32_t>());
    }
    iter = g_linkStateMap.find(engineName);
    auto stateIter = iter->second.find(curNodeId);
    if (stateIter == iter->second.end()) {
        iter->second.emplace(curNodeId, 0);
    }
    stateIter = iter->second.find(curNodeId);
    switch (state) {
        case MxmLinkState::LINK_UP:
            stateIter->second++;
            gLinkEventHandler(GetLinkInfoFromMap(engineName, pid));
            break;
        case MxmLinkState::LINK_DOWN:
            if (stateIter->second > 0) {
                stateIter->second--;
            }
            gLinkEventHandler(GetLinkInfoFromMap(engineName, pid));
            if (curNodeId.compare(0, strlen(REMOTE_NODE_ID_PREFIX), REMOTE_NODE_ID_PREFIX) == 0) {
                g_linkStateMap.erase(engineName);
            }
            break;
        case MxmLinkState::LINK_STATE_UNKNOWN:
            gLinkEventHandler(GetLinkInfoFromMap(engineName, pid));
            break;
    }
    DBG_LOGINFO("node " << curNodeId << " link state " << static_cast<uint32_t>(state) << ", link number "
                        << stateIter->second);
    auto notifyIter = g_notifyFuncMap.find(engineName);
    if (notifyIter == g_notifyFuncMap.end()) {
        return;
    }
    auto linkInfo = GetLinkInfoFromMap(engineName, pid);
    for (const auto &notify : notifyIter->second) {
        notify(linkInfo);
    }
}

void ReplyCallback(void *ctx, void *recv, uint32_t len, int32_t result)
{
    if (result != 0) {
        DBG_LOGERROR("reply message failed, ret: " << result);
    }
}

void Reply(MxmComMessageCtx &message, MsgBase *respPtr)
{
    if (respPtr == nullptr) {
        DBG_LOGERROR("response is null");
        return;
    }
    NetMsgPacker packer;
    respPtr->Serialize(packer);
    auto respStr = packer.String();
    if (respStr.empty()) {
        DBG_LOGERROR("response is empty");
        return;
    }
    uint32_t rspLen = respStr.size();
    auto rspData = new (std::nothrow) char[rspLen];
    if (!rspData) {
        DBG_LOGERROR("rspData is nullptr.");
        return;
    }
    auto ret = memcpy_s(rspData, rspLen, respStr.data(), rspLen);
    if (ret != 0) {
        DBG_LOGERROR("copy response data failed, ret: " << ret);
        delete[] rspData;
        return;
    }
    MxmComDataDesc data(reinterpret_cast<uint8_t *>(rspData), rspLen);
    MxmCommunication::MxmComMsgReply(message, data, MxmComCallback{ReplyCallback, &message});
    delete[] rspData;
}

HRESULT MxmComBase::ReplyMsg(MxmComMessageCtx &message, const MxmComDataDesc &response)
{
    MxmCommunication::MxmComMsgReply(message, response, MxmComCallback{ReplyCallback, &message});
    return HOK;
}

void MxmComBase::SetHandlerExecutor(const HandlerExecutor &handlerExecutor)
{
    gHandlerExecutor = handlerExecutor;
}

void MxmComBase::SetIpcHandlerExecutor(const HandlerExecutor &handlerExecutor)
{
    gIpcHandlerExecutor = handlerExecutor;
}

void MxmComBase::SetLinkEventHandler(const LinkEventHandler &handler)
{
    gLinkEventHandler = handler;
}

std::vector<MxmLinkInfo> MxmComBase::GetAllLinkInfo()
{
    ReadLocker<ReadWriteLock> lock(&g_lock);
    return GetLinkInfoFromMap(name);
}

void MxmComBase::AddLinkNotifyFunc(const LinkNotifyFunction &func)
{
    WriteLocker<ReadWriteLock> lock(&g_lock);
    auto iter = g_notifyFuncMap.find(name);
    if (iter == g_notifyFuncMap.end()) {
        g_notifyFuncMap.emplace(name, std::vector<LinkNotifyFunction>());
    }
    iter = g_notifyFuncMap.find(name);
    iter->second.emplace_back(func);
}

uint64_t MxmComBaseMessageHandlerCtx::GetChannelId() const
{
    return channelId;
}

uintptr_t MxmComBaseMessageHandlerCtx::GetResponseCtx()
{
    return rspCtx;
}

MxmComBaseMessageHandlerCtx::MxmComBaseMessageHandlerCtx(std::string engineName, uint64_t channelId, uintptr_t rspCtx)
    : engineName(std::move(engineName)),
      channelId(channelId),
      rspCtx(rspCtx)
{
}

const std::string &MxmComBaseMessageHandlerCtx::GetEngineName() const
{
    return engineName;
}

const MxmUdsIdInfo &MxmComBaseMessageHandlerCtx::GetUdsIdInfo() const
{
    return udsIdInfo;
}

void MxmComBaseMessageHandlerCtx::SetUdsIdInfo(const MxmUdsIdInfo &uds)
{
    MxmComBaseMessageHandlerCtx::udsIdInfo = uds;
}

uint32_t MxmComBaseMessageHandlerCtx::GetCrc() const
{
    return crc;
}

void MxmComBaseMessageHandlerCtx::SetCrc(uint32_t dataCrc)
{
    crc = dataCrc;
}

MxmLinkInfo::MxmLinkInfo(std::string nodeId, MxmLinkState state) : nodeId(std::move(nodeId)), state(state) {}

const std::string &MxmLinkInfo::GetNodeId() const
{
    return nodeId;
}
const uint32_t &MxmLinkInfo::GetPID() const
{
    return pid;
}

MxmLinkState MxmLinkInfo::GetState() const
{
    return state;
}

void MxmLinkInfo::SetTimeStamp(uint64_t nowTime)
{
    timeStamp = nowTime;
}

uint64_t MxmLinkInfo::GetTimeStamp() const
{
    return timeStamp;
}

void DefaultHandlerExecutor(const std::function<void()> &task)
{
    task();
}

void DefaultLinkEventHandler(const std::vector<MxmLinkInfo> &linkInfoList) {}

MxmLinkInfo::MxmLinkInfo(std::string nodeId, MxmLinkState state, uint64_t timeStamp, uint32_t pid)
    : nodeId(std::move(nodeId)),
      pid(pid),
      state(state),
      timeStamp(timeStamp)
{
}

} // namespace ock::com