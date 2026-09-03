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
#ifndef SERVICE_H
#define SERVICE_H

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "log.h"
#include "util/conf_constants.h"
#include "util/defines.h"
#include "util/ref.h"
namespace ock {
namespace common {

class Service : public Referable {
public:
    Service() = default;
    ~Service() override = default;

    HRESULT ServiceProcessArgs(int argc, char *argv[])
    {
        HRESULT hr = OnServiceProcessArgs(argc, argv);
        if (hr != 0) {
            DBG_LOGERROR("service failed to process args { argc " << argc << ", argv " << argv << " }");
        }
        return hr;
    }

    HRESULT ServiceInitialize()
    {
        return OnServiceInitialize();
    }

    HRESULT ServiceStart()
    {
        HRESULT hr = OnServiceStart();
        if (hr != 0) {
            DBG_LOGERROR("Failed to service start");
        }
        return hr;
    }

    HRESULT ServiceHealthy()
    {
        HRESULT hr = OnServiceHealthy();
        if (hr != 0) {
            DBG_LOGERROR("service is not healthy");
        }
        return hr;
    }

    HRESULT ServiceShutdown()
    {
        HRESULT hr = OnServiceShutdown();
        if (hr != 0) {
            DBG_LOGERROR("Failed to service shutdown");
        }
        return hr;
    }

    HRESULT ServiceUninitialize()
    {
        HRESULT hr = OnServiceUninitialize();
        if (hr != 0) {
            DBG_LOGERROR("Failed to service UnInitialize");
        }
        return hr;
    }

    std::string ServiceName()
    {
        return serviceName;
    }

    const std::vector<std::string> &ServiceApplyArgs() const
    {
        return serviceArgs;
    }

protected:
    virtual HRESULT OnServiceProcessArgs(int argc, char *argv[]) = 0;
    virtual HRESULT OnServiceInitialize() = 0;
    virtual HRESULT OnServiceStart() = 0;
    virtual HRESULT OnServiceShutdown() = 0;
    virtual HRESULT OnServiceUninitialize() = 0;
    virtual HRESULT OnServiceHealthy()
    {
        return HOK;
    }

    virtual void PrintVersionInfo() = 0;

    std::string serviceName;
    std::vector<std::string> serviceArgs{};
};

using ServicePtr = Ref<Service>;

} // namespace common
} // namespace ock

#endif