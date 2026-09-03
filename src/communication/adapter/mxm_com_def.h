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

#ifndef MXM_COM_DEF_H
#define MXM_COM_DEF_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include <securec.h>
#include "common_def.h"
#include "hcom_service.h"
#include "hcom_service_channel.h"
#include "hcom_service_context.h"
#include "mxm_com_error.h"
#include "mxm_msg.h"
#include "mxm_msg_base.h"
#include "mxm_msg_packer.h"
#include "rack_mem_constants.h"
#include "rpc_config.h"
#include "strings/dg_str_util.h"
#include "ubsm_com_constants.h"
#include "util/defines.h"

namespace ock::com {
using namespace ock::hcom;
using namespace ock::mxmd;
using namespace ock::rpc;
// 消息回复钩子函数
using ReplyFuncHook = std::function<HRESULT(const UBSHcomRequest &msg, const Callback *done)>;

enum class MxmEngineType { CLIENT = 0, SERVER };

enum class MxmProtocol { TCP = 1, UDS, HCCS, UB };

enum class MxmWorkerMode {
    NET_BUSY_POLLING = 0, // Worker保持空转，CPU占用高，性能高。
    NET_EVENT_POLLING     // Worker采用事件驱动，CPU占用低，性能相比略低。
};

enum class MxmLinkState { LINK_UP = 0, LINK_DOWN, LINK_STATE_UNKNOWN };

enum class MxmChannelType {
    SINGLE_SIDE = 0, // 单向通道，客户端向服务端发送数据流单向通道
    NORMAL,          // 双向通道
    EMERGENCY,       // 紧急消息通道
    HEARTBEAT        // 心跳通道
};

struct ConnectOption {
    std::string nodeId;         // 连接远端节点ID
    std::string ip;             // 连接远端节点IP
    uint16_t port;              // 连接远端节点端口
    MxmChannelType channelType; // 链路类型
};

// 参数一：日志级别
// 参数二：日志内容
using MxmComLogFunc = void (*)(int, const char *);
/* 与server端重新建链后的处理函数，可以汇报自己的信息 */

using MxmComPostReconnectHandler = std::function<int32_t()>;

inline int DefaultPostReconnectHandler()
{
    return 0;
}
/**
 * 重连函数钩子，在通道断连后判断是否需要重连
 * 参数一： 远端节点ID
 * 参数二： 通道类型
 */
using IsReconnectHook = std::function<bool(std::string remoteNodeId, MxmChannelType type)>;
class MxmComEngineInfo {
public:
    MxmComEngineInfo() = default;

    MxmEngineType GetEngineType() const;

    void SetEngineType(MxmEngineType type);

    MxmProtocol GetProtocol() const;

    void SetProtocol(MxmProtocol proto);

    MxmWorkerMode GetWorkerMode() const;

    void SetWorkerMode(MxmWorkerMode mode);

    const std::string &GetNodeId() const;

    void SetNodeId(const std::string &id);

    const std::string &GetWorkGroup() const;

    void SetWorkGroup(const std::string &group);

    const std::string &GetName() const;

    void SetName(const std::string &engineName);

    void SetLogFunc(MxmComLogFunc func);

    MxmComLogFunc GetLogFunc() const;

    const RpcNode &GetIpInfo() const;

    void SetIpInfo(const RpcNode &ipInfo);

    const std::pair<std::string, uint16_t> &GetUdsInfo() const;

    void SetUdsInfo(const std::pair<std::string, uint16_t> &udsInfo);

    const uint32_t &GetMaxSendReceiveSize() const;

    void SetMaxSendReceiveSize(uint32_t maxSize);

    const uint32_t &GetSendReceiveSegCount() const;

    void SetSendReceiveSegCount(uint32_t segCount);

    bool IsUds() const;

    bool IsServerSide() const;

    const IsReconnectHook &GetReConnectHook() const;

    void SetReconnectHook(IsReconnectHook reconnectHook);

    const bool &GetEnableTls() const;

    void SetEnableTls(const bool &isEnable);

    const UBSHcomNetCipherSuite &GetCipherSuite() const;

    void SetCipherSuite(const UBSHcomNetCipherSuite &suite);

    uint32_t GetMaxHcomConnectNum() const;

    void SetMaxHcomConnectNum(const uint32_t maxNum);

private:
    MxmEngineType engineType{MxmEngineType::SERVER};           // 引擎类型
    MxmProtocol protocol{MxmProtocol::TCP};                    // 通信协议
    MxmWorkerMode workerMode{MxmWorkerMode::NET_BUSY_POLLING}; // hcom工作模式
    std::string nodeId;                                        // 节点标识
    RpcNode ipInfo{};                                          // TCP，RDMA模式下的ip端口信息
    std::pair<std::string, uint16_t> udsInfo{};
    std::string workGroup;                                      // hcom的workgroup配置信息
    std::string name;                                           // 引擎名
    MxmComLogFunc logFunc{};                                    // 注册给hcom的日志钩子函数
    uint32_t maxSendReceiveSize = DEFAULT_MAX_SENDRECEIVE_SIZE; // 大数据场景下，单次最大发送数据量大小，单位MB
    IsReconnectHook reconnectHook = nullptr;                    // 通道断连后重连钩子     //
        // 大数据场景下，单次最大发送数据量大小，单位MB
    bool enableTls = false; // 是否采用tls认证
    uint32_t sendReceiveSegCount = DEFAULT_SEND_RECEIVE_SEG_COUNT;
    UBSHcomNetCipherSuite cipherSuite = AES_GCM_128; // 加密套算法
    uint32_t maxHcomConnectNum{CONNECT_NUM};         // 默认256
};

class MxmComChannelConnectInfo {
public:
    MxmComChannelConnectInfo() = default;

    MxmComChannelConnectInfo(bool isUds, std::string ip, uint16_t port, std::string remoteNodeId, std::string curNodeId)
        : isUds(isUds),
          ip(std::move(ip)),
          port(port),
          remoteNodeId(std::move(remoteNodeId)),
          curNodeId(std::move(curNodeId)){};

    const std::string &GetIp() const;

    void SetIp(const std::string &ipAddr);

    uint16_t GetPort() const;

    void SetPort(uint16_t conPort);

    uint16_t GetLinkNum() const;

    void SetLinkNum(uint16_t num);

    bool IsUds() const;

    void SetIsUds(bool uds);

    const std::string &GetRemoteNodeId() const;

    void SetRemoteNodeId(const std::string &remoteNodeIdSet);

    const std::string &GetCurNodeId() const;

    void SetCurNodeId(const std::string &nodeId);

private:
    bool isUds{false};        // 是否是Uds协议
    std::string ip;           // 非uds模式下，为建连通道的远端节点Ip;uds模式为uds路径
    uint16_t port{0};         // 建连通道的远端节点端口
    std::string remoteNodeId; // 远程节点的Id
    std::string curNodeId;    // 当前节点Id
    uint16_t linkNum{1};      // 通道的链路数
};

class MxmComChannelInfo {
public:
    MxmComChannelInfo() = default;

    MxmComChannelInfo(bool isServer, MxmChannelType channelType, std::string engineName, UBSHcomChannelPtr channel,
                      MxmComChannelConnectInfo connectInfo)
        : isServer(isServer),
          channelType(channelType),
          engineName(std::move(engineName)),
          channel(std::move(channel)),
          connectInfo(std::move(connectInfo)){};

    ~MxmComChannelInfo()
    {
        channel = nullptr;
    }

    bool IsServerSide() const;

    void SetIsServer(bool isServerSide);

    const UBSHcomChannelPtr &GetChannel() const;

    void SetChannel(const UBSHcomChannelPtr &channel);

    const MxmComChannelConnectInfo &GetConnectInfo() const;

    void SetConnectInfo(const MxmComChannelConnectInfo &conInfo);

    MxmChannelType GetChannelType() const;

    void SetChannelType(MxmChannelType chType);

    void SetUDSInfo(UBSHcomNetUdsIdInfo &uds);

    UBSHcomNetUdsIdInfo GetUDSInfo() const;

    const std::string &GetEngineName() const;

    void SetEngineName(const std::string &name);

    std::string ConvertMxmComChannelInfoToString();

private:
    bool isServer{true};                                // 是否是server端
    MxmChannelType channelType{MxmChannelType::NORMAL}; // 通道类型
    std::string engineName;                             // 引擎名
    UBSHcomChannelPtr channel = nullptr;                // channel指针
    UBSHcomNetUdsIdInfo uds;
    MxmComChannelConnectInfo connectInfo; // 连接信息
};

/**
 * 参数一：引擎描述信息
 * 参数二：远端节点ID
 * 参数三：端口信息
 * 参数四：连接状态
 */
using MxmComLinkStateNotify =
    std::function<void(const MxmComEngineInfo &, const std::string &, uint64_t, MxmLinkState)>;
using MxmComMessagePtr = uint8_t *;

class MxmComMessageHead {
public:
    MxmComMessageHead() = default;

    uint16_t GetOpCode() const;

    void SetOpCode(uint16_t transOpCode);

    uint16_t GetModuleCode() const;

    void SetModuleCode(uint16_t transModuleCode);

    uint32_t GetBodyLen() const;

    void SetBodyLen(uint32_t transBodyLen);

    uint32_t GetCrc() const;

    void SetCrc(uint32_t dataCrc);

private:
    uint16_t opCode;     // 操作码
    uint16_t moduleCode; // 模块码
    uint32_t bodyLen;    // 消息体长度
    uint32_t crc;
};

class MxmComMessage {
public:
    MxmComMessage() = default;

    static MxmComMessagePtr AllocMessage(uint32_t len);

    static void FreeMessage(MxmComMessagePtr msg);

    void SetMessageHead(MxmComMessageHead &msgHead);

    const MxmComMessageHead &GetMessageHead() const;

    HRESULT SetMessageBody(const uint8_t *data, uint32_t len);

    uint8_t *GetMessageBody();

    uint32_t GetMessageBodyLen();

private:
    MxmComMessageHead head; // 消息头
    uint8_t body[0];        // 消息体
};

struct MxmUdsIdInfo {
    uint32_t pid = 0; // process id
    uint32_t uid = 0; // user id
    uint32_t gid = 0; // group id
};

class MxmComMessageCtx {
public:
    MxmComMessageCtx() = default;

    MxmComMessageCtx(MxmComMessagePtr message, std::string srcId, std::string dstId, MxmChannelType channelType);

    MxmComMessagePtr GetMessage() const;

    int SetMessage(MxmComMessagePtr comMessage, size_t length);

    uintptr_t GetRspCtx() const;

    void SetRspCtx(uintptr_t transRspCtx);

    uint64_t GetChannelId() const;

    void SetChannelId(uint64_t netChannel);

    const std::string &GetSrcId() const;

    void SetSrcId(const std::string &msgSrcId);

    const std::string &GetDstId() const;

    void SetDstId(const std::string &id);

    void SetChannelType(MxmChannelType chType);

    MxmChannelType GetChannelType();

    const std::string &GetEngineName() const;

    void SetEngineName(const std::string &egName);

    const MxmUdsIdInfo &GetUdsInfo() const;

    void SetUdsInfo(const MxmUdsIdInfo &uds);

    void SetReplyFuncHook(ReplyFuncHook replyHook);

    const ReplyFuncHook &GetReplyFuncHook() const;

    void FreeMessage();

private:
    MxmComMessagePtr message;   // 消息指针
    std::string srcId;          // 源节点Id
    std::string dstId;          // 目标节点Id
    uintptr_t rspCtx = 0;       // 放回消息上下文指针
    MxmChannelType channelType; // 消息通道类型
    uint64_t channelId = 0;     // 通道Id
    std::string engineName;     // 引擎名
    MxmUdsIdInfo udsInfo;
    ReplyFuncHook replyFuncHook = nullptr;
};

struct MxmComDataDesc {
    uint8_t *data = nullptr; // 消息指针
    uint32_t len = 0;        // 消息长度

    MxmComDataDesc() : data(nullptr), len(0) {}
    MxmComDataDesc(uint8_t *d, uint32_t l) : data(d), len(l) {}
};

/**
 * 消息发送回调函数
 * 参数一：上下文指针
 * 参数二：接受消息指针
 * 参数三：消息长度
 * 参数四：返回结果
 */
using MxmComAsyncMsgCbkHook = std::function<void(void *ctx, void *recv, uint32_t len, int32_t result)>;

struct MxmComCallback {
    MxmComAsyncMsgCbkHook cb;
    void *cbCtx;

    MxmComCallback(MxmComAsyncMsgCbkHook usrCb, void *usrCbCtx) : cb(std::move(usrCb)), cbCtx(usrCbCtx) {}
    MxmComCallback() : cb([](void *, void *, uint32_t, int32_t) {}), cbCtx(nullptr) {}
};

struct MxmComMsgHandler {
    uint16_t moduleCode;                           // 模块编码
    uint16_t opCode;                               // 操作码
    void (*handler)(MxmComMessageCtx &messageCtx); //  消息处理函数指针
};

MxmComMessagePtr TransRequestMsg(const NetMsgPacker &packer, uint16_t opCode, uint16_t moduleCode);

std::pair<std::string, MxmChannelType> SplitPayload(const std::string &payload);

bool GetIpPortByChannel(const UBSHcomChannelPtr &ch, std::string &ip, uint16_t &port);

MxmComMessage *GetMessageFromNetServiceContext(const UBSHcomServiceContext &context);

MxmComMessage *GetMessageFromNetServiceMessage(UBSHcomRequest &msg);

void GetUdsInfoFromNetServiceContext(const UBSHcomServiceContext &context, MxmUdsIdInfo &udsIdInfo);

bool CheckMessageBodyLen(UBSHcomServiceContext &context, MxmComMessage &msg);

bool CheckMessageBodyLen(UBSHcomRequest &netServiceMessage, MxmComMessage &msg);

uint64_t GetChannelIdFromNetServiceContext(const UBSHcomServiceContext &context);

MxmChannelType StringToChannelType(const std::string &type);

std::string ChannelTypeToString(MxmChannelType type);

std::string ChannelTypeToPayload(const std::string &id, MxmChannelType type);
} // namespace ock::com
#endif // MXM_COM_DEF_H
