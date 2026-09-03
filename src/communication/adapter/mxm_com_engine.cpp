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
#include "mxm_com_engine.h"
#include <fcntl.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include "crc/dg_crc.h"
#include "log.h"
#include "ptracer.h"
#include "ubs_common_config.h"
#include "ubsm_com_constants.h"
#include "ubsm_ptracer.h"

namespace ock::com {
using namespace ock::common;
using namespace ock::ubsm;

static std::string GetUdsPath(const std::string &udsName)
{
    std::string udsPath = MXM_IPC_UDS_PATH_PREFIX_DEFAULT + udsName;
    return udsPath;
}

static void Logger(int level, const char *str)
{
    switch (level) {
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

void MxmComLinkManager::LogChannelInfo()
{
    DBG_LOGDEBUG("---------Log All Channel Info-----------");
    for (auto &item : nodeChannelMap) {
        std::string debugInfo = "Node:" + item.first + ": ";
        for (auto &channel : item.second) {
            debugInfo += channel.second.ConvertMxmComChannelInfoToString();
        }
        DBG_LOGDEBUG(debugInfo);
    }
    for (auto &item : channelIdMap) {
        std::string debugInfo;
        debugInfo += item.second.ConvertMxmComChannelInfoToString();
        DBG_LOGDEBUG(debugInfo);
    }
}

HRESULT MxmComLinkManager::GetChannelByChannelId(uint64_t id, MxmComChannelInfo &channelInfo)
{
    auto iter = channelIdMap.find(id);
    if (iter == channelIdMap.end()) {
        DBG_LOGWARN("No channel " << id);
        LogChannelInfo();
        return MXM_COM_ERROR_CHANNEL_NOT_FOUND;
    }
    channelInfo = iter->second;
    return HOK;
}

HRESULT MxmComLinkManager::GetChannelByRemoteNodeId(const std::string &nodeId, MxmChannelType chType,
                                                    MxmComChannelInfo &channelInfo)
{
    auto iter = nodeChannelMap.find(nodeId);
    if (iter == nodeChannelMap.end()) {
        DBG_LOGERROR("No channel for node " << nodeId);
        LogChannelInfo();
        return MXM_COM_ERROR_CHANNEL_NOT_FOUND;
    }

    auto chTypeIter = iter->second.find(chType);
    if (chTypeIter == iter->second.end()) {
        DBG_LOGERROR("No channel for node " << nodeId << " type is " << static_cast<int>(chType));
        LogChannelInfo();
        return MXM_COM_ERROR_CHANNEL_NOT_FOUND;
    }
    channelInfo = chTypeIter->second;
    return HOK;
}

bool MxmComLinkManager::IsChannelExists(const std::string &nodeId, MxmChannelType chType)
{
    auto iter = nodeChannelMap.find(nodeId);
    if (iter == nodeChannelMap.end()) {
        return false;
    }
    auto chTypeIter = iter->second.find(chType);
    if (chTypeIter == iter->second.end()) {
        return false;
    }
    return true;
}
void MxmComLinkManager::UpdateChannel(const std::string &nodeId, MxmChannelType chType)
{
    auto iter = nodeChannelMap.find(nodeId);
    if (iter != nodeChannelMap.end()) {
        nodeChannelMap[nodeId].erase(chType);
    }
}

void MxmComLinkManager::SetStop()
{
    isStopped = true;
    DBG_LOGDEBUG("Set varible isStop, isStop=true");
}

bool MxmComLinkManager::IsStopped()
{
    return isStopped;
}

void MxmComLinkManager::InsertChannel(MxmComChannelInfo &channelInfo)
{
    auto chType = channelInfo.GetChannelType();
    if (chType == MxmChannelType::SINGLE_SIDE && channelInfo.IsServerSide()) {
        auto channelId = channelInfo.GetChannel()->GetId();
        channelIdMap.emplace(channelId, channelInfo);
        DBG_LOGINFO("Insert channel id: " << channelId << ", cur node id" << channelInfo.GetConnectInfo().GetCurNodeId()
                                          << ", remote node id" << channelInfo.GetConnectInfo().GetRemoteNodeId());
        return;
    }
    const auto &nodeId = channelInfo.GetConnectInfo().GetRemoteNodeId();
    auto engineName = channelInfo.GetEngineName();
    if (IsChannelExists(nodeId, chType)) {
        DBG_LOGINFO("Engine " << engineName << " channel already exists, type is " << static_cast<uint16_t>(chType)
                              << ", remote node is " << nodeId);
        UpdateChannel(nodeId, chType);
    }
    auto iter = nodeChannelMap.find(nodeId);
    if (iter == nodeChannelMap.end()) {
        std::map<MxmChannelType, MxmComChannelInfo> map;
        map.emplace(chType, channelInfo);
        nodeChannelMap.emplace(nodeId, map);
    } else {
        iter->second.emplace(chType, channelInfo);
    }
    auto channelId = channelInfo.GetChannel()->GetId();
    channelIdMap.emplace(channelId, channelInfo);
    DBG_LOGINFO("Insert channel id: " << channelId << ", cur node id" << channelInfo.GetConnectInfo().GetCurNodeId()
                                      << ", remote node id" << channelInfo.GetConnectInfo().GetRemoteNodeId());
}

void MxmComLinkManager::RemoveChannelByChannelId(uint64_t id, UBSHcomService *hcomNetService)
{
    auto iter = channelIdMap.find(id);
    if (iter == channelIdMap.end()) {
        DBG_LOGERROR("No channel " << id << " ,can't remove");
        return;
    }
    auto chPtr = iter->second.GetChannel();
    auto remoteId = iter->second.GetConnectInfo().GetRemoteNodeId();
    auto chType = iter->second.GetChannelType();
    auto engineName = iter->second.GetEngineName();
    auto chTypeIter = nodeChannelMap.find(remoteId);
    if (chTypeIter != nodeChannelMap.end()) {
        chTypeIter->second.erase(chType);
    }

    channelIdMap.erase(id);
    if (chPtr == nullptr) {
        DBG_LOGERROR("Channel " << id << " hcom NetChannelPtr is nullptr");
        return;
    }
    if (hcomNetService != nullptr) {
        hcomNetService->Disconnect(chPtr);
    }
}

void MxmComLinkManager::RemoveAllChannel(UBSHcomService *hcomNetService)
{
    std::vector<uint64_t> ids;
    for (const auto &id : channelIdMap) {
        ids.push_back(id.first);
    }
    for (auto id : ids) {
        RemoveChannelByChannelId(id, hcomNetService);
    }
}

std::map<std::string, MxmComEngine *> MxmComEngineManager::G_ENGINE_MAP;
std::mutex MxmComEngineManager::G_MUTEX;

static void HandlerWorkDefault(void (*handler)(MxmComMessageCtx &messageCtx), MxmComMessageCtx &messageCtx)
{
    if (handler != nullptr) {
        handler(messageCtx);
    }
}

MxmComEngine::MxmComEngine(MxmComEngineInfo engineInfo, UBSHcomService *hcomNetService,
                           MxmComLinkStateNotify linkStateNotify, MxmComLinkManager linkManager)
    : engineInfo(std::move(engineInfo)),
      hcomNetService(hcomNetService),
      linkStateNotify(std::move(linkStateNotify)),
      linkManager(std::move(linkManager))
{
}

const MxmComEngineInfo &MxmComEngine::GetEngineInfo() const
{
    return engineInfo;
}

HRESULT MxmComEngine::RegMxmComMsgHandler(const MxmComMsgHandler &handle)
{
    rwLock.LockWrite();
    if (handle.moduleCode >= MODULES_SIZE || handle.opCode >= OP_CODE_SIZE) {
        DBG_LOGERROR("Invalid module code or op code, module code is " << handle.moduleCode << ", op code is "
                                                                       << handle.opCode);
        rwLock.UnLock();
        return MXM_COM_ERROR_MESSAGE_INVALID_OP_CODE;
    }
    handlerMap[{handle.moduleCode, handle.opCode}] = handle;
    rwLock.UnLock();
    return HOK;
}

constexpr uint32_t SEND_RECEIVE_SIZE = 32 * 1024;
constexpr uint32_t SEND_RECEIVE_COUNT = 128;
constexpr uint32_t SEND_QUEUE_SIZE = 4096;
constexpr uint32_t CTX_STORE_CAP = 1024;
constexpr uint32_t MIN_BLOCK_SIZE = 4096;
constexpr uint32_t CACHE_TIER_COUNT = 10;
constexpr uint32_t CACHE_BLOCK_COUNT_PER_TIER = 8;
constexpr uint32_t BUCKET_COUNT = 8;
constexpr uint32_t QUEUE_SIZE = 2048;
constexpr uint32_t POST_RECEIVE_SIZE = 32;
constexpr uint32_t POLLING_SIZE = 1024;
constexpr uint32_t POST_SEND_COUNT = 1024;
constexpr uint32_t HB_IDLE_TIME = 2;
constexpr uint32_t MAX_SEND_RECV_DATA_COUNT = 256;
const std::string DEFAULT_DEVICE_IP_MASK = "127.0.0.1/24";
const std::string DEFAULT_WORKER_GROUP = "8";
constexpr int WORKER_THREAD_PRIORITY = -20;
constexpr uint16_t WORKER_THREAD_COUNT = 6;

static void AssignServiceOptions(const MxmComEngineInfo &engineInfo, UBSHcomServiceOptions &options)
{
    options.workerGroupMode = static_cast<UBSHcomWorkerMode>(engineInfo.GetWorkerMode());
    options.workerGroupThreadCount = WORKER_THREAD_COUNT;
    options.maxSendRecvDataSize = engineInfo.GetMaxSendReceiveSize() * SEND_RECEIVE_SIZE;
}

static void AssignHcomServiceOptions(const MxmComEngineInfo &engineInfo, UBSHcomService *hcomNetService)
{
    hcomNetService->SetConnectLBPolicy(NET_ROUND_ROBIN);
    hcomNetService->SetSendQueueSize(SEND_QUEUE_SIZE);
    hcomNetService->SetRecvQueueSize(QUEUE_SIZE);
    hcomNetService->SetQueuePrePostSize(POST_RECEIVE_SIZE);
    hcomNetService->SetCtxStoreCapacity(CTX_STORE_CAP);
    hcomNetService->SetMaxConnectionCount(engineInfo.GetMaxHcomConnectNum());
    hcomNetService->SetMaxSendRecvDataCount(MAX_SEND_RECV_DATA_COUNT);
    UBSHcomHeartBeatOptions opt;
    opt.heartBeatIdleSec = HB_IDLE_TIME;
    hcomNetService->SetHeartBeatOptions(opt);

    std::vector<std::string> ipMasks;
    if ((engineInfo.GetEngineType() == MxmEngineType::SERVER) &&
        (engineInfo.GetProtocol() == MxmProtocol::TCP || engineInfo.GetProtocol() == MxmProtocol::HCCS ||
         engineInfo.GetProtocol() == MxmProtocol::UB)) {
        ipMasks.push_back(engineInfo.GetIpInfo().ip + "/24");
    } else {
        ipMasks.push_back(DEFAULT_DEVICE_IP_MASK);
    }
    hcomNetService->SetDeviceIpMask(ipMasks);
}

const int16_t DEFAULT_REQUEST_TIMEOUT = 60;
const int16_t DEFAULT_HEARTBEAT_REQUEST_TIMEOUT = 0;
const auto ENV_COM_CHANNEL_TIMEOUT = "MXM_CHANNEL_TIMEOUT";

static int16_t GetIPCTimeOutFromEnv()
{
    auto envValue = std::getenv(ENV_COM_CHANNEL_TIMEOUT);
    uint64_t timeOut;
    if (envValue != nullptr && StrUtil::StrToULong(envValue, timeOut)) {
        DBG_LOGINFO("IPC timeout from varible, time=" << timeOut << "s");
        return static_cast<int16_t>(timeOut);
    }
    DBG_LOGINFO("IPC timeout from default, time=" << DEFAULT_REQUEST_TIMEOUT << "s");
    return DEFAULT_REQUEST_TIMEOUT;
}

HRESULT MxmComEngine::CreateChannel(MxmComChannelConnectInfo &info, MxmChannelType chType)
{
    DBG_LOGDEBUG("Start to create channel, remote node id=" << info.GetRemoteNodeId());
    UBSHcomConnectOptions options{};
    options.linkCount = info.GetLinkNum();
    auto engineName = engineInfo.GetName();
    if (hcomNetService == nullptr) {
        DBG_LOGERROR("Failed to create channel, engine " << engineName << "has not been initilized.");
        return MXM_COM_ERROR_ENGINE_NOT_INIT;
    }
    auto &remoteNodeId = info.GetRemoteNodeId();
    if (linkManager.IsChannelExists(remoteNodeId, chType)) {
        DBG_LOGDEBUG("Engine channel" << engineName
                                      << " has been already existed, type=" << static_cast<uint16_t>(chType));
        return HOK;
    }
    UBSHcomChannelPtr channelPtr;
    options.payload = ChannelTypeToPayload(info.GetCurNodeId(), chType);
    HRESULT ret;
    if (engineInfo.GetProtocol() == MxmProtocol::UDS) {
        ret = hcomNetService->Connect("uds://" + info.GetIp(), channelPtr, options);
    } else {
        ret = hcomNetService->Connect("tcp://" + info.GetRemoteNodeId(), channelPtr, options);
    }
    if (ret != 0) {
        DBG_LOGERROR("Failed to connect remote, peer node=" << info.GetIp() << ":" << info.GetPort()
                                                            << ", ret=" << ret);
        return MXM_COM_ERROR_CONNECT_TO_PEER_FAIL;
    }
    if (channelPtr == nullptr) {
        DBG_LOGERROR("Failed to create channel, pointer of channel is null");
        return HFAIL;
    }
    DBG_LOGINFO("Create channel successfully, engine=" << engineName << ", channel id=" << channelPtr->GetId());
    if (chType == MxmChannelType::HEARTBEAT) {
        channelPtr->SetChannelTimeOut(DEFAULT_HEARTBEAT_REQUEST_TIMEOUT, DEFAULT_HEARTBEAT_REQUEST_TIMEOUT);
    } else {
        channelPtr->SetChannelTimeOut(GetIPCTimeOutFromEnv(), GetIPCTimeOutFromEnv());
    }
    MxmComChannelInfo chInfo(false, chType, engineName, channelPtr, info);

    if (engineInfo.GetProtocol() == MxmProtocol::UDS && engineInfo.IsServerSide()) {
        UBSHcomNetUdsIdInfo uds;
        channelPtr->GetRemoteUdsIdInfo(uds);
        chInfo.SetUDSInfo(uds);
    }
    rwLock.LockWrite();
    linkManager.InsertChannel(chInfo);
    rwLock.UnLock();
    linkStateNotify(engineInfo, remoteNodeId, channelPtr->GetId(), MxmLinkState::LINK_UP);
    return HOK;
}

HRESULT MxmComEngine::GetChannelByRemoteNodeId(const std::string &nodeId, MxmChannelType chType,
                                               MxmComChannelInfo &channelInfo)
{
    rwLock.LockRead();
    auto ret = linkManager.GetChannelByRemoteNodeId(nodeId, chType, channelInfo);
    rwLock.UnLock();
    return ret;
}

MxmComMsgHandler *MxmComEngine::GetMessageHandler(uint16_t moduleCode, uint16_t opCode)
{
    rwLock.LockRead();
    if (moduleCode >= MODULES_SIZE || opCode >= OP_CODE_SIZE) {
        DBG_LOGERROR("Invalid module code or op code, module code is " << moduleCode << ", op code is " << opCode);
        rwLock.UnLock();
        return nullptr;
    }
    MxmComMsgHandler *hdl = &handlerMap[{moduleCode, opCode}];
    rwLock.UnLock();
    return hdl;
}

void MxmComEngine::RemoveChannel(std::string remoteNodeId, MxmChannelType type)
{
    MxmComChannelInfo channelInfo{};
    HRESULT ret = linkManager.GetChannelByRemoteNodeId(remoteNodeId, type, channelInfo);
    if (ret != HOK) {
        DBG_LOGERROR("Failed to get channel info, error code is," << ret);
        return;
    }
    linkManager.RemoveChannelByChannelId(channelInfo.GetChannel()->GetId(), hcomNetService);
}

HRESULT MxmComEngine::PrepareMem()
{
    address = memalign(NN_NO4096, engineInfo.GetMaxSendReceiveSize() * SEND_RECEIVE_SIZE);
    if (address == nullptr) {
        DBG_LOGERROR("Failed to alloc memory, maybe lack of spare memory in system.");
        return HFAIL;
    }

    UBSHcomNetMemoryAllocatorOptions options;
    options.address = reinterpret_cast<uintptr_t>(address);
    options.size = engineInfo.GetMaxSendReceiveSize() * SEND_RECEIVE_SIZE;
    options.minBlockSize = MIN_BLOCK_SIZE;
    options.alignedAddress = true;
    options.cacheTierCount = CACHE_TIER_COUNT;
    options.cacheBlockCountPerTier = CACHE_BLOCK_COUNT_PER_TIER;
    options.bucketCount = BUCKET_COUNT;
    options.cacheTierPolicy = TIER_POWER;
    auto result = UBSHcomNetMemoryAllocator::Create(ock::hcom::DYNAMIC_SIZE_WITH_CACHE, options, memPtr);
    if (result != 0) {
        DBG_LOGERROR("Failed to create mem allocator, " << result);
    }
    return result;
}

static bool IsDirectory(const std::string &dir)
{
    struct stat info {
    };
    if (stat(dir.c_str(), &info) != 0) {
        return false; // 无法访问路径
    }
    return S_ISDIR(info.st_mode); // 检查是否为目录
}

static bool MkdirRecursive(const std::string &path, mode_t mode = 0755)
{
    size_t pos = 0;
    std::string dir;
    if (path[0] == '/') {
        dir = "/";
        pos = 1;
    }
    while ((pos = path.find('/', pos)) != std::string::npos) {
        dir = path.substr(0, pos);
        pos++;

        if (mkdir(dir.c_str(), mode) != 0) {
            if (errno != EEXIST) {
                return false;
            }
        }
    }
    if (mkdir(path.c_str(), mode) != 0) {
        return errno == EEXIST;
    }

    return true;
}

static HRESULT CheckAndCreateUdsPath(const MxmComEngineInfo &engineInfo)
{
    if (engineInfo.GetEngineType() != MxmEngineType::CLIENT && engineInfo.IsUds()) {
        if (!IsDirectory(MXM_IPC_UDS_PATH_PREFIX_DEFAULT)) { // 检查是否为目录
            if (MkdirRecursive(MXM_IPC_UDS_PATH_PREFIX_DEFAULT, UDS_PATH_PERM)) {
                DBG_LOGINFO("Success to create directory: " << MXM_IPC_UDS_PATH_PREFIX_DEFAULT);
            } else {
                DBG_LOGERROR("Failed to create directory: " << MXM_IPC_UDS_PATH_PREFIX_DEFAULT << " errno=" << errno);
                return HFAIL;
            }
        }
    }
    return HOK;
}

HRESULT MxmComEngine::Start()
{
    HRESULT ret = HOK;
    auto engineName = engineInfo.GetName();
    AssignHcomServiceOptions(engineInfo, hcomNetService);
    RegisterEngineHandlers();
    ret = RegisterTlsCallback();
    if (ret != HOK) {
        DBG_LOGERROR("Failed to register tls callback, " << ret);
        return ret;
    }
    ret = CheckAndCreateUdsPath(engineInfo);
    if (ret != HOK) {
        DBG_LOGERROR("Fail to create uds path. path: " << MXM_IPC_UDS_PATH_PREFIX_DEFAULT);
        return ret;
    }
    ret = hcomNetService->Start();
    if (ret != 0) {
        DBG_LOGERROR("Create engine " << engineName << " failed, start service fail");
        hcomNetService->Destroy(engineName);
        return MXM_COM_ERROR_ENGINE_START_FAIL;
    }

    if (engineInfo.GetEngineType() != MxmEngineType::CLIENT && engineInfo.IsUds()) {
        // 设置uds文件权限
        const std::string udsPath = GetUdsPath(engineInfo.GetUdsInfo().first);
        if (chmod(udsPath.c_str(), engineInfo.GetUdsInfo().second) != 0) {
            DBG_LOGERROR("Failed to change uds file permission, " << strerror(errno));
            return HFAIL;
        }
    }

    if (engineInfo.IsUds()) {
        HRESULT retCode = PrepareMem();
        if (retCode != 0) {
            DBG_LOGERROR("Failed to prepare rndv mem, " << retCode);
            return retCode;
        }
        auto result = hcomNetService->RegisterMemoryRegion(reinterpret_cast<uintptr_t>(address),
                                                           engineInfo.GetMaxSendReceiveSize() * SEND_RECEIVE_SIZE, mr);
        if (result != 0) {
            DBG_LOGERROR("Failed to register mem region, " << result);
            return result;
        }
        UBSHcomMemoryKey key{};
        mr.GetMemoryKey(key);
        memPtr->MrKey(key.keys[0]);
    }
    return HOK;
}

int MxmComEngine::RegisterHandlerWork(const EngineHandlerWorker &handlerWorker)
{
    if (handlerWorker == nullptr) {
        DBG_LOGINFO("HandlerWorker is empty");
        return MXM_COM_ERROR_CHANNEL_NULL;
    }
    handlerWork = handlerWorker;
    return HOK;
}

void MxmComEngine::Stop()
{
    rwLock.LockWrite();
    linkManager.SetStop();
    auto engineName = engineInfo.GetName();
    linkManager.RemoveAllChannel(hcomNetService);
    rwLock.UnLock();
    if (hcomNetService != nullptr) {
        hcomNetService->DestroyMemoryRegion(mr);
        auto ret = hcomNetService->Destroy(engineName);
        DBG_LOGINFO("Destroy hcom instance=" << engineName << " finish, ret=" << ret);
        hcomNetService = nullptr;
        deleted.store(true);
    }
    if (address != nullptr) {
        free(address);
        address = nullptr;
    }
}

void MxmComEngine::RegisterEngineHandlers()
{
    bool isOobSvr = engineInfo.GetEngineType() != MxmEngineType::CLIENT;
    if (isOobSvr) {
        UBSHcomServiceNewChannelHandler newChannelHandler = [this](const std::string &addr, const UBSHcomChannelPtr &ch,
                                                                   const std::string &payload) -> HRESULT {
            if (UNLIKELY(ch == nullptr)) {
                return MXM_COM_ERROR_CHANNEL_NULL;
            }
            return NewChannel(addr, ch, payload);
        };
        if (engineInfo.IsUds()) {
            hcomNetService->Bind("uds://" + GetUdsPath(engineInfo.GetUdsInfo().first) + ":" +
                                     std::to_string(engineInfo.GetUdsInfo().second),
                                 newChannelHandler);
        } else {
            hcomNetService->Bind(
                "tcp://" + engineInfo.GetIpInfo().ip + ":" + std::to_string(engineInfo.GetIpInfo().port),
                newChannelHandler);
        }
    }
    UBSHcomServiceChannelBrokenHandler channelBrokenHandler = [this](const UBSHcomChannelPtr &ch) {
        BrokenChannel(ch);
    };
    hcomNetService->RegisterChannelBrokenHandler(channelBrokenHandler, UBSHcomChannelBrokenPolicy::BROKEN_ALL);
    UBSHcomServiceRecvHandler receivedHandler = [this](UBSHcomServiceContext &context) -> HRESULT {
        return ReceivedRequest(context);
    };
    hcomNetService->RegisterRecvHandler(receivedHandler);
    UBSHcomServiceSendHandler serviceSendHandler = [this](const UBSHcomServiceContext &context) -> HRESULT {
        return SendRequest(context);
    };
    hcomNetService->RegisterSendHandler(serviceSendHandler);
    UBSHcomServiceOneSideDoneHandler oneSideDoneRequest = [this](const UBSHcomServiceContext &context) -> HRESULT {
        return OneSideDoneRequest(context);
    };
    hcomNetService->RegisterOneSideHandler(oneSideDoneRequest);
}

HRESULT MxmComEngine::RegisterTlsCallback()
{
    if (!engineInfo.GetEnableTls()) {
        UBSHcomTlsOptions tlsOptions;
        tlsOptions.enableTls = false;
        hcomNetService->SetTlsOptions(tlsOptions);
        return 0;
    }

    auto ret = UbsCryptorHandler::SetCryptorLogger(Logger);
    if (ret != 0) {
        DBG_LOGERROR("Set cryptor logger failed, ret: " << ret);
        return ret;
    }

    UBSHcomTLSCaCallback caCb = [this](const std::string &name, std::string &capath, std::string &crlPath,
                                       UBSHcomPeerCertVerifyType &verifyPeerCert,
                                       UBSHcomTLSCertVerifyCallback &cb) -> bool {
        return TlsCaCallBack(name, capath, crlPath, verifyPeerCert, cb);
    };
    UBSHcomTLSCertificationCallback cfCb = [this](const std::string &name, std::string &path) -> bool {
        return TlsCertificationCallback(name, path);
    };
    UBSHcomTLSPrivateKeyCallback pkCb = [this](const std::string &name, std::string &path, void *&password, int &length,
                                               UBSHcomTLSEraseKeypass &erase) -> bool {
        return TlsPrivateKeyCallback(name, path, password, length, erase);
    };

    UBSHcomTlsOptions tlsOptions;
    tlsOptions.caCb = caCb;
    tlsOptions.cfCb = cfCb;
    tlsOptions.pkCb = pkCb;
    tlsOptions.netCipherSuite = engineInfo.GetCipherSuite();
    tlsOptions.enableTls = true;
    hcomNetService->SetTlsOptions(tlsOptions);
    DBG_LOGINFO("Register tls callback finished.");
    return 0;
}

bool MxmComEngine::TlsCaCallBack(const std::string &name, std::string &capath, std::string &crlPath,
                                 UBSHcomPeerCertVerifyType &verifyPeerCert, UBSHcomTLSCertVerifyCallback &cb)
{
    capath = UbsCommonConfig::GetInstance().GetCaPath();
    if (!UbsCommonConfig::GetInstance().GetCrlPath().empty()) {
        crlPath = UbsCommonConfig::GetInstance().GetCrlPath();
    }
    verifyPeerCert = UBSHcomPeerCertVerifyType::VERIFY_BY_DEFAULT;
    cb = nullptr;
    DBG_LOGINFO("TLS get ca path finished, peer name: " << name);
    return true;
}

bool MxmComEngine::TlsPrivateKeyCallback(const std::string &name, std::string &path, void *&pw, int &length,
                                         UBSHcomTLSEraseKeypass &erase)
{
    path = UbsCommonConfig::GetInstance().GetKeyPath();
    DBG_LOGINFO("key.path=" << path);
    std::pair<char *, int> pwPair;
    auto ret = UbsCryptorHandler::GetInstance().Decrypt(0, UbsCommonConfig::GetInstance().GetKeypassPath(), pwPair);
    if (ret != 0) {
        DBG_LOGERROR("Decrypt failed, ret: " << ret << ", peer name: " << name);
        return false;
    }
    pw = static_cast<void *>(pwPair.first);
    length = pwPair.second;

    UBSHcomTLSEraseKeypass eraseFunc = [this](void *addr, int len) -> void {
        TlsEraseKeypass(addr, len);
    };
    erase = eraseFunc;
    DBG_LOGINFO("TLS get private key path finished, peer name: " << name);
    return true;
}

bool MxmComEngine::TlsCertificationCallback(const std::string &name, std::string &path)
{
    path = UbsCommonConfig::GetInstance().GetCertPath();
    DBG_LOGINFO("TLS get cert path finished, peer name: " << name);
    return true;
}

void MxmComEngine::TlsEraseKeypass(void *addr, int len)
{
    UbsCryptorHandler::GetInstance().EraseDecryptData(static_cast<char *>(addr), len);
    DBG_LOGINFO("TLS erase key pass finished.");
}

HRESULT MxmComEngine::NewChannel(const std::string &ipPort, const UBSHcomChannelPtr &ch, const std::string &payload)
{
    if (linkManager.IsStopped()) {
        DBG_LOGERROR("linkManager is stopping, cannot new channel.");
        return MXM_COM_ERROR_CHANNEL_NULL;
    }
    const auto &engineName = engineInfo.GetName();
    bool isUds = engineInfo.IsUds();
    DBG_LOGINFO("Engine " << engineName << " get new channel");
    if (ch == nullptr) {
        DBG_LOGERROR("NetChannelPtr of NewChannel is nullptr ipPort is: " << ipPort << " payload is: " << payload);
        return MXM_COM_ERROR_CHANNEL_NULL;
    }
    std::pair<std::string, MxmChannelType> payLoadPair = SplitPayload(payload);
    if (payLoadPair.first.empty()) {
        DBG_LOGERROR("Empty payLoad pair name is: " << payload);
        return MXM_COM_ERROR_INVALID_CHANNEL_TYPE;
    }
    auto channelId = ch.Get()->GetId();
    DBG_LOGINFO("New channel " << channelId << " from: " << ipPort << ", payload is: " << payload);
    MxmComChannelConnectInfo connectInfo;
    connectInfo.SetCurNodeId(engineInfo.GetNodeId());
    connectInfo.SetRemoteNodeId(payLoadPair.first);
    connectInfo.SetIsUds(isUds);

    if (payLoadPair.second == MxmChannelType::HEARTBEAT) {
        ch->SetChannelTimeOut(DEFAULT_HEARTBEAT_REQUEST_TIMEOUT, DEFAULT_HEARTBEAT_REQUEST_TIMEOUT);
    } else {
        ch->SetChannelTimeOut(GetIPCTimeOutFromEnv(), GetIPCTimeOutFromEnv());
    }
    MxmComChannelInfo chInfo(true, payLoadPair.second, engineName, ch, connectInfo);
    if (engineInfo.GetProtocol() == MxmProtocol::UDS && engineInfo.IsServerSide()) {
        UBSHcomNetUdsIdInfo uds;
        ch->GetRemoteUdsIdInfo(uds);
        chInfo.SetUDSInfo(uds);
    }
    rwLock.LockWrite();
    linkManager.InsertChannel(chInfo);
    DBG_LOGINFO("Insert channel " << channelId << " from [" << ipPort << "," << payload << "] success");
    rwLock.UnLock();
    if (payLoadPair.second == MxmChannelType::SINGLE_SIDE) {
        return HOK;
    }
    linkStateNotify(engineInfo, payLoadPair.first, channelId, MxmLinkState::LINK_UP);
    return HOK;
}

void MxmComEngine::BrokenChannel(const UBSHcomChannelPtr &ch)
{
    const auto &engineName = engineInfo.GetName();
    DBG_LOGINFO("Engine " << engineName << " channel broken");
    if (ch == nullptr) {
        DBG_LOGERROR("Engine " << engineName << " channel broken, and channel is nullptr");
        return;
    }
    auto channelId = ch->GetId();
    DBG_LOGINFO("Engine " << engineName << " channel broken, id: " << ch->GetId());
    std::pair<std::string, MxmChannelType> payLoadPair = SplitPayload(ch->GetPeerConnectPayload());
    rwLock.LockWrite();
    MxmComChannelInfo channelInfo;

    auto ret = linkManager.GetChannelByChannelId(channelId, channelInfo);
    if (ret != 0) {
        DBG_LOGWARN("Channel : " << ch->GetId() << " not found in MxmTransLinkManager.");
        rwLock.UnLock();
        return;
    }
    UBSHcomNetUdsIdInfo uds = channelInfo.GetUDSInfo();
    linkManager.RemoveChannelByChannelId(channelId, hcomNetService);
    rwLock.UnLock();
    linkStateNotify(engineInfo, channelInfo.GetConnectInfo().GetRemoteNodeId(), uds.pid, MxmLinkState::LINK_DOWN);
    auto isReconnectHook = engineInfo.GetReConnectHook();
    // 配置是否重连回调且回调返回false，则不进行重连；未配置重连回调，或者回调返回true则默认重连
    if (isReconnectHook != nullptr &&
        !isReconnectHook(channelInfo.GetConnectInfo().GetRemoteNodeId(), payLoadPair.second)) {
        return;
    }
    std::thread task([channelInfo, this]() { DoReconnect(channelInfo); });
    task.detach();
}

int MxmComEngine::ConvertContextToMessageCtx(const UBSHcomServiceContext &context, MxmComMessageCtx &msgCtx)
{
    MxmUdsIdInfo udsIdInfo{0, 0, 0};
    if (engineInfo.IsUds()) {
        GetUdsInfoFromNetServiceContext(context, udsIdInfo);
    }
    msgCtx.SetUdsInfo(udsIdInfo);
    auto msg = GetMessageFromNetServiceContext(context);

    auto msgPtr = static_cast<MxmComMessagePtr>(static_cast<void *>(msg));
    msgCtx.SetEngineName(engineInfo.GetName());
    msgCtx.SetChannelId(GetChannelIdFromNetServiceContext(context));
    auto ret = msgCtx.SetMessage(msgPtr, context.MessageDataLen());
    if (ret != 0) {
        DBG_LOGERROR("SetMessage failed. ret=" << ret);
        return -1;
    }
    msgCtx.SetRspCtx(context.RspCtx());
    return 0;
}

HRESULT MxmComEngine::ReceivedRequest(UBSHcomServiceContext &context)
{
    TP_TRACE_BEGIN(TP_UBSM_GET_HANDLER);
    auto engineName = engineInfo.GetName();
    DBG_LOGDEBUG("Engine " << engineName << " get new request");
    auto msg = GetMessageFromNetServiceContext(context);
    if (UNLIKELY(msg == nullptr)) {
        DBG_LOGERROR("Engine " << engineName << " get message is nullptr");
        return MXM_COM_ERROR_MESSAGE_INVALID;
    }
    if (UNLIKELY(!CheckMessageBodyLen(context, *msg))) {
        DBG_LOGERROR("Engine " << engineName << " check message size failed.");
        return MXM_COM_ERROR_MESSAGE_CHECK_SIZE_FAIL;
    }
    auto crc = msg->GetMessageHead().GetCrc();
    auto moduleCode = msg->GetMessageHead().GetModuleCode();
    auto opCode = msg->GetMessageHead().GetOpCode();
    auto crcNew = CrcUtil::SoftCrc32(msg->GetMessageBody(), msg->GetMessageBodyLen(), SHIFT_1);
    DBG_LOGDEBUG("Engine " << engineName << " get new request, op: " << opCode << ", module: " << moduleCode
                           << ", msg body len is :" << msg->GetMessageBodyLen() << ", crc is :" << crc
                           << ", crc new is:" << crcNew);

    if (crc != crcNew) {
        DBG_LOGERROR("Engine " << engineName << " check crc failed, op: " << opCode << ", module: " << moduleCode
                               << ", msg body len is :" << msg->GetMessageBodyLen() << ", crc is :" << crc
                               << ", crc new is:" << crcNew);
        return MXM_COM_ERROR_MESSAGE_CHECK_SIZE_FAIL;
    }
    MxmComMessageCtx msgCtx;
    auto ret = ConvertContextToMessageCtx(context, msgCtx);
    if (ret != 0) {
        DBG_LOGERROR("ConvertContextToMessageCtx failed. ret=" << ret);
        return MXM_COM_ERROR_MESSAGE_INVALID;
    }
    auto *hdl = GetMessageHandler(moduleCode, opCode);
    if (hdl == nullptr || hdl->handler == nullptr) {
        DBG_LOGERROR("Engine " << engineName << ", no handler for module " << moduleCode << ", op code " << opCode
                               << ", crc " << crc);
        msgCtx.FreeMessage();
        return MXM_COM_ERROR_MESSAGE_INVALID_OP_CODE;
    }
    TP_TRACE_END(TP_UBSM_GET_HANDLER, 0);
    if (handlerWork != nullptr) {
        handlerWork(hdl->handler, msgCtx);
    } else {
        msgCtx.FreeMessage();
        DBG_LOGWARN("Handler is empty, can not response, module code=" << moduleCode << "opcode=" << opCode);
    }
    return HOK;
}

HRESULT MxmComEngine::GetChannelById(uint64_t channelId, MxmComChannelInfo &channelInfo)
{
    rwLock.LockRead();
    auto ret = linkManager.GetChannelByChannelId(channelId, channelInfo);
    rwLock.UnLock();
    return ret;
}

HRESULT MxmComEngine::SendRequest(const UBSHcomServiceContext &context)
{
    int32_t ret = context.Result();
    DBG_LOGINFO("Send request finish, result is " << ret);
    return HOK;
}

HRESULT MxmComEngine::OneSideDoneRequest(const UBSHcomServiceContext &context)
{
    int32_t ret = context.Result();
    DBG_LOGINFO("One side done finish, result is " << ret);
    return HOK;
}

void MxmComEngine::DoReconnect(const MxmComChannelInfo &channelInfo)
{
    DBG_LOGINFO("Start reconnect task, engine " << channelInfo.GetEngineName() << ", remote is "
                                                << channelInfo.GetConnectInfo().GetRemoteNodeId());
    auto ret = pthread_setname_np(pthread_self(), "Reconnect");
    if (ret != 0) {
        DBG_LOGERROR("Set thread name fail," << ret);
    }
    int waitSeconds = 1;
    while (!deleted.load()) {
        auto result = CreateChannel(const_cast<MxmComChannelConnectInfo &>(channelInfo.GetConnectInfo()),
                                    channelInfo.GetChannelType());
        if (result == 0) {
            break;
        }
        DBG_LOGERROR("Reconnect fail, engine " << channelInfo.GetEngineName() << ", remote is "
                                               << channelInfo.GetConnectInfo().GetRemoteNodeId());
        std::this_thread::sleep_for(std::chrono::seconds(waitSeconds));
    }
    if (this->postReconnectHandler) {
        DBG_LOGINFO("Reconnect successfully. Start to check invoke postReconnectHandler.");
        ret = postReconnectHandler();
        if (ret != 0) {
            DBG_LOGERROR("Failed to invoke postReconnectHandler, ret=" << ret);
            return;
        }
        DBG_LOGINFO("Success to invoke postReconnectHandler.");
    }
    DBG_LOGINFO("Reconnect task finish, engine " << channelInfo.GetEngineName() << ", remote is "
                                                 << channelInfo.GetConnectInfo().GetRemoteNodeId());
}

HRESULT MxmComEngineManager::CreateEngine(const MxmComEngineInfo &engineInfo, const MxmComLinkStateNotify &notify,
                                          const EngineHandlerWorker &handlerWorker)
{
    std::lock_guard<std::mutex> locker(G_MUTEX);
    const auto &engineName = engineInfo.GetName();
    auto iter = G_ENGINE_MAP.find(engineName);
    if (iter != G_ENGINE_MAP.end()) {
        DBG_LOGINFO("Engine " << engineName << " has already been created");
        return HOK;
    }
    NetLogger::Instance()->SetExternalLogFunction(engineInfo.GetLogFunc());
    MxmProtocol hcomProtocol = engineInfo.GetProtocol();
    UBSHcomServiceOptions options{};
    AssignServiceOptions(engineInfo, options);
    auto service = UBSHcomService::Create(static_cast<UBSHcomServiceProtocol>(hcomProtocol), engineName, options);
    if (UNLIKELY(service == nullptr)) {
        DBG_LOGERROR("Failed to create engine, name=" << engineName << "error=no memory");
        return MXM_COM_ERROR_ENGINE_CREATE_FAIL;
    }
    MxmComLinkManager linkManager;
    auto engine = new (std::nothrow) MxmComEngine(engineInfo, service, notify, linkManager);
    if (engine == nullptr) {
        DBG_LOGERROR("Failed to create engine, error=no memory");
        service->Destroy(engineName);
        return MXM_COM_ERROR_ENGINE_CREATE_FAIL;
    }

    int hr = engine->RegisterHandlerWork(handlerWorker);
    if (hr != HOK) {
        DBG_LOGWARN("Failed to rgister worker, ret=" << hr);
    }

    auto ret = engine->Start();
    if (ret != 0) {
        DBG_LOGERROR("Failed to start engine, ret=" << ret);
        delete engine;
        service->Destroy(engineName);
        return ret;
    }
    G_ENGINE_MAP.emplace(engineName, engine);
    DBG_LOGINFO("Create engine=" << engineName << " successfully");
    return HOK;
}

void MxmComEngineManager::DeleteEngine(const std::string &name)
{
    MxmComEngine *enginePtr;
    {
        std::lock_guard<std::mutex> locker(G_MUTEX);
        auto iter = G_ENGINE_MAP.find(name);
        if (iter == G_ENGINE_MAP.end()) {
            DBG_LOGWARN("Engine " << name << "has not been created or already been destroyed");
            return;
        }
        enginePtr = iter->second;
        G_ENGINE_MAP.erase(name);
    }
    if (enginePtr != nullptr) {
        enginePtr->Stop();
        delete enginePtr;
        enginePtr = nullptr;
    }
}

void MxmComEngineManager::DeleteAllEngine()
{
    std::lock_guard<std::mutex> locker(G_MUTEX);
    std::vector<std::string> keys;
    for (const auto &kv : G_ENGINE_MAP) {
        keys.push_back(kv.first);
    }
    for (const auto &key : keys) {
        auto iter = G_ENGINE_MAP.find(key);
        iter->second->Stop();
        delete iter->second;
        G_ENGINE_MAP.erase(key);
    }
}

MxmComEngine *MxmComEngineManager::GetEngine(const std::string &name)
{
    std::lock_guard<std::mutex> locker(G_MUTEX);
    auto iter = G_ENGINE_MAP.find(name);
    if (iter == G_ENGINE_MAP.end()) {
        DBG_LOGERROR("Engine " << name << "has not been created");
        return nullptr;
    }
    return iter->second;
}

HRESULT CreateChannel(bool isUds, const std::string &engineName, const std::pair<std::string, uint16_t> &ipAndPort,
                      const std::pair<std::string, std::string> &nodeIds, MxmChannelType chType)
{
    if (ipAndPort.first.empty()) {
        DBG_LOGERROR("connect ip or udsPath is empty");
        return HFAIL;
    }
    if (nodeIds.first.empty() || nodeIds.second.empty()) {
        DBG_LOGERROR("Node id is empty, current node id=" << nodeIds.first << ", remote node id=" << nodeIds.second);
        return HFAIL;
    }
    MxmComEngine *engine = MxmComEngineManager::GetEngine(engineName);
    if (engine == nullptr) {
        DBG_LOGERROR("Failed to get engine, engine name=" << engineName);
        return MXM_COM_ERROR_GET_ENGINE_FAIL;
    }
    DBG_LOGDEBUG("Creating Channel, current node id=" << nodeIds.first << ", remote node id=" << nodeIds.second);
    MxmComChannelConnectInfo connectInfo(isUds, ipAndPort.first, ipAndPort.second, nodeIds.second, nodeIds.first);
    connectInfo.SetLinkNum(SHIFT_1);
    return engine->CreateChannel(connectInfo, chType);
}

HRESULT CreateCallBack(const MxmComCallback &usrCb, Callback *&done)
{
    done = UBSHcomNewCallback(
        [usrCb](UBSHcomServiceContext &context) {
            if (context.Result() != 0) {
                DBG_LOGERROR("callback return failed," << context.Result());
            }
            usrCb.cb(usrCb.cbCtx, context.MessageData(), context.MessageDataLen(), context.Result());
            return;
        },
        std::placeholders::_1);
    if (done == nullptr) {
        DBG_LOGERROR("New net callback failed.");
        usrCb.cb(usrCb.cbCtx, nullptr, 0, MXM_COM_ERROR_NEW_NET_CALLBACK_FAIL);
        return MXM_COM_ERROR_NEW_NET_CALLBACK_FAIL;
    }
    return HOK;
}

HRESULT GetChannel(const std::string &engineName, MxmComMessageCtx &message, UBSHcomChannelPtr &channel,
                   uint32_t &sendLen)
{
    auto *transMsg = static_cast<MxmComMessage *>(static_cast<void *>(message.GetMessage()));
    if (transMsg == nullptr) {
        DBG_LOGERROR("The trans msg is nullptr.");
        return MXM_COM_ERROR_MESSAGE_INVALID;
    }
    const std::string &nodeId = message.GetDstId();
    MxmComChannelInfo channelInfo;
    auto engine = MxmComEngineManager::GetEngine(engineName);
    if (engine == nullptr) {
        DBG_LOGERROR("get engine failed, engineName: " << engineName);
        return MXM_COM_ERROR_GET_ENGINE_FAIL;
    }
    HRESULT res = engine->GetChannelByRemoteNodeId(nodeId, message.GetChannelType(), channelInfo);
    if (res != 0) {
        DBG_LOGERROR("Channel info is abnormal, nodeId: " << nodeId);
        return MXM_COM_ERROR_CHANNEL_NOT_FOUND;
    }
    channel = channelInfo.GetChannel();
    if (channel == nullptr) {
        DBG_LOGERROR("Channel is nullptr, nodeId: " << nodeId);
        return MXM_COM_ERROR_CHANNEL_NULL;
    }
    sendLen = static_cast<uint32_t>(sizeof(MxmComMessage)) + transMsg->GetMessageBodyLen();
    return HOK;
}

HRESULT MxmCommunication::CreateMxmComEngine(const MxmComEngineInfo &engine, const MxmComLinkStateNotify &notify,
                                             const EngineHandlerWorker &handlerWorker)
{
    return MxmComEngineManager::CreateEngine(engine, notify, handlerWorker);
}

void MxmCommunication::DeleteMxmComEngine(const std::string &name)
{
    return MxmComEngineManager::DeleteEngine(name);
}

HRESULT MxmCommunication::MxmComRpcConnect(const std::string &engineName, const RpcNode &remoteNodeId,
                                           const std::string &nodeId, MxmChannelType chType)
{
    DBG_LOGDEBUG("rpc connect start, curNodeId is " << nodeId << " remoteNodeId is " << remoteNodeId.ip);
    std::pair<std::string, uint16_t> ipAndPort = std::make_pair(remoteNodeId.ip, remoteNodeId.port);
    std::pair<std::string, std::string> nodeIds = std::make_pair(nodeId, remoteNodeId.name);
    HRESULT res = CreateChannel(false, engineName, ipAndPort, nodeIds, chType);
    if (res != 0) {
        DBG_LOGERROR("create rpc_connect channel failed," << res);
        return HFAIL;
    }
    DBG_LOGDEBUG("rpc_connect channel success, curNodeId: " << nodeId << " remoteNodeId:" << remoteNodeId.ip);
    return HOK;
}

HRESULT MxmCommunication::MxmComIpcConnect(const std::string &engineName, const std::string &udsPath,
                                           const std::string &nodeId, MxmChannelType chType)
{
    DBG_LOGINFO("ipc connect start, nodeId=" << nodeId << ", udsName=" << udsPath);
    std::string udsAbsolutePath = GetUdsPath(udsPath);
    const std::pair<std::string, uint16_t> &ipAndPort = std::make_pair(udsAbsolutePath, 0);
    const std::pair<std::string, std::string> &nodeIds = std::make_pair(nodeId, nodeId);
    HRESULT res = CreateChannel(true, engineName, ipAndPort, nodeIds, chType);
    if (res != 0) {
        DBG_LOGERROR("Failed to create channel, ret=" << res << ", engineName=" << engineName << ", nodeId=" << nodeId);
        return HFAIL;
    }
    DBG_LOGINFO("create ipc_connect channel successfully, engineName=" << engineName << ", nodeId=" << nodeId);
    return HOK;
}

HRESULT MxmCommunication::RegMxmComMsgHandler(const std::string &engineName, const MxmComMsgHandler &handle)
{
    MxmComEngine *engine = MxmComEngineManager::GetEngine(engineName);
    if (engine == nullptr) {
        DBG_LOGERROR("get engine failed, engineName: " << engineName);
        return MXM_COM_ERROR_GET_ENGINE_FAIL;
    }
    return engine->RegMxmComMsgHandler(handle);
}

HRESULT MxmCommunication::MxmComMsgSend(const std::string &engineName, MxmComMessageCtx &message,
                                        MxmComDataDesc &retData)
{
    UBSHcomChannelPtr channel;
    uint32_t sendLen;
    HRESULT channelRes = GetChannel(engineName, message, channel, sendLen);
    if (channelRes != HOK) {
        return channelRes;
    }
    MxmComDataDesc sendData = {message.GetMessage(), sendLen};
    UBSHcomRequest reqMsg{(sendData.data), sendData.len, 0};
    UBSHcomResponse rspMsg;

    TP_TRACE_BEGIN(TP_UBSM_HCOM_CALL);
    auto ret = channel->Call(reqMsg, rspMsg);
    TP_TRACE_END(TP_UBSM_HCOM_CALL, ret);
    if (ret != 0) {
        DBG_LOGERROR("Channel syncCallRaw failed, ret," << ret);
        return MXM_COM_ERROR_SYNC_CALL_FAIL;
    }
    retData.data = static_cast<uint8_t *>(rspMsg.address);
    retData.len = rspMsg.size;
    return HOK;
}

void MxmCommunication::MxmComMsgReply(MxmComMessageCtx &message, const MxmComDataDesc &data,
                                      const MxmComCallback &usrCb)
{
    uintptr_t rspCtx = message.GetRspCtx();
    const std::string &nodeId = message.GetDstId();
    const auto &engineName = message.GetEngineName();
    auto engine = MxmComEngineManager::GetEngine(engineName);
    if (engine == nullptr) {
        DBG_LOGERROR("Reply fail, get engine failed, engineName: " << engineName << ", channel id "
                                                                   << message.GetChannelId());
        return;
    }
    MxmComChannelInfo channelInfo;
    HRESULT res = engine->GetChannelById(message.GetChannelId(), channelInfo);
    if (res != 0) {
        DBG_LOGERROR("Channel info is abnormal, channel id: " << message.GetChannelId());
        return;
    }
    const UBSHcomChannelPtr &channel = channelInfo.GetChannel();
    if (channel == nullptr) {
        DBG_LOGERROR("Channel is nullptr, nodeId: " << nodeId;
                     usrCb.cb(usrCb.cbCtx, nullptr, 0, MXM_COM_ERROR_CHANNEL_NULL));
        return;
    }
    UBSHcomRequest reqMsg{(data.data), data.len, 0};
    UBSHcomReplyContext replyCtx(rspCtx, 0);

    Callback *done;
    if (CreateCallBack(usrCb, done) != HOK) {
        return;
    }

    auto replyHook = message.GetReplyFuncHook();
    if (replyHook != nullptr) {
        res = replyHook(reqMsg, done);
    } else {
        res = channel->Reply(replyCtx, reqMsg, done);
    }
    if (res != 0) {
        DBG_LOGERROR("Channel sendRaw failed, res: " << res << " nodeId: " << nodeId);
        usrCb.cb(usrCb.cbCtx, nullptr, 0, MXM_COM_ERROR_REPLY_FAIL);
    }
}

void MxmCommunication::RemoveChannel(const std::string &engineName, const std::string &remoteNodeId,
                                     MxmChannelType type)
{
    auto engine = MxmComEngineManager::GetEngine(engineName);
    if (engine == nullptr) {
        DBG_LOGWARN("Get engine failed, engineName: " << engineName);
        return;
    }
    engine->RemoveChannel(remoteNodeId, type);
}

int MxmCommunication::SetPostReconnectHandler(const std::string &name, MxmComPostReconnectHandler handler)
{
    auto engine = MxmComEngineManager::GetEngine(name);
    if (engine == nullptr) {
        DBG_LOGERROR("Get engine failed, name: " << name);
        return HFAIL;
    }
    return engine->SetMxmComPostReconnectHandler(std::move(handler));
}
} // namespace ock::com
