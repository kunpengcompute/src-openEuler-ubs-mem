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
#include "ock_service.h"

#include <dlfcn.h>
#include <thread>

using namespace ock::daemon;

void *OckService::GetAgentMethodP(const std::string &name)
{
    char *error;
    dlerror();
    void *agentP = dlsym(serviceHandler, name.data());
    if ((error = dlerror())) {
        DBG_LOGERROR("init method error with code: " << error << ", method name: " << name);
        return nullptr;
    }
    return agentP;
}

void *OckService::GetAgentDefaultedMethodP(const std::string &name)
{
    char *error;
    dlerror();
    void *agentP = dlsym(serviceHandler, name.data());
    if ((error = dlerror())) {
        return nullptr;
    }
    return agentP;
}

OckService::~OckService()
{
    if (agentDestroyP != nullptr && ockServiceAgentP != nullptr) {
        (*agentDestroyP)(ockServiceAgentP);
    }
    if (serviceHandler != nullptr) {
        dlclose(serviceHandler);
    }
}

HRESULT OckService::Initialize(OckServiceAdapter *serviceConf)
{
    if (serviceConf == nullptr) {
        DBG_LOGERROR("OckService: service initialize with a null adapter pointer.");
        return HFAIL;
    }
    auto libPath = serviceConf->LibPath();
    if (!OckFileDirExists(libPath) || !(serviceHandler = dlopen(libPath.data(), RTLD_NOW | RTLD_GLOBAL))) {
        DBG_LOGERROR("OckService: service " << serviceConf->Name() << " open lib <" << libPath
                                            << "> failed with error: " << dlerror());
        return HFAIL;
    }
    agentCreateP = reinterpret_cast<CreateT>(GetAgentMethodP(SERVICE_METHOD_CREATE));
    if (agentCreateP == nullptr) {
        return HFAIL;
    }
    agentProcessArgsP = reinterpret_cast<ServiceProcessArgsT>(GetAgentMethodP(SERVICE_METHOD_PROCESS_ARGS));
    agentInitializeP = reinterpret_cast<ServiceInitializeT>(GetAgentMethodP(SERVICE_METHOD_INITIALIZE));
    agentStartP = reinterpret_cast<ServiceStartT>(GetAgentMethodP(SERVICE_METHOD_START));
    agentHealthyP = reinterpret_cast<ServiceHealthyT>(GetAgentDefaultedMethodP(SERVICE_METHOD_HEALTHY));
    agentShutdownP = reinterpret_cast<ServiceShutdownT>(GetAgentMethodP(SERVICE_METHOD_SHUTDOWN));
    agentUninitializeP = reinterpret_cast<ServiceUninitializeT>(GetAgentMethodP(SERVICE_METHOD_UN_INITIALIZE));
    agentDestroyP = reinterpret_cast<DestroyT>(GetAgentMethodP(SERVICE_METHOD_DESTROY));
    ockServiceAgentP = (*agentCreateP)();
    if (ockServiceAgentP == nullptr) {
        return HFAIL;
    }
    serviceName = serviceConf->Name();
    constructStatus = HOK;
    return HOK;
}

const std::vector<std::string> &OckService::ServiceApplyArgs()
{
    mArgNames = std::vector<std::string>();
    if (!ockServiceAgentP) {
        DBG_LOGERROR("OckService: " << serviceName << ", agent is NULL.");
        return mArgNames;
    }
    mArgNames = reinterpret_cast<Service *>(ockServiceAgentP)->ServiceApplyArgs();
    return mArgNames;
}

HRESULT OckService::OnServiceProcessArgs(int argc, char **argv)
{
    DBG_LOGINFO("OckService: " << serviceName << " OnServiceProcessArgs.");
    if (constructStatus != HOK) {
        return HFAIL;
    }
    processArgsStatus = HFAIL;
    if (constructStatus != HOK || !ockServiceAgentP || !agentProcessArgsP) {
        return processArgsStatus;
    }
    processArgsStatus = (*agentProcessArgsP)(ockServiceAgentP, argc, argv);
    return processArgsStatus;
}

HRESULT OckService::OnServiceInitialize()
{
    DBG_LOGINFO("OckService: " << serviceName << " OnServiceInitialize.");
    if (constructStatus != HOK) {
        return HFAIL;
    }
    initializeStatus = HFAIL;
    if (!ockServiceAgentP || !agentInitializeP) {
        return processArgsStatus;
    }
    initializeStatus = (*agentInitializeP)(ockServiceAgentP);
    return initializeStatus;
}

HRESULT OckService::OnServiceStart()
{
    DBG_LOGINFO("OckService: " << serviceName << "OnServiceStart.");
    if (constructStatus != HOK) {
        return HFAIL;
    }
    startStatus = HFAIL;
    if (!ockServiceAgentP || !agentStartP) {
        return processArgsStatus;
    }
    startStatus = (*agentStartP)(ockServiceAgentP);
    return startStatus;
}

HRESULT OckService::OnServiceHealthy()
{
    if (constructStatus != HOK) {
        return HFAIL;
    }

    if (startStatus != HOK) {
        return HFAIL;
    }

    if (!ockServiceAgentP || !agentHealthyP) {
        return HOK;
    }

    return (*agentHealthyP)(ockServiceAgentP);
}

HRESULT OckService::OnServiceShutdown()
{
    DBG_LOGINFO("OckService: " << serviceName << " OnServiceShutdown.");
    if (constructStatus != HOK) {
        return HFAIL;
    }
    shutdownStatus = HFAIL;
    if (!ockServiceAgentP || !agentShutdownP) {
        return processArgsStatus;
    }
    shutdownStatus = (*agentShutdownP)(ockServiceAgentP);
    return shutdownStatus;
}

HRESULT OckService::OnServiceUninitialize()
{
    DBG_LOGINFO("OckService: " << serviceName << " OnServiceUninitialize.");
    if (constructStatus != HOK) {
        return HFAIL;
    }
    uninitializeStatus = HFAIL;
    if (!ockServiceAgentP || !agentUninitializeP) {
        return processArgsStatus;
    }
    uninitializeStatus = (*agentUninitializeP)(ockServiceAgentP);
    return uninitializeStatus;
}
