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
#include "mem_share_service.h"
#include "ipc_server.h"
#include "region_manager.h"
#include "rpc_server.h"
#include "shm_manager.h"

namespace ock::share::service {
using namespace ock::rpc::service;
using namespace ock::com::ipc;
using namespace ock::com::rpc;
std::atomic_bool MemShareService::inited{false};
static int MemMxmComStopIpcServer()
{
    MxmComStopIpcServer();
    return HOK;
}

MemShareService::MemShareService() noexcept
{
    serviceName = "MemShare";
    modules = {
        {"MemShareIPCServer", &MxmComStartIpcServer, &IpcServer::GetInstance().RackMemConBaseInitialize,
         &MemMxmComStopIpcServer, nullptr},
        {"MemShareRecovery", &SHMManager::Initial, &SHMManager::Recovery, nullptr, nullptr},
        {"MemRegionRecovery", &RegionManager::Initial, &RegionManager::Recovery, nullptr, nullptr},
    };
}

HRESULT MemShareService::OnServiceProcessArgs(int argc, char *argv[])
{
    return HOK;
}
} // namespace ock::share::service