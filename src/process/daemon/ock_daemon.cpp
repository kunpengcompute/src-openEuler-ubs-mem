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
#include "ock_daemon.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <syslog.h>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include "dlock_config.h"
#include "dlock_context.h"
#include "logger/ubsmem_logger_manager.h"
#include "mxm_rpc_server_interface.h"
#include "record_store.h"
#include "rpc_server.h"
#include "ubs_common_config.h"
#include "ubs_mem_monitor.h"
#include "ubse_mem_adapter.h"
#include "ubsm_lock.h"
#include "ubsm_lock_event.h"
#include "ubsm_ptracer.h"
#include "ubsm_thread_pool.h"
#include "zen_discovery.h"

static constexpr int KEEP_ALIVE_DIVIDE = 2;

namespace ock {
namespace daemon {
using ZenEventType = zendiscovery::ElectionModule::ZenElectionEventType;
using ZenDiscovery = zendiscovery::ZenDiscovery;
using namespace ock::com::rpc;
using namespace ock::rpc::service;
using namespace ock::dlock_utils;
OCKDaemonPtr OckDaemon::mDaemon = nullptr;

const auto DAEMON_CONF_FILE = std::string("ubsmd.conf");
const auto BIN_PATH_HEADER = std::string("-binpath=");
constexpr int MB_MULTIPLER = 1024 * 1024;
OckDaemon::OckDaemon() : mHtracerEnable(false)
{
    mDaemon = this;
    mDaemon->mStatus = ServerStatus::UNINITIALIZED;
}

OckDaemon::~OckDaemon()
{
    StoppedKeepAlive();
    ubsmem::log::UbsmemLoggerManager::Destroy();
    mDaemon->SetRefCount(0);
    mDaemon = nullptr;
}

HRESULT OckDaemon::CheckParam(const std::string &binPath)
{
    if (CheckBinPath(binPath.c_str()) != 0) {
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::CheckBinPath(const char *binPath)
{
    if (binPath == nullptr) {
        std::cerr << "Failed to get binpath." << std::endl;
        return HFAIL;
    }
    if (strncmp(binPath, BIN_PATH_HEADER.c_str(), BIN_PATH_HEADER.size()) != 0) {
        std::cerr << "Parameter binpath input format error." << std::endl;
        return HFAIL;
    }
    binPath += BIN_PATH_HEADER.size();
    if (strnlen(binPath, PATH_MAX) == PATH_MAX) {
        std::cerr << "Binpath size exceeds maximum limit." << std::endl;
        return HFAIL;
    }
    char realBinPath[PATH_MAX + 1] = {0x00};
    if (realpath(binPath, realBinPath) == nullptr) {
        std::cerr << "The binpath input is not accessible." << std::endl;
        return HFAIL;
    }
    if (!OckFileDirExists(realBinPath)) {
        std::cerr << "The binpath input cannot be accessed." << std::endl;
        return HFAIL;
    }
    mHomePath = std::string(realBinPath);
    return HOK;
}

HRESULT OckDaemon::GetConfPath(std::string &confPath)
{
    confPath = mHomePath;
    confPath += "/config/" + DAEMON_CONF_FILE;
    if (!OckFileDirExists(confPath)) {
        std::cerr << "Configuration file <" << confPath << "> doesn't exist" << std::endl;
        return HFAIL;
    }

    return HOK;
}

HRESULT OckDaemon::LoadDaemonConf()
{
    std::string confPath;
    if (GetConfPath(confPath) != 0) {
        std::cerr << "GetConfPath fail." << std::endl;
        return HFAIL;
    }
    mConf = ock::common::Configuration::GetInstance(confPath);
    if (mConf == nullptr) {
        std::cerr << "Configuration initialize failed." << std::endl;
        return HFAIL;
    }
    if (ValidateConfiguration(confPath) != 0) {
        std::cerr << "Configuration invalid, please check <" << DAEMON_CONF_FILE << ">" << std::endl;
        return HFAIL;
    }
    std::string binPathArg = BIN_PATH_HEADER + mHomePath;
    mConf->Set(ock::common::ConfConstant::MXMD_DAEMON_BINPATH.first, binPathArg);
    return HOK;
}

HRESULT OckDaemon::InitDaemonLog()
{
    if (mConf == nullptr) {
        std::cerr << "Fail to load configuration." << std::endl;
        return HFAIL;
    }
    std::string level = mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_LOG_LEVEL.first);
    int32_t logLevel = static_cast<int32_t>(ock::common::LogAdapter::StringToLogLevel(level));
    std::string logOutputPath = mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_LOG_PATH.first);
    int32_t rotationFileSize = mConf->GetInt(ock::common::ConfConstant::MXMD_SERVER_LOG_ROTATION_FILE_SIZE.first);
    int32_t rotationFileCount = mConf->GetInt(ock::common::ConfConstant::MXMD_SERVER_LOG_ROTATION_FILE_COUNT.first);
    std::string auditEnable = mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_AUDIT_ENABLE.first);
    std::string auditLogOutputPath = mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_AUDIT_LOG_PATH.first);
    int32_t auditRotationFileSize =
        mConf->GetInt(ock::common::ConfConstant::MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_SIZE.first);
    int32_t auditRotationFileCount =
        mConf->GetInt(ock::common::ConfConstant::MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_COUNT.first);
    int64_t rotationFileSpace = static_cast<int64_t>(rotationFileSize) * static_cast<int64_t>(MB_MULTIPLER);
    if (rotationFileSpace > INT32_MAX) {
        return HFAIL;
    }
    HRESULT hr = ock::common::LogAdapter::LogServerInit(logLevel, logOutputPath,
                                                        static_cast<int32_t>(rotationFileSpace), rotationFileCount);
    if (hr != 0) {
        std::cerr << "Log initialize fail." << std::endl;
        return HFAIL;
    }
    if (auditEnable == "on") {
        int64_t auditRotationFileSpace =
            static_cast<int64_t>(auditRotationFileSize) * static_cast<int64_t>(MB_MULTIPLER);
        if (auditRotationFileSpace > INT32_MAX) {
            return HFAIL;
        }
        hr = ock::common::LogAdapter::AuditLogInit(auditLogOutputPath, static_cast<int32_t>(auditRotationFileSpace),
                                                   auditRotationFileCount);
        if (hr != 0) {
            std::cerr << "Audit log initialize fail." << std::endl;
            return HFAIL;
        }
    }
    return HOK;
}

HRESULT OckDaemon::InitHtrace()
{
    if (mConf == nullptr) {
        DBG_LOGERROR("Fail to load configuration.");
        return HFAIL;
    }
    std::string htraceEnable =
        mConf->GetString(ock::common::ConfConstant::MXMD_SEVER_PERFORMANCE_STATISTICS_ENABLE.first);
    if (htraceEnable == "on") {
        mHtracerEnable = true;
        int hr = ubsm::tracer::UbsmPtracer::Init();
        if (hr != HOK) {
            DBG_LOGWARN("Performance statistics tool failed to start, ret=" << hr);
        }
    } else {
        DBG_LOGINFO("Performance statistics is not enabled, conf item=" << htraceEnable);
    }
    return HOK;
}

HRESULT OckDaemon::InitDaemonService()
{
    if (serviceManager != nullptr) {
        DBG_LOGWARN("Cannot init services again.");
        return HOK;
    }
    serviceManager = new (std::nothrow) OckServiceManager();
    if (serviceManager == nullptr) {
        DBG_LOGERROR("Create service manager Failed.");
        return HFAIL;
    }
    if (mConf == nullptr) {
        DBG_LOGERROR("Fail to load configuration.");
        return HFAIL;
    }
    std::string serviceConf = std::string(ock::common::ConfConstant::MXMD_FEATURES_ENABLE.second);
    std::vector<std::string> splitStrings;
    SplitStr(serviceConf, "|", splitStrings);
    return serviceManager->ServicePut(splitStrings, mHomePath);
}

HRESULT OckDaemon::InitHandler()
{
    if (LoadDaemonConf() != 0) {
        std::cerr << "LoadDaemonConf failed." << std::endl;
        return HFAIL;
    }
    if (InitDaemonLog() != 0) {
        std::cerr << "InitDaemonLog failed." << std::endl;
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::ValidateConfiguration(const std::string &confPath)
{
    if (mConf == nullptr) {
        std::cerr << "Fail to load configuration" << std::endl;
        return HFAIL;
    }
    std::vector<std::string> validationError = mConf->ValidateDaemonConf();
    if (!(validationError.empty())) {
        std::cerr << "Wrong configuration in file <" << confPath << ">, because of following mistakes:" << std::endl;
        for (auto &item : validationError) {
            std::cout << item << std::endl;
        }
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::CommonInitialize()
{
    if (InitHandler() != 0 || mConf == nullptr) {
        std::cerr << "Failed to initialize logger or load configuration." << std::endl;
        return HFAIL;
    }
    if (RegisterSignalHandler() != 0) {
        DBG_LOGERROR("Failed to register signal handler.");
        std::cerr << "Failed to RegisterSignalHandler()." << std::endl;
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::ServicesInitialize()
{
    if (InitDaemonService() != 0) {
        DBG_LOGERROR("Failed to initialize services.");
        std::cerr << "Failed to InitDaemonService()." << std::endl;
        return HFAIL;
    }
    if (CheckServicesCount() != 0) {
        DBG_LOGERROR("No service was loaded successfully, please check <"
                     << ock::common::ConfConstant::MXMD_FEATURES_ENABLE.first << "> and library files");
        std::cerr << "No service was loaded successfully, please check <"
                  << ock::common::ConfConstant::MXMD_FEATURES_ENABLE.first << "> and library files" << std::endl;
        return static_cast<HRESULT>(MXM_ERR_DAEMON_NO_FEATURE_ENABLED);
    }
    if (serviceManager->ServiceProcessArgs() != 0) {
        DBG_LOGERROR("OckDaemon initialize failed. ");
        mConf->DumpConfig();
        return HFAIL;
    }
    if (serviceManager->ServiceInitialize() != 0) {
        DBG_LOGERROR("OckDaemon initialize failed. ");
        mConf->DumpConfig();
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::Initialize()
{
    if (mHomePath.empty()) {
        std::cerr << "Cannot initialize OckDaemon without checking parameters." << std::endl;
        return HFAIL;
    }
    if (mStatus != ServerStatus::UNINITIALIZED) {
        std::cerr << "Unable to initialize OckDaemon again." << std::endl;
        return HFAIL;
    }

    HRESULT InitHr = HOK;
    InitHr = CommonInitialize();
    if (InitHr != 0) {
        return InitHr;
    }
    InitHr = ServicesInitialize();
    if (InitHr != 0) {
        return InitHr;
    }
    InitHr = InitHtrace();
    if (InitHr != 0) {
        DBG_LOGERROR("Failed to initialize htrace.");
        return InitHr;
    }
    DBG_LOGINFO("UBS Memory Daemon(ubsmd) is initialized.");
    mConf->DumpConfig();

    mStatus = ServerStatus::INITIALIZED;
    return InitHr;
}

HRESULT OckDaemon::CheckServicesCount()
{
    if (serviceManager == nullptr) {
        return HFAIL;
    }
    if (serviceManager->ServiceNum() == 0) {
        return HFAIL;
    }
    return HOK;
}

HRESULT OckDaemon::Start(const std::chrono::time_point<std::chrono::steady_clock> &start)
{
    PrintStartTime(start, "START");
    common::systemd::NotifyReady();
    if (StartServices() != 0) {
        return HFAIL;
    }
    if (Wait() != 0) {
        return HFAIL;
    }
    StoppingKeepAlive();
    return HOK;
}

int32_t OckDaemon::InitializeRpcServer()
{
    int32_t ret = MxmComStartRpcServer();
    if (ret != 0) {
        DBG_LOGERROR("MxmComStartRpcServer failed, ret: " << ret);
        return ret;
    }
    ret = RpcServer::GetInstance().RackMemConBaseInitialize();
    if (ret != 0) {
        DBG_LOGERROR("RackMemConBaseInitialize failed, ret: " << ret);
        return ret;
    }
    DBG_LOGINFO("InitializeRpcServer success.");
    return HOK;
}

void OckDaemon::ConfigureDLock()
{
    auto &cfg = dlock_utils::DLockContext::Instance().GetConfig();
    cfg.dlockDevName = mConf->GetString(ock::common::ConfConstant::MXMD_LOCK_DEV_NAME.first);
    cfg.dlockDevEid = mConf->GetString(ock::common::ConfConstant::MXMD_LOCK_DEV_EID.first);
    cfg.lockExpireTime = mConf->GetInt(ock::common::ConfConstant::MXMD_LOCK_EXPIRE_TIME.first);
    cfg.enableTls = (mConf->GetString(ock::common::ConfConstant::MXMD_IS_LOCK_TLS_ENABLE.first) == "on");
    cfg.ubToken = (mConf->GetString(ock::common::ConfConstant::MXMD_IS_LOCK_UB_TOKEN_ENABLE.first) == "on");
    std::string level = mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_LOG_LEVEL.first);
    cfg.dlockLogLevel = dlock_utils::MapUbsmLogLevel2DLockLevel(ock::common::LogAdapter::StringToLogLevel(level));
    DBG_LOGINFO("UbsmLock config loaded, devName=" << cfg.dlockDevName << ", devEid=" << cfg.dlockDevEid
                                                   << ", lock expireTime=" << cfg.lockExpireTime
                                                   << ", lock tls=" << cfg.enableTls << ", ubToken=" << cfg.ubToken
                                                   << ", dlockLogLevel=" << cfg.dlockLogLevel);
}

bool OckDaemon::InitializeZenDiscovery()
{
    int32_t joinTimeout = mConf->GetInt(ock::common::ConfConstant::MXMD_DISCOVERY_ELECTION_TIMEOUT.first);
    int32_t minNodes = mConf->GetInt(ock::common::ConfConstant::MXMD_DISCOVERY_MIN_NODES.first);
    ZenDiscovery::Initialize(3000L, 5000L, joinTimeout, minNodes);
    ZenDiscovery *zenDiscovery = ZenDiscovery::GetInstance();
    if (zenDiscovery == nullptr) {
        std::cerr << "Configuration initialize failed." << std::endl;
        return false;
    }
    zenDiscovery->AddElectionListener(
        [this](ZenEventType event, const std::string &masterId, const std::string &voteOnlyId) {
            dlock_utils::UbsmLockEvent::HandleElectionEvent(event, masterId, voteOnlyId);
        });
    zenDiscovery->Start();
    return true;
}

uint32_t OckDaemon::InitUbsmLock()
{
    if (mConf == nullptr) {
        std::cerr << "Configuration initialize failed." << std::endl;
        return HFAIL;
    }
    auto lockEnable = mConf->GetString(ConfConstant::MXMD_IS_LOCK_ENABLE.first);
    if (lockEnable != "on") {
        DBG_LOGINFO("Dlock do not need to init.");
        return HOK;
    }
    // dlock config
    ConfigureDLock();
    // init tls config
    if (InitRpcTlsConfig() != 0 || InitLockTlsConfig() != 0) {
        DBG_LOGERROR("Init tls config failed.");
        return HOK;
    }
    // init rpc
    if (InitializeRpcServer() != 0) {
        DBG_LOGERROR("InitializeRpcServer failed.");
        return HOK;
    }
    // init zenDiscovery
    if (!InitializeZenDiscovery()) {
        DBG_LOGERROR("InitializeZenDiscovery failed.");
    }
    return HOK;
}

int32_t OckDaemon::InitLockTlsConfig()
{
    if (mConf == nullptr) {
        std::cerr << "Configuration initialize failed." << std::endl;
        return HFAIL;
    }

    auto lockTls = mConf->GetString(ock::common::ConfConstant::MXMD_IS_LOCK_TLS_ENABLE.first) == "on";
    if (!lockTls) {
        DBG_LOGINFO("dlock tls is not enabled.");
        return HOK;
    }

    auto caPath = mConf->GetString(ock::common::ConfConstant::UBSMD_LOCK_TLS_CA_PATH.first);
    auto crlPath = mConf->GetString(ock::common::ConfConstant::UBSMD_LOCK_TLS_CRL_PATH.first);
    auto certPath = mConf->GetString(ock::common::ConfConstant::UBSMD_LOCK_TLS_CERT_PATH.first);
    auto keyPath = mConf->GetString(ock::common::ConfConstant::UBSMD_LOCK_TLS_KEY_PATH.first);

    auto keypassPath = mConf->GetString(ock::common::ConfConstant::UBSMD_LOCK_TLS_KEYPASS_PATH.first);

    std::vector<std::pair<std::string, bool>> paths{};
    paths.push_back(std::make_pair(caPath, true));
    paths.push_back(std::make_pair(crlPath, false));
    paths.push_back(std::make_pair(certPath, true));
    paths.push_back(std::make_pair(keyPath, true));
    paths.push_back(std::make_pair(keypassPath, true));

    auto errors = mConf->ValidateFilePath(paths);
    if (!errors.empty()) {
        for (auto &err : errors) {
            DBG_LOGERROR("Check dlock tls path failed: " << err);
        }
        return HFAIL;
    }

    ock::ubsm::UbsCommonConfig::GetInstance().SetLockCaPath(caPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetLockCrlPath(crlPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetLockCertPath(certPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetLockKeyPath(keyPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetLockKeypassPath(keypassPath);

    DBG_LOGINFO("InitLockTlsConfig finished, ca path: " << ock::ubsm::UbsCommonConfig::GetInstance().GetLockCaPath());
    return HOK;
}

int32_t OckDaemon::InitRpcTlsConfig()
{
    if (mConf == nullptr) {
        std::cerr << "Configuration initialize failed." << std::endl;
        return HFAIL;
    }
    auto tlsEnable = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_ENABLE.first) == "on";
    if (!tlsEnable) {
        DBG_LOGINFO("rpc tls is not enabled.");
        return HOK;
    }
    auto ciphersuits = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_CIPHERSUITS.first);
    auto caPath = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_CA_PATH.first);
    auto crlPath = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_CRL_PATH.first);
    auto certPath = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_CERT_PATH.first);
    auto keyPath = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_KEY_PATH.first);
    auto keypassPath = mConf->GetString(ock::common::ConfConstant::UBSMD_SERVER_TLS_KEYPASS_PATH.first);
    std::vector<std::pair<std::string, bool>> paths{};
    paths.push_back(std::make_pair(caPath, true));
    paths.push_back(std::make_pair(crlPath, false));
    paths.push_back(std::make_pair(certPath, true));
    paths.push_back(std::make_pair(keyPath, true));
    paths.push_back(std::make_pair(keypassPath, true));
    auto errors = mConf->ValidateFilePath(paths);
    if (!errors.empty()) {
        for (auto &err : errors) {
            DBG_LOGERROR("Check tls path failed: " << err);
        }
        return HFAIL;
    }
    ock::ubsm::UbsCommonConfig::GetInstance().SetTlsSwitch(tlsEnable);
    ock::ubsm::UbsCommonConfig::GetInstance().SetCipherSuit(ciphersuits);
    ock::ubsm::UbsCommonConfig::GetInstance().SetCaPath(caPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetCrlPath(crlPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetCertPath(certPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetKeyPath(keyPath);
    ock::ubsm::UbsCommonConfig::GetInstance().SetKeypassPath(keypassPath);
    DBG_LOGDEBUG("InitTlsConfig finished, ca path: " << ock::ubsm::UbsCommonConfig::GetInstance().GetCaPath());
    return HOK;
}

HRESULT OckDaemon::CheckUbseStatus()
{
    int ret = HOK;
    size_t retryInterval = 1;
    SHMRegions regionList;
    do {
        ret = ock::mxm::UbseMemAdapter::LookupRegionList(regionList);
        if (ret != HOK) {
            if (mStatus == ServerStatus::UNINITIALIZED) {
                DBG_LOGERROR("OckDaemon status changed to UNINITIALIZED, stop check ubse status.");
                return HFAIL;
            }
            DBG_LOGWARN("UBS Engine is not ready, will retry, retcode: " << ret);
            common::systemd::NotifyWatchdog();
            sleep(retryInterval);
        }
    } while (ret != HOK);

    uint32_t nodeId = {UINT32_MAX};
    ret = mxm::UbseMemAdapter::EnsureGetLocalNodeId(nodeId);
    if (ret != HOK) {
        DBG_LOGWARN("Failed to get local node id.");
    }
    DBG_LOGINFO("UBS Engine is ready.");
    return HOK;
}

HRESULT OckDaemon::StartServices()
{
    common::systemd::Notify("STATUS=unavailable");

    if (serviceManager == nullptr) {
        DBG_LOGERROR("Failed to initialize services.");
        return HFAIL;
    }
    if (mStatus != ServerStatus::INITIALIZED) {
        DBG_LOGERROR("Failed to start OckDaemon. OckDaemon hasn't been initialized or is running.");
        return HFAIL;
    }
    if (!GenerateShareMemory("/ubsm_records")) {
        DBG_LOGERROR("generate share memory for ubsm records store failed.");
        return HFAIL;
    }

    auto ret = ubsm::RecordStore::GetInstance().Initialize(mShmFd);
    if (ret != 0) {
        DBG_LOGERROR("initialize RecordStore failed.");
        close(mShmFd);
        mShmFd = -1;
        return HFAIL;
    }

    ret = UBSMThreadPool::GetInstance().Start();
    if (ret != 0) {
        DBG_LOGERROR("Failed to initialize pthread pool. ret=" << ret);
        return ret;
    }

    if (mConf->GetString(ock::common::ConfConstant::MXMD_SERVER_LEASE_CACHE_ENABLE.first) == "off") {
        ubsm::UbsCommonConfig::GetInstance().SetLeaseCacheSwitch(false);
    }

    HRESULT hr = CheckUbseStatus();
    if (hr != 0) {
        DBG_LOGERROR("Check ubse status failed.");
        close(mShmFd);
        mShmFd = -1;
        return HFAIL;
    }
    DBG_LOGINFO("ubsmd began to start services.");
    hr = serviceManager->ServiceStart();
    if (hr != 0) {
        ubsm::RecordStore::GetInstance().Destroy();
        close(mShmFd);
        mShmFd = -1;
        return hr;
    }

    std::string pidFilePath = mHomePath;
    pidFilePath += "/work/pids/ockd.pid";
    std::ofstream fout(pidFilePath.c_str());
    fout << getpid() << std::endl;
    fout << std::flush;
    fout.close();

    ret = ubsm::UBSMemMonitor::GetInstance().Initialize();
    if (ret != 0) {
        DBG_LOGERROR("initialize UBSMemMonitor failed. ret: " << ret);
        ubsm::RecordStore::GetInstance().Destroy();
        close(mShmFd);
        mShmFd = -1;
        return ret;
    }
    common::systemd::Notify("STATUS=available");
    hr = InitUbsmLock();
    if (hr != 0) {
        DBG_LOGERROR("initialize ZenDiscovery and DLock failed. ret: " << hr);
        ubsm::RecordStore::GetInstance().Destroy();
        close(mShmFd);
        mShmFd = -1;
        return hr;
    }

    DBG_LOGINFO("ubsmd is started.");
    return HOK;
}

HRESULT OckDaemon::Wait()
{
    if (mDaemon == nullptr) {
        DBG_LOGERROR("Daemon haven't started up, cannot start services.");
        return HFAIL;
    }
    mDaemon->mStatus = ServerStatus::WAITING; // 设置等待状态

    std::unique_lock<std::mutex> lock(mDaemon->mMutex);
    mDaemon->mCV.wait(lock, [this] {
        if (mDaemon->mStatus == ServerStatus::WAITING) {
            return false;
        }
        return true;
    });

    keepAliveStatus = KEEP_ALIVE_STOPPING;
    mCV.notify_all();

    common::systemd::Notify("STATUS=unavailable");
    DBG_LOGINFO("ubsmd stopped.");
    return HOK;
}

void OckDaemon::TryStop()
{
    if (mDaemon == nullptr) {
        DBG_LOGERROR("Daemon haven't started up, cannot start services.");
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mDaemon->mMutex);
        if (mDaemon->mStatus != ServerStatus::WAITING) {
            DBG_LOGWARN("A stop signal was received, while OckDaemon hasn't been started");
            mDaemon->mStatus = ServerStatus::UNINITIALIZED;
            return;
        }
        mDaemon->mStatus = ServerStatus::INITIALIZED;
    }
    mDaemon->mCV.notify_all();
    ZenDiscovery *zenDiscovery = ZenDiscovery::GetInstance();
    if (zenDiscovery != nullptr) {
        zenDiscovery->Stop();
    }
    MxmComStopRpcServer();
    (void)mDaemon->Shutdown();
}

HRESULT OckDaemon::Shutdown()
{
    if (serviceManager == nullptr) {
        DBG_LOGERROR("Failed to initialize services.");
        return HFAIL;
    }

    UBSMThreadPool::GetInstance().Stop();
    ubsm::UBSMemMonitor::GetInstance().Destroy();
    if (serviceManager->ServiceShutdown() != 0) {
        DBG_LOGERROR("Failed to ServiceShutdown.");
        return HFAIL;
    }
    DBG_LOGINFO("ubsmd is shutting down.");

    return HOK;
}

HRESULT OckDaemon::Uninitialize()
{
    if (mStatus != ServerStatus::INITIALIZED) {
        DBG_LOGERROR("failed to uninitialize OckDaemon. OckDaemon hasn't been initialized or is running");
        return HFAIL;
    }

    if (serviceManager == nullptr) {
        return HFAIL;
    }
    if (serviceManager->ServiceUninitialize() != 0) {
        DBG_LOGERROR("Uninitialize services failed.");
        return HFAIL;
    }
    DBG_LOGINFO("ubsmd is uninitializing.");

    mStatus = ServerStatus::UNINITIALIZED;
    return HOK;
}

void OckDaemon::StoppingKeepAlive()
{
    if (mDaemon == nullptr) {
        DBG_LOGERROR("Daemon haven't started up, cannot start services.");
        return;
    }
    keepAliveStatus.store(KEEP_ALIVE_STOPPING);
}

void OckDaemon::StoppedKeepAlive()
{
    if (mDaemon == nullptr) {
        DBG_LOGERROR("Daemon haven't started up, cannot start services.");
        return;
    }
    DBG_LOGINFO("Stopping...");
    keepAliveStatus.store(KEEP_ALIVE_STOPPED);
    if (mShmFd >= 0) {
        close(mShmFd);
        mShmFd = -1;
    }
}

HRESULT OckDaemon::RegisterSignalHandler()
{
    struct sigaction saUsr {
    };
    saUsr.sa_handler = &OckDaemon::HandleSignal;
    if (sigaction(SIGTERM, &saUsr, nullptr) < 0) {
        DBG_LOGERROR("Register signal SIGTERM failed. errno(" << errno << "). ");
        return HFAIL;
    }

    struct sigaction saUsr1 {
    };
    saUsr1.sa_handler = &OckDaemon::HandleSigpipe;
    if (sigaction(SIGPIPE, &saUsr1, nullptr) < 0) {
        DBG_LOGERROR("Register signal SIGTERM failed. errno(" << errno << "). ");
        return HFAIL;
    }
    return HOK;
}

void OckDaemon::HandleSignal(int signum)
{
    (void)signum;
    if (mDaemon == nullptr) {
        return;
    }
    mDaemon->TryStop();
}

void OckDaemon::HandleSigpipe(int signum)
{
    (void)signum;
    DBG_LOGWARN("get Signal SIGPIPE");
}

bool OckDaemon::GenerateShareMemory(const std::string &shmName) noexcept
{
    int fd = shm_open(shmName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        DBG_LOGERROR("shm_open with name: " << shmName << " failed: " << errno << ":" << strerror(errno));
        return false;
    }
    mShmFd = fd;
    return true;
}

void OckDaemon::PrintStartTime(const std::chrono::time_point<std::chrono::steady_clock> &start, const std::string &log)
{
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    double seconds = duration.count();
    syslog(LOG_INFO, "The process %s cost is %f s", log.c_str(), seconds);
}
} // namespace daemon
} // namespace ock
