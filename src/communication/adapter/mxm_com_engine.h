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
#ifndef MXM_COM_ENGINE_H
#define MXM_COM_ENGINE_H

#include <thread>
#include "hcom_service.h"
#include "lock/dg_lock.h"
#include "mxm_com_def.h"
#include "ubs_cryptor_handler.h"
#include "util/defines.h"

namespace ock::com {
using namespace ock::hcom;
using namespace ock::dagger;
using namespace ock::ubsm;

using HandlerMap = PairMap<uint16_t, uint16_t, MxmComMsgHandler>;
using NodeChannelMap = std::map<std::string, std::map<MxmChannelType, MxmComChannelInfo>>;
using ChannelIdMap = std::map<uint64_t, MxmComChannelInfo>;
using EngineHandlerWorker =
    std::function<void(void (*handler)(MxmComMessageCtx &messageCtx), MxmComMessageCtx &messageCtx)>;
class MxmComLinkManager {
public:
    MxmComLinkManager() = default;

    /* *
   * @brief 根据channel id获取channel
   *
   * @param[in] id: channel 的标识
   * @param[out] channelInfo: channel的描述信息
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT GetChannelByChannelId(uint64_t id, MxmComChannelInfo &channelInfo);

    /* *
   * @brief 根据远端节点Id获取channel
   *
   * @param[in] nodeId: 远端节点Id
   * @param[out] channelInfo: channel的描述信息
   * @param[in] chType: 通道类型
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT GetChannelByRemoteNodeId(const std::string &nodeId, MxmChannelType chType, MxmComChannelInfo &channelInfo);

    /* *
   * @brief 通道是否已经存在
   *
   * @param[in] nodeId: 远端节点Id
   * @param[in] chType: 通道类型
   * @return bool, 存在返回true, 失败返回false
   */
    bool IsChannelExists(const std::string &nodeId, MxmChannelType chType);

    /* *
   * @brief 新增Channel
   *
   * @param[in] channelInfo: Channel信息
   */
    void InsertChannel(MxmComChannelInfo &channelInfo);

    /* *
   * @brief 根据channel id移除channel
   *
   * @param[in] id: channel 的标识
   */
    void RemoveChannelByChannelId(uint64_t id, UBSHcomService *hcomNetService);

    void UpdateChannel(const std::string &nodeId, MxmChannelType chType);

    /* *
   * @brief 移除所有channel
   *
   */
    void RemoveAllChannel(UBSHcomService *hcomNetService);

    void SetStop();

    bool IsStopped();

private:
    void LogChannelInfo();
    NodeChannelMap nodeChannelMap;
    ChannelIdMap channelIdMap;
    bool isStopped{false};
};

class MxmComEngine {
public:
    MxmComEngine(MxmComEngineInfo engineInfo, UBSHcomService *hcomNetService, MxmComLinkStateNotify linkStateNotify,
                 MxmComLinkManager linkManager);

    /* *
   * @brief 获取引擎信息
   *
   * @return MxmComEngineInfo引用
   */
    const MxmComEngineInfo &GetEngineInfo() const;

    /* *
   * @brief 注册消息处理函数
   *
   * @param[in] handle: 消息处理函数
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT RegMxmComMsgHandler(const MxmComMsgHandler &handle);

    /* *
   * @brief 创建一个信息通道
   *
   * @param[in] info: channel连接信息
   * @param[in] chType: 通道类型
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT CreateChannel(MxmComChannelConnectInfo &info, MxmChannelType chType);

    /* *
   * @brief 获取消息通道
   *
   * @param[in] nodeId: 远端节点Id
   * @param[in] chType: 通道类型
   * @param[out] channelInfo: 通道信息
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT GetChannelByRemoteNodeId(const std::string &nodeId, MxmChannelType chType, MxmComChannelInfo &channelInfo);

    /* *
   * @brief 通过ChannelId获取Channel
   *
   * @param[in] channelId: channelId
   * @param[out] channelInfo: 通道信息
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT GetChannelById(uint64_t channelId, MxmComChannelInfo &channelInfo);

    /* *
   * @brief 获取消息处理函数
   *
   * @param[in] moduleCode: 模块编码
   * @param[in] opCode: 操作码
   * @return TransMessageHandler *, 成功返回函数指针, 失败返回非nullptr
   */
    MxmComMsgHandler *GetMessageHandler(uint16_t moduleCode, uint16_t opCode);

    /**
   * @brief 通过远端节点ID，通道类型删除channel
   * @param remoteNodeId [in] 远端节点id
   * @param type [int] 通道类型
   */
    void RemoveChannel(std::string remoteNodeId, MxmChannelType type);

    /* *
   * @brief 启动引擎
   */
    HRESULT Start();

    /* *
   * @brief 注册消息处理函数响应方式(同步/异步)
   */
    int RegisterHandlerWork(const EngineHandlerWorker &handlerWorker);

    /* *
   * @brief 停止引擎
   */
    void Stop();

    int SetMxmComPostReconnectHandler(MxmComPostReconnectHandler handler)
    {
        this->postReconnectHandler = std::move(handler);
        return 0;
    }
    MxmComPostReconnectHandler GetMxmComPostReconnectHandler()
    {
        return this->postReconnectHandler;
    }

    int ConvertContextToMessageCtx(const UBSHcomServiceContext &context, MxmComMessageCtx &msgCtx);

protected:
    /**
   * @brief 大数据通信，预申请内存
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT PrepareMem();
    /* *
   * @brief 注册引擎消息处理函数
   */
    void RegisterEngineHandlers();

    HRESULT RegisterTlsCallback();

    bool TlsCaCallBack(const std::string &name, std::string &capath, std::string &crlPath,
                       UBSHcomPeerCertVerifyType &verifyPeerCert, UBSHcomTLSCertVerifyCallback &cb);

    bool TlsPrivateKeyCallback(const std::string &name, std::string &path, void *&pw, int &length,
                               UBSHcomTLSEraseKeypass &erase);

    bool TlsCertificationCallback(const std::string &name, std::string &path);

    void TlsEraseKeypass(void *addr, int len);

    /* *
   * @brief 新连接建立消息处理函数
   * @param[in] ipPort: 远端ip，端口信息
   * @param[in] ch: 通道指针
   * @param[in] payload: 远端标识
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT NewChannel(const std::string &ipPort, const UBSHcomChannelPtr &ch, const std::string &payload);

    /* *
   * @brief 通道断开消息处理函数
   * @param[in] ch: 通道指针
   */
    void BrokenChannel(const UBSHcomChannelPtr &ch);

    /* *
   * @brief 通信消息处理函数
   * @param[in] context: 消息上下文
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT ReceivedRequest(UBSHcomServiceContext &context);

    /* *
   * @brief 通信消息发送完成处理函数
   * @param[in] context: 消息上下文
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT SendRequest(const UBSHcomServiceContext &context);

    /* *
   * @brief 单边消息发送完成处理函数
   * @param[in] context: 消息上下文
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    HRESULT OneSideDoneRequest(const UBSHcomServiceContext &context);

    /* *
   * @brief 通道重连
   * @param[in] channelInfo: 通道连接信息
   */
    void DoReconnect(const MxmComChannelInfo &channelInfo);

    /* *
   * @brief 添加引擎监听信息
   */
    void AddListenOptions();

protected:
    MxmComEngineInfo engineInfo;              // 引擎信息
    UBSHcomService *hcomNetService = nullptr; // hcom service实例
    MxmComLinkStateNotify linkStateNotify;    // 通道状态变更回调函数
    MxmComPostReconnectHandler postReconnectHandler = DefaultPostReconnectHandler;
    std::atomic<bool> deleted{false}; // 引擎是否销毁
    MxmComLinkManager linkManager;    // 连接通道管理器
    HandlerMap handlerMap{};          // 操作函数映射表，一个引擎一个表
    EngineHandlerWorker handlerWork;
    ReadWriteLock rwLock;                          // 读写锁
    void *address = nullptr;                       // 预分配内存地址
    UBSHcomNetMemoryAllocatorPtr memPtr = nullptr; // 内存指针
    UBSHcomRegMemoryRegion mr;
};

class MxmComEngineManager {
public:
    /* *
   * @brief 创建引擎
   *
   * @param[in] engineInfo: 引擎信息
   * @param[in] notify: 连接状态变更回调接口
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT CreateEngine(const MxmComEngineInfo &engineInfo, const MxmComLinkStateNotify &notify,
                                const EngineHandlerWorker &handlerWorker);

    /* *
   * @brief 根据引擎名销毁引擎实例
   *
   */
    static void DeleteEngine(const std::string &name);

    /* *
   * @brief 销毁所有引擎
   *
   */
    static void DeleteAllEngine();

    /* *
   * @brief 根据引擎名获取获取引擎实例
   *
   * @return MxmComEngine 指针
   */
    static MxmComEngine *GetEngine(const std::string &name);

private:
    static std::map<std::string, MxmComEngine *> G_ENGINE_MAP; // 引擎全局映射表
    static std::mutex G_MUTEX;
};

class MxmCommunication {
public:
    /* *
   * @brief 创建传输引擎
   *
   * @param[in] engineInfo: 引擎信息
   * @param[in] notify: 连接状态变更回调接口
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT CreateMxmComEngine(const MxmComEngineInfo &engine, const MxmComLinkStateNotify &notify,
                                      const EngineHandlerWorker &handlerWorker);
    /* *
   * @brief 根据引擎名，销毁引擎
   *
   * @param[in] name: 引擎名
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static void DeleteMxmComEngine(const std::string &name);

    /* *
   * @brief 与Server端建立Rpc通道
   *
   * @param[in] engineName: 引擎名
   * @param[in] nodeId: (ip,port)
   * @param[in] nodeIds: (当前节点Id，远端节点Id)
   * @param[in] chType: 通道类型
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT MxmComRpcConnect(const std::string &engineName, const RpcNode &remoteNodeId,
                                    const std::string &nodeId, MxmChannelType chType = MxmChannelType::NORMAL);

    /* *
   * @brief 创建Ipc传输引擎
   *
   * @param[in] engineName: 引擎名
   * @param[in] udsPath: uds路径
   * @param[in] nodeId: 当前节点Id
   * @param[in] chType: 通道类型
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT MxmComIpcConnect(const std::string &engineName, const std::string &udsPath,
                                    const std::string &nodeId, MxmChannelType chType = MxmChannelType::NORMAL);

    /* *
   * @brief 创建Rpc传输引擎
   *
   * @param[in] engineName: 引擎名
   * @param[in] handle: 消息处理函数定义
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT RegMxmComMsgHandler(const std::string &engineName, const MxmComMsgHandler &handle);

    /* *
   * @brief 同步发消息
   *
   * @param[in] engineName: 引擎名
   * @param[in] message: 消息上下文
   * @param[out] retData: 返回体数据
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static HRESULT MxmComMsgSend(const std::string &engineName, MxmComMessageCtx &message, MxmComDataDesc &retData);

    /* *
   * @brief 回复消息
   *
   * @param[in] message: 消息上下文
   * @param[out] retData: 返回体数据
   * @param[in] usrCb: 回调函数
   * @return HRESULT, 成功返回0, 失败返回非0
   */
    static void MxmComMsgReply(MxmComMessageCtx &message, const MxmComDataDesc &data,
                               const MxmComCallback &usrCb = MxmComCallback());

    /**
   * @brief 通过engine名称，对端节点id，通道类型删除引擎
   * @param engineName [in] 引擎名称
   * @param remoteNodeId [in] 对端节点id
   * @param type [in] 通道引擎
   */
    static void RemoveChannel(const std::string &engineName, const std::string &remoteNodeId, MxmChannelType type);

    static int SetPostReconnectHandler(const std::string &name, MxmComPostReconnectHandler handler);
};
} // namespace ock::com
#endif // MXM_COM_ENGINE_H
