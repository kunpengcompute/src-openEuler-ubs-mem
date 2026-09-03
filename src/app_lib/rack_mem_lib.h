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

#ifndef MEMORYFABRIC_RACK_MEM_LIB_H
#define MEMORYFABRIC_RACK_MEM_LIB_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>
#include "log.h"
#include "rack_mem_err.h"

extern "C" {
void __attribute__((constructor)) InitRackMemLib();
}

namespace ock::mxmd {

using LibInitFunc = std::function<uint32_t()>;
using LibShutdownFunc = std::function<uint32_t()>;

struct RackMemModule {
    std::string name;
    LibInitFunc init{nullptr};
    LibShutdownFunc shutdown{nullptr};
    bool isInitialized = false;

    RackMemModule() = default;
    RackMemModule(std::string name, LibInitFunc initFunc, LibShutdownFunc shutdownFunc, const bool initTag)
        : name(std::move(name)),
          init(std::move(initFunc)),
          shutdown(std::move(shutdownFunc)),
          isInitialized(initTag)
    {
    }
};

class RackMemLib {
public:
    uint32_t Initialize();

    void Destroy();

    [[nodiscard]] int StartRackMem() const
    {
        if (inited) {
            return 0;
        }
        const uint32_t hr = GetInstance().Initialize();
        if (BresultFail(hr)) {
            DBG_LOGERROR("Failed to initilize ubsm sdk");
            GetInstance().Destroy();
            return -1;
        }
        ubsmem::log::UbsmemLoggerManager::Instance()->SetLogLevel(ubsmem::log::UbsmemLogLevel::CRIT);
        return 0;
    }

    bool GetIsInitilized()
    {
        return inited;
    }

    static RackMemLib &GetInstance()
    {
        static RackMemLib instance;
        return instance;
    }
    RackMemLib(const RackMemLib &other) = delete;
    RackMemLib(RackMemLib &&other) = delete;
    RackMemLib &operator=(const RackMemLib &other) = delete;
    RackMemLib &operator=(RackMemLib &&other) noexcept = delete;

private:
    [[nodiscard]] uint32_t InitHtrace() const;

    [[nodiscard]] uint32_t ExitHtrace() const;

    static uint32_t InitShmMetaMgr();

    static uint32_t InitIpc();

    static uint32_t ShutdownIpc();
    void InitModules();
    std::vector<RackMemModule> pModules{};
    std::mutex lock{};
    std::atomic_bool inited{false};
    RackMemLib() = default;
};
} // namespace ock::mxmd

#endif // MEMORYFABRIC_RACK_MEM_LIB_H