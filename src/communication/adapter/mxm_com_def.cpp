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

#include "mxm_com_def.h"

#include "crc/dg_crc.h"
#include "strings/dg_str_util.h"

#include <log.h>

namespace ock::com {
using namespace ock::common;
using namespace ock::mxmd;
using namespace ock::dagger;
MxmEngineType MxmComEngineInfo::GetEngineType() const
{
    return engineType;
}

void MxmComEngineInfo::SetEngineType(MxmEngineType type)
{
    MxmComEngineInfo::engineType = type;
}

MxmProtocol MxmComEngineInfo::GetProtocol() const
{
    return protocol;
}

void MxmComEngineInfo::SetProtocol(MxmProtocol proto)
{
    MxmComEngineInfo::protocol = proto;
}

MxmWorkerMode MxmComEngineInfo::GetWorkerMode() const
{
    return workerMode;
}

void MxmComEngineInfo::SetWorkerMode(MxmWorkerMode mode)
{
    MxmComEngineInfo::workerMode = mode;
}

const std::string &MxmComEngineInfo::GetNodeId() const
{
    return nodeId;
}

void MxmComEngineInfo::SetNodeId(const std::string &id)
{
    MxmComEngineInfo::nodeId = id;
}

const std::string &MxmComEngineInfo::GetWorkGroup() const
{
    return workGroup;
}

void MxmComEngineInfo::SetWorkGroup(const std::string &group)
{
    MxmComEngineInfo::workGroup = group;
}

const std::string &MxmComEngineInfo::GetName() const
{
    return name;
}

void MxmComEngineInfo::SetName(const std::string &engineName)
{
    MxmComEngineInfo::name = engineName;
}

void MxmComEngineInfo::SetLogFunc(MxmComLogFunc func)
{
    MxmComEngineInfo::logFunc = func;
}

MxmComLogFunc MxmComEngineInfo::GetLogFunc() const
{
    return logFunc;
}

const RpcNode &MxmComEngineInfo::GetIpInfo() const
{
    return ipInfo;
}

void MxmComEngineInfo::SetIpInfo(const RpcNode &ipInfo)
{
    MxmComEngineInfo::ipInfo = ipInfo;
}

const std::pair<std::string, uint16_t> &MxmComEngineInfo::GetUdsInfo() const
{
    return udsInfo;
}

void MxmComEngineInfo::SetUdsInfo(const std::pair<std::string, uint16_t> &udsInfo)
{
    MxmComEngineInfo::udsInfo = udsInfo;
}

uint32_t MxmComEngineInfo::GetMaxHcomConnectNum() const
{
    return maxHcomConnectNum;
}

void MxmComEngineInfo::SetMaxHcomConnectNum(const uint32_t maxNum)
{
    this->maxHcomConnectNum = maxNum;
}

const uint32_t &MxmComEngineInfo::GetMaxSendReceiveSize() const
{
    return maxSendReceiveSize;
}

void MxmComEngineInfo::SetMaxSendReceiveSize(uint32_t maxSize)
{
    MxmComEngineInfo::maxSendReceiveSize = maxSize;
}

const uint32_t &MxmComEngineInfo::GetSendReceiveSegCount() const
{
    return sendReceiveSegCount;
}

void MxmComEngineInfo::SetSendReceiveSegCount(uint32_t segCount)
{
    MxmComEngineInfo::sendReceiveSegCount = segCount;
}

bool MxmComEngineInfo::IsUds() const
{
    return protocol == MxmProtocol::UDS;
}

bool MxmComEngineInfo::IsServerSide() const
{
    return engineType == MxmEngineType::SERVER;
}

const IsReconnectHook &MxmComEngineInfo::GetReConnectHook() const
{
    return reconnectHook;
}

void MxmComEngineInfo::SetReconnectHook(IsReconnectHook reconnectHook)
{
    MxmComEngineInfo::reconnectHook = reconnectHook;
}

const bool &MxmComEngineInfo::GetEnableTls() const
{
    return enableTls;
}

void MxmComEngineInfo::SetEnableTls(const bool &isEnable)
{
    enableTls = isEnable;
}

const UBSHcomNetCipherSuite &MxmComEngineInfo::GetCipherSuite() const
{
    return cipherSuite;
}

void MxmComEngineInfo::SetCipherSuite(const UBSHcomNetCipherSuite &suite)
{
    cipherSuite = suite;
}

const std::string &MxmComChannelConnectInfo::GetIp() const
{
    return ip;
}

void MxmComChannelConnectInfo::SetIp(const std::string &ipAddr)
{
    MxmComChannelConnectInfo::ip = ipAddr;
}

uint16_t MxmComChannelConnectInfo::GetPort() const
{
    return port;
}

void MxmComChannelConnectInfo::SetPort(uint16_t conPort)
{
    MxmComChannelConnectInfo::port = conPort;
}

uint16_t MxmComChannelConnectInfo::GetLinkNum() const
{
    return linkNum;
}

void MxmComChannelConnectInfo::SetLinkNum(uint16_t num)
{
    MxmComChannelConnectInfo::linkNum = num;
}

bool MxmComChannelConnectInfo::IsUds() const
{
    return isUds;
}

void MxmComChannelConnectInfo::SetIsUds(bool uds)
{
    MxmComChannelConnectInfo::isUds = uds;
}

const std::string &MxmComChannelConnectInfo::GetRemoteNodeId() const
{
    return remoteNodeId;
}

void MxmComChannelConnectInfo::SetRemoteNodeId(const std::string &remoteNodeIdSet)
{
    MxmComChannelConnectInfo::remoteNodeId = remoteNodeIdSet;
}

const std::string &MxmComChannelConnectInfo::GetCurNodeId() const
{
    return curNodeId;
}

void MxmComChannelConnectInfo::SetCurNodeId(const std::string &nodeId)
{
    MxmComChannelConnectInfo::curNodeId = nodeId;
}

bool MxmComChannelInfo::IsServerSide() const
{
    return isServer;
}

void MxmComChannelInfo::SetIsServer(bool isServerSide)
{
    MxmComChannelInfo::isServer = isServerSide;
}

const UBSHcomChannelPtr &MxmComChannelInfo::GetChannel() const
{
    return channel;
}

void MxmComChannelInfo::SetUDSInfo(UBSHcomNetUdsIdInfo &uds)
{
    MxmComChannelInfo::uds = uds;
}
UBSHcomNetUdsIdInfo MxmComChannelInfo::GetUDSInfo() const
{
    return uds;
}

void MxmComChannelInfo::SetChannel(const UBSHcomChannelPtr &ch)
{
    MxmComChannelInfo::channel = ch;
}

const MxmComChannelConnectInfo &MxmComChannelInfo::GetConnectInfo() const
{
    return connectInfo;
}

void MxmComChannelInfo::SetConnectInfo(const MxmComChannelConnectInfo &conInfo)
{
    MxmComChannelInfo::connectInfo = conInfo;
}

MxmChannelType MxmComChannelInfo::GetChannelType() const
{
    return channelType;
}

void MxmComChannelInfo::SetChannelType(MxmChannelType chType)
{
    MxmComChannelInfo::channelType = chType;
}

const std::string &MxmComChannelInfo::GetEngineName() const
{
    return engineName;
}

void MxmComChannelInfo::SetEngineName(const std::string &name)
{
    MxmComChannelInfo::engineName = name;
}
std::string MxmComChannelInfo::ConvertMxmComChannelInfoToString()
{
    std::string infoStr = "engine Name: " + engineName + "; ";
    infoStr = infoStr + "channel type: " + std::to_string(static_cast<int>(channelType)) + "; ";
    infoStr = infoStr + "channel id: " + std::to_string(channel->GetId()) + "; ";
    infoStr = infoStr + "cur node id: " + connectInfo.GetCurNodeId() + "; ";
    infoStr = infoStr + "remote node id: " + connectInfo.GetRemoteNodeId() + "; ";
    return infoStr;
}

const std::string PAYLOAD_SPLIT_SEP = "@";

MxmChannelType StringToChannelType(const std::string &type)
{
    if (type == "Normal") {
        return MxmChannelType::NORMAL;
    }
    if (type == "Emergency") {
        return MxmChannelType::EMERGENCY;
    }
    if (type == "Heartbeat") {
        return MxmChannelType::HEARTBEAT;
    }
    return MxmChannelType::SINGLE_SIDE;
}

std::string ChannelTypeToString(MxmChannelType type)
{
    if (type == MxmChannelType::NORMAL) {
        return "Normal";
    }
    if (type == MxmChannelType::EMERGENCY) {
        return "Emergency";
    }
    if (type == MxmChannelType::HEARTBEAT) {
        return "Heartbeat";
    }
    return "SingleSide";
}

std::string ChannelTypeToPayload(const std::string &id, MxmChannelType type)
{
    return id + PAYLOAD_SPLIT_SEP + ChannelTypeToString(type);
}

uint16_t MxmComMessageHead::GetOpCode() const
{
    return opCode;
}

void MxmComMessageHead::SetOpCode(uint16_t transOpCode)
{
    MxmComMessageHead::opCode = transOpCode;
}

uint16_t MxmComMessageHead::GetModuleCode() const
{
    return moduleCode;
}

void MxmComMessageHead::SetModuleCode(uint16_t transModuleCode)
{
    MxmComMessageHead::moduleCode = transModuleCode;
}

uint32_t MxmComMessageHead::GetBodyLen() const
{
    return bodyLen;
}

void MxmComMessageHead::SetBodyLen(uint32_t transBodyLen)
{
    MxmComMessageHead::bodyLen = transBodyLen;
}

uint32_t MxmComMessageHead::GetCrc() const
{
    return crc;
}

void MxmComMessageHead::SetCrc(uint32_t dataCrc)
{
    crc = dataCrc;
}

MxmComMessagePtr MxmComMessageCtx::GetMessage() const
{
    return message;
}

int MxmComMessageCtx::SetMessage(MxmComMessagePtr comMessage, size_t length)
{
    if (comMessage == nullptr) {
        DBG_LOGERROR("Message is nullptr");
        return -1;
    }
    message = new (std::nothrow) uint8_t[length];
    if (message == nullptr) {
        DBG_LOGERROR("Set message error, failed to allocate memory. length=" << length);
        return -1;
    }
    auto ret = memcpy_s(message, length, comMessage, length);
    if (ret != 0) {
        DBG_LOGERROR("Set message error, failed to copy memory. length=" << length);
        SafeDeleteArray(message);
        return -1;
    }
    return ret;
}

void MxmComMessageCtx::FreeMessage()
{
    if (message != nullptr) {
        SafeDeleteArray(message);
    }
}

uintptr_t MxmComMessageCtx::GetRspCtx() const
{
    return rspCtx;
}

void MxmComMessageCtx::SetRspCtx(uintptr_t transRspCtx)
{
    MxmComMessageCtx::rspCtx = transRspCtx;
}

uint64_t MxmComMessageCtx::GetChannelId() const
{
    return channelId;
}

void MxmComMessageCtx::SetChannelId(uint64_t netChannel)
{
    MxmComMessageCtx::channelId = netChannel;
}

const std::string &MxmComMessageCtx::GetSrcId() const
{
    return srcId;
}

void MxmComMessageCtx::SetSrcId(const std::string &msgSrcId)
{
    MxmComMessageCtx::srcId = msgSrcId;
}

const std::string &MxmComMessageCtx::GetDstId() const
{
    return dstId;
}

void MxmComMessageCtx::SetDstId(const std::string &id)
{
    MxmComMessageCtx::dstId = id;
}

MxmComMessageCtx::MxmComMessageCtx(MxmComMessagePtr message, std::string srcId, std::string dstId,
                                   MxmChannelType channelType)
    : message(message),
      srcId(std::move(srcId)),
      dstId(std::move(dstId)),
      channelType(channelType)
{
}

void MxmComMessageCtx::SetChannelType(MxmChannelType chType)
{
    channelType = chType;
}

MxmChannelType MxmComMessageCtx::GetChannelType()
{
    return channelType;
}

const std::string &MxmComMessageCtx::GetEngineName() const
{
    return engineName;
}

void MxmComMessageCtx::SetEngineName(const std::string &egName)
{
    engineName = egName;
}

const MxmUdsIdInfo &MxmComMessageCtx::GetUdsInfo() const
{
    return udsInfo;
}

void MxmComMessageCtx::SetUdsInfo(const MxmUdsIdInfo &uds)
{
    MxmComMessageCtx::udsInfo = uds;
}

void MxmComMessageCtx::SetReplyFuncHook(ReplyFuncHook replyHook)
{
    MxmComMessageCtx::replyFuncHook = replyHook;
}

const ReplyFuncHook &MxmComMessageCtx::GetReplyFuncHook() const
{
    return replyFuncHook;
}

void MxmComMessage::SetMessageHead(MxmComMessageHead &msgHead)
{
    head = msgHead;
}

const MxmComMessageHead &MxmComMessage::GetMessageHead() const
{
    return head;
}

MxmComMessagePtr MxmComMessage::AllocMessage(uint32_t len)
{
    uint32_t sumLen = sizeof(MxmComMessageHead) + len;
    auto msg = new (std::nothrow) uint8_t[sumLen];
    return msg;
}

void MxmComMessage::FreeMessage(MxmComMessagePtr msg)
{
    SafeDeleteArray(msg);
}

HRESULT MxmComMessage::SetMessageBody(const uint8_t *data, uint32_t len)
{
    if (LIKELY((len != 0) && data != nullptr)) {
        uint8_t *buff = static_cast<uint8_t *>(static_cast<void *>(this)) + sizeof(MxmComMessageHead);
        auto ret = memcpy_s(buff, len, data, len);
        if (UNLIKELY(ret != EOK)) {
            DBG_LOGERROR("messagebody memcpy_s failed.");
            return ret;
        }
    }
    head.SetBodyLen(len);
    return HOK;
}

MxmComMessagePtr MxmComMessage::GetMessageBody()
{
    return body;
}

uint32_t MxmComMessage::GetMessageBodyLen()
{
    return head.GetBodyLen();
}

MxmComMessagePtr TransRequestMsg(const NetMsgPacker &packer, uint16_t opCode, uint16_t moduleCode)
{
    std::string serializedData = packer.String();
    if (serializedData.empty()) {
        DBG_LOGERROR("request serialize failed.");
        return nullptr;
    }
    uint32_t bodyLen = serializedData.size();
    MxmComMessagePtr msg = MxmComMessage::AllocMessage(bodyLen);
    if (msg == nullptr) {
        DBG_LOGERROR("trans req message alloc failed.");
        return nullptr;
    }

    MxmComMessageHead msgHead{};
    msgHead.SetOpCode(opCode);
    msgHead.SetModuleCode(moduleCode);
    msgHead.SetBodyLen(bodyLen);
    const uint8_t *reqMsgData = reinterpret_cast<const uint8_t *>(serializedData.data());
    auto crc = CrcUtil::SoftCrc32(reqMsgData, bodyLen, 1);
    DBG_LOGDEBUG("Send msg body len is :" << bodyLen << ", crc is :" << crc);
    msgHead.SetCrc(crc);
    auto ucMsg = static_cast<MxmComMessage *>(static_cast<void *>(msg));
    ucMsg->SetMessageHead(msgHead);
    auto ret = ucMsg->SetMessageBody(reqMsgData, bodyLen);
    if (ret != HOK) {
        DBG_LOGERROR("set req message body failed.");
        MxmComMessage::FreeMessage(msg);
        return nullptr;
    }
    return msg;
}

std::pair<std::string, MxmChannelType> SplitPayload(const std::string &payload)
{
    std::vector<std::string> payloadPair;
    StrUtil::Split(payload, PAYLOAD_SPLIT_SEP, payloadPair);
    if (payloadPair.empty()) {
        return std::make_pair(payload, MxmChannelType::SINGLE_SIDE);
    }
    if (payloadPair.size() < 2U) {
        return {};
    }
    return std::make_pair(payloadPair[0], StringToChannelType(payloadPair[1]));
}

MxmComMessage *GetMessageFromNetServiceContext(const UBSHcomServiceContext &context)
{
    return static_cast<MxmComMessage *>(context.MessageData());
}

MxmComMessage *GetMessageFromNetServiceMessage(UBSHcomRequest &msg)
{
    return static_cast<MxmComMessage *>(msg.address);
}

bool CheckMessageBodyLen(UBSHcomServiceContext &context, MxmComMessage &msg)
{
    return context.MessageDataLen() == (sizeof(MxmComMessage) + msg.GetMessageBodyLen());
}

bool CheckMessageBodyLen(UBSHcomRequest &netServiceMessage, MxmComMessage &msg)
{
    return netServiceMessage.size == (sizeof(MxmComMessage) + msg.GetMessageBodyLen());
}

uint64_t GetChannelIdFromNetServiceContext(const UBSHcomServiceContext &context)
{
    return context.Channel()->GetId();
}

void GetUdsInfoFromNetServiceContext(const UBSHcomServiceContext &context, MxmUdsIdInfo &udsIdInfo)
{
    UBSHcomNetUdsIdInfo uds;
    auto ret = context.Channel()->GetRemoteUdsIdInfo(uds);
    if (ret == 0) {
        DBG_LOGDEBUG("Uds info is " << uds.pid << "," << uds.uid << "," << uds.gid);
        udsIdInfo.pid = uds.pid;
        udsIdInfo.uid = uds.uid;
        udsIdInfo.gid = uds.gid;
    }
}
} // namespace ock::com
