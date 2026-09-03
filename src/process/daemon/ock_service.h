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
#ifndef OCK_SERVICE_H
#define OCK_SERVICE_H

#include "ock_service_adapter.h"
#include "service/service.h"
#include "util/common_headers.h"

namespace ock {
namespace daemon {
using CreateT = void *(*)();

using ServiceProcessArgsT = HRESULT (*)(void *, int, char **);

using ServiceInitializeT = HRESULT (*)(void *service);

using ServiceStartT = HRESULT (*)(void *service);

using ServiceHealthyT = HRESULT (*)(void *service);

using ServiceShutdownT = HRESULT (*)(void *service);

using ServiceUninitializeT = HRESULT (*)(void *service);

using DestroyT = void (*)(void *service);

const auto SERVICE_METHOD_CREATE = "Create";
const auto SERVICE_METHOD_PROCESS_ARGS = "ServiceProcessArgs";
const auto SERVICE_METHOD_INITIALIZE = "ServiceInitialize";
const auto SERVICE_METHOD_START = "ServiceStart";
const auto SERVICE_METHOD_HEALTHY = "ServiceHealthy";
const auto SERVICE_METHOD_SHUTDOWN = "ServiceShutdown";
const auto SERVICE_METHOD_UN_INITIALIZE = "ServiceUninitialize";
const auto SERVICE_METHOD_DESTROY = "Destroy";

class OckService : public Service {
public:
    OckService() = default;

    ~OckService();

    HRESULT Initialize(OckServiceAdapter *serviceConf);

    const std::vector<std::string> &ServiceApplyArgs();

    HRESULT constructStatus = HFAIL;
    HRESULT processArgsStatus = HFAIL;
    HRESULT initializeStatus = HFAIL;
    HRESULT startStatus = HFAIL;
    HRESULT shutdownStatus = HFAIL;
    HRESULT uninitializeStatus = HFAIL;

protected:
    HRESULT OnServiceProcessArgs(int argc, char **argv) override;

    HRESULT OnServiceInitialize() override;

    HRESULT OnServiceStart() override;

    HRESULT OnServiceHealthy() override;

    HRESULT OnServiceShutdown() override;

    HRESULT OnServiceUninitialize() override;

    void PrintVersionInfo() override {}

private:
    void *serviceHandler;
    void *ockServiceAgentP;
    CreateT agentCreateP;
    ServiceProcessArgsT agentProcessArgsP;
    ServiceInitializeT agentInitializeP;
    ServiceStartT agentStartP;
    ServiceHealthyT agentHealthyP;
    ServiceShutdownT agentShutdownP;
    ServiceUninitializeT agentUninitializeP;
    DestroyT agentDestroyP;
    std::vector<std::string> mArgNames;

    void *GetAgentMethodP(const std::string &name);
    void *GetAgentDefaultedMethodP(const std::string &name);
};
} // namespace daemon
} // namespace ock

#endif
