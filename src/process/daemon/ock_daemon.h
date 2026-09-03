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
#ifndef OCK_DAEMON_H
#define OCK_DAEMON_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include "log.h"
#include "ock_service_adapter.h"
#include "ock_service_manager.h"
#include "util/common_headers.h"
#include "util/log_adapter.h"
#include "util/systemd_wrapper.h"

namespace ock {
namespace daemon {
constexpr int ARGS_NUM = 2;
constexpr int BIN_PATH_POSITION = 1;
class OckDaemon : public ock::common::Referable {
public:
    OckDaemon();
    ~OckDaemon() override;

    HRESULT CheckParam(const std::string &binPath);
    HRESULT Initialize();
    HRESULT Start(const std::chrono::time_point<std::chrono::steady_clock> &start);
    void TryStop();
    HRESULT InitHandler();
    virtual HRESULT Shutdown();
    HRESULT Uninitialize();
    HRESULT ValidateConfiguration(const std::string &confPath);
    virtual HRESULT CheckServicesCount();
    static void PrintStartTime(const std::chrono::time_point<std::chrono::steady_clock> &start, const std::string &log);
    bool GetHtracerEnable()
    {
        return mHtracerEnable;
    }

private:
    HRESULT StartServices();
    HRESULT Wait();
    bool GenerateShareMemory(const std::string &shmName) noexcept;
    uint32_t InitUbsmLock();
    int32_t InitializeRpcServer();
    void ConfigureDLock();
    bool InitializeZenDiscovery();
    int32_t InitRpcTlsConfig();
    int32_t InitLockTlsConfig();

    enum class ServerStatus { UNINITIALIZED = 0, INITIALIZED = 1, WAITING = 2 };

    HpcServiceManagerPtr serviceManager = nullptr;
    ock::common::ConfigurationPtr mConf = nullptr;
    std::string mHomePath = "";

    enum KeepAliveStatus : int {
        KEEP_ALIVE_IDLE,
        KEEP_ALIVE_RUNNING,
        KEEP_ALIVE_STOPPING,
        KEEP_ALIVE_STOPPED,
        KEEP_ALIVE_TOTAL
    };
    std::atomic<KeepAliveStatus> keepAliveStatus{KEEP_ALIVE_IDLE};

    static ock::common::Ref<OckDaemon> mDaemon;

    HRESULT GetConfPath(std::string &confPath);
    HRESULT LoadDaemonConf();
    HRESULT InitDaemonLog();
    HRESULT InitHtrace();
    HRESULT CheckBinPath(const char *binPath);

    void StoppingKeepAlive();
    void StoppedKeepAlive();

    HRESULT InitDaemonService();
    HRESULT RegisterSignalHandler();
    HRESULT CommonInitialize();
    HRESULT ServicesInitialize();
    static void HandleSignal(int signum);
    static void HandleSigpipe(int signum);
    HRESULT CheckUbseStatus();

    std::condition_variable mCV;
    std::mutex mMutex;
    std::atomic<ServerStatus> mStatus;
    int mShmFd{-1};
    bool mHtracerEnable;
};

using OCKDaemonPtr = ock::common::Ref<OckDaemon>;
} // namespace daemon
} // namespace ock

#endif