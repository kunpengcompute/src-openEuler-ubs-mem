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
#include "ock_service_manager.h"
namespace ock {
namespace daemon {

OckServiceManager::~OckServiceManager()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto serviceIter : serviceMap) {
        if (serviceIter.second != nullptr) {
            SAFE_DELETE(serviceIter.second);
        }
    }
    for (auto adapterIter : serviceAdapterMap) {
        if (adapterIter.second != nullptr) {
            SAFE_DELETE(adapterIter.second);
        }
    }
}

HRESULT OckServiceManager::ServicePut(const std::vector<std::string> &services, const std::string &libHomePath)
{
    if ((services.size() < 1) || (services.size() > 2)) { // 2
        DBG_LOGERROR("Services count should be within the range of [1, 2], size is: " << services.size());
        return HFAIL;
    }
    for (auto &service : services) {
        std::string serviceName = service;
        OckTrimString(serviceName);
        std::string lowerServiceName = serviceName;
        transform(lowerServiceName.begin(), lowerServiceName.end(), lowerServiceName.begin(), ::tolower);
        std::string libPath = libHomePath + "/lib/lib" + lowerServiceName + ".so";
        if (libPath.size() > PATH_MAX) {
            DBG_LOGERROR("libPath size exceeds maximum limit.");
            return HFAIL;
        }

        OckServiceAdapter *adapter = new (std::nothrow) OckServiceAdapter(serviceName, libPath);
        if (adapter == nullptr) {
            DBG_LOGERROR("Fail to create service adapter.");
            return HFAIL;
        }
        auto *serviceP = new (std::nothrow) OckService();
        if (serviceP == nullptr) {
            DBG_LOGERROR("Service(" << serviceName.c_str() << ") build failed, ignored.");
            SAFE_DELETE(adapter);
            continue;
        }
        if (serviceP->Initialize(adapter) != 0) {
            DBG_LOGERROR("Service(" << serviceName.c_str() << ") build failed, ignored.");
            SAFE_DELETE(adapter);
            SAFE_DELETE(serviceP);
            continue;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        serviceAdapterMap.insert(std::make_pair(serviceName, adapter));
        serviceMap.insert(std::make_pair(serviceName, serviceP));
    }
    return HOK;
}

HRESULT OckServiceManager::ServiceProcessArgs()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (const auto &adapterIter : serviceAdapterMap) {
        auto &name = adapterIter.first;
        if (adapterIter.second == nullptr) {
            DBG_LOGERROR("Ock service adapter of " << name.c_str() << "is NULL. ");
            continue;
        }
        auto serviceIter = serviceMap.find(name);
        if (serviceIter == serviceMap.end()) {
            DBG_LOGERROR("Cannot find service(" << name.c_str() << ") in ock service manager. ");
            continue;
        }
        if (serviceIter->second == nullptr) {
            DBG_LOGERROR("An wrong service(" << name.c_str() << ") in ock service manager.");
            continue;
        }
        OckServiceArgs *serviceArgs = OckServiceAdapter::CreateServiceArgs(serviceIter->second->ServiceApplyArgs());
        if (serviceArgs == nullptr) {
            DBG_LOGERROR("Service(" << name.c_str() << ") create arg array failed.");
            return HFAIL;
        }
        HRESULT ret = serviceIter->second->ServiceProcessArgs(serviceArgs->Argc(), serviceArgs->Argv());
        if (ret != 0) {
            DBG_LOGERROR("Get exception when process args, Service: " << name.c_str());
            SAFE_DELETE(serviceArgs);
            return HFAIL;
        }
        SAFE_DELETE(serviceArgs);
    }
    return HOK;
}

HRESULT OckServiceManager::OnServiceProcessArgs(int argc, char **argv)
{
    return HOK;
}

HRESULT OckServiceManager::OnServiceInitialize()
{
    DBG_LOGINFO("OckServiceManager OnServiceInitialize.");
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &iter : serviceMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("An wrong service in OckServiceManager.");
            continue;
        }

        if (iter.second->ServiceInitialize() != 0) {
            DBG_LOGERROR("Get exception when ServiceInitialize, service name: " << iter.first);
            return HFAIL;
        }
    }
    return HOK;
}

HRESULT OckServiceManager::OnServiceStart()
{
    HRESULT hr = HOK;
    DBG_LOGINFO("OckServiceManager OnServiceStart.");
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &iter : serviceMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("An wrong service in OckServiceManager.");
            continue;
        }

        hr = iter.second->ServiceStart();
        if (hr != 0) {
            DBG_LOGERROR("Get exception when services start as " << iter.first);
        }
    }
    return hr;
}

HRESULT OckServiceManager::OnServiceHealthy()
{
    std::unique_lock<std::mutex> lock(mutex_);
    HRESULT hr = HOK;
    for (auto &iter : serviceMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("An wrong service in OckServiceManager.");
            continue;
        }

        hr = iter.second->ServiceHealthy();
        if (hr != 0) {
            DBG_LOGWARN("OckServiceManager service(" << iter.first << ") not healthy.");
        }
    }
    return hr;
}

HRESULT OckServiceManager::OnServiceShutdown()
{
    std::unique_lock<std::mutex> lock(mutex_);
    HRESULT hr = HOK;
    DBG_LOGINFO("OckServiceManager OnServiceShutdown.");
    for (auto &iter : serviceMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("An wrong service in OckServiceManager.");
            continue;
        }

        hr = iter.second->ServiceShutdown();
        if (hr != 0) {
            DBG_LOGERROR("Get exception when ServiceShutdown, service name: " << iter.first);
        }
    }
    return hr;
}

HRESULT OckServiceManager::OnServiceUninitialize()
{
    std::unique_lock<std::mutex> lock(mutex_);
    HRESULT hr = HOK;
    DBG_LOGINFO("OckServiceManager OnServiceUninitialize.");
    for (auto &iter : serviceMap) {
        if (iter.second == nullptr) {
            DBG_LOGERROR("An wrong service in OckServiceManager.");
            continue;
        }

        hr = iter.second->ServiceUninitialize();
        if (hr != 0) {
            DBG_LOGERROR("Get exception when ServiceUninitialize, service name: " << iter.first);
        }
    }
    return hr;
}

HRESULT OckServiceManager::ServiceInitialize(const std::string &serviceName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    DBG_LOGINFO("execute ServiceInitialize with serviceName: " << serviceName);
    auto iter = serviceMap.find(serviceName);
    if (iter == serviceMap.end()) {
        DBG_LOGERROR("not exist service with serviceName: " << serviceName);
        return HFAIL;
    }
    if (iter->second == nullptr) {
        DBG_LOGERROR("A wrong service in OckServiceManager.");
        return HFAIL;
    }
    return iter->second->ServiceInitialize();
}

HRESULT OckServiceManager::ServiceStart(const std::string &serviceName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    DBG_LOGINFO("execute ServiceStart with serviceName: " << serviceName);
    auto iter = serviceMap.find(serviceName);
    if (iter == serviceMap.end()) {
        DBG_LOGERROR("not exist service with serviceName: " << serviceName);
        return HFAIL;
    }
    if (iter->second == nullptr) {
        DBG_LOGERROR("A wrong service in OckServiceManager.");
        return HFAIL;
    }
    return iter->second->ServiceStart();
}

HRESULT OckServiceManager::ServiceShutdown(const std::string &serviceName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    DBG_LOGINFO("execute ServiceShutdown with serviceName: " << serviceName);
    auto iter = serviceMap.find(serviceName);
    if (iter == serviceMap.end()) {
        DBG_LOGERROR("not exist service with serviceName: " << serviceName);
        return HFAIL;
    }
    if (iter->second == nullptr) {
        DBG_LOGERROR("A wrong service in OckServiceManager.");
        return HFAIL;
    }
    return iter->second->ServiceShutdown();
}

HRESULT OckServiceManager::ServiceUninitialize(const std::string &serviceName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    DBG_LOGINFO("execute ServiceUninitialize with serviceName: " << serviceName);
    auto iter = serviceMap.find(serviceName);
    if (iter == serviceMap.end()) {
        DBG_LOGERROR("not exist service with serviceName: " << serviceName);
        return HFAIL;
    }
    if (iter->second == nullptr) {
        DBG_LOGERROR("A wrong service in OckServiceManager.");
        return HFAIL;
    }
    return iter->second->ServiceUninitialize();
}
} // namespace daemon
} // namespace ock
