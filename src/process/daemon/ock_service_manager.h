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

#ifndef OCK_OCK_SERVICE_MANAGER_H
#define OCK_OCK_SERVICE_MANAGER_H

#include <set>
#include <vector>
#include "ock_service.h"
#include "ock_service_adapter.h"
namespace ock {
namespace daemon {

class OckServiceManager : public Service {
public:
    OckServiceManager() = default;
    ~OckServiceManager();

    using Service::ServiceInitialize;
    using Service::ServiceProcessArgs;
    using Service::ServiceShutdown;
    using Service::ServiceStart;
    using Service::ServiceUninitialize;

    HRESULT ServicePut(const std::vector<std::string> &services, const std::string &libHomePath);

    HRESULT ServiceProcessArgs();

    HRESULT ServiceInitialize(const std::string &serviceName);

    HRESULT ServiceStart(const std::string &serviceName);

    HRESULT ServiceShutdown(const std::string &serviceName);

    HRESULT ServiceUninitialize(const std::string &serviceName);

    int ServiceNum()
    {
        return serviceMap.size();
    }

protected:
    HRESULT OnServiceProcessArgs(int argc, char **argv) override;

    HRESULT OnServiceInitialize() override;

    HRESULT OnServiceStart() override;

    HRESULT OnServiceHealthy() override;

    HRESULT OnServiceShutdown() override;

    HRESULT OnServiceUninitialize() override;

    void PrintVersionInfo() override {}

private:
    std::mutex mutex_;
    std::map<std::string, OckService *> serviceMap{};
    std::map<std::string, OckServiceAdapter *> serviceAdapterMap{};
};
using HpcServiceManagerPtr = ock::common::Ref<OckServiceManager>;
} // namespace daemon
} // namespace ock
#endif
