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
#include "mem_lease_service.h"
#include "ipc_server.h"
#include "mls_manager.h"
#include "ubsm_ptracer.h"

using namespace ock::com::ipc;
using namespace ock::ubsm::tracer;
namespace ock::lease::service {
std::atomic_bool MemLeaseService::inited{false};
static int MemMxmComStopIpcServer()
{
    MxmComStopIpcServer();
    return HOK;
}

MemLeaseService::MemLeaseService() noexcept
{
    serviceName = "MemLease";

    modules = {
        {"MemLeaseIPCServer", &MxmComStartIpcServer, &IpcServer::GetInstance().RackMemConBaseInitialize,
         &MemMxmComStopIpcServer, nullptr},
        {"MemLeaseRecovery", &MLSManager::Init, &MLSManager::Recovery, &MLSManager::Finalize, nullptr},
    };
}

HRESULT MemLeaseService::OnServiceProcessArgs(int argc, char *argv[])
{
    return HOK;
}
} // namespace ock::lease::service