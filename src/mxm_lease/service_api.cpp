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
#include "service_api.h"
#include <iostream>
#include "mem_lease_service.h"
#include "shm_service_api.h"

static ock::common::Service *g_shmService = nullptr;
void *Create()
{
    g_shmService = static_cast<ock::common::Service *>(ShmCreate());
    if (g_shmService == nullptr) {
        return nullptr;
    }
    auto *leaseService = new (std::nothrow) ock::lease::service::MemLeaseService();
    if (leaseService == nullptr) {
        ShmDestroy(g_shmService);
        return nullptr;
    }
    return leaseService;
}

int ServiceProcessArgs(void *service, int argc, char *argv[])
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    if (g_shmService == nullptr || ShmServiceProcessArgs(g_shmService, argc, argv) != 0) {
        return -1;
    }
    return real->ServiceProcessArgs(argc, argv);
}

int ServiceInitialize(void *service)
{
    if (g_shmService == nullptr || ShmServiceInitialize(g_shmService) != 0) {
        return -1;
    }
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceInitialize();
}

int ServiceStart(void *service)
{
    if (g_shmService == nullptr || ShmServiceStart(g_shmService) != 0) {
        return -1;
    }
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceStart();
}

int ServiceHealthy(void *service)
{
    if (g_shmService == nullptr || ShmServiceHealthy(g_shmService) != 0) {
        return -1;
    }
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceHealthy();
}

int ServiceShutdown(void *service)
{
    if (g_shmService == nullptr || ShmServiceShutdown(g_shmService) != 0) {
        return -1;
    }
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceShutdown();
}

int ServiceUninitialize(void *service)
{
    if (g_shmService == nullptr || ShmServiceUninitialize(g_shmService) != 0) {
        return -1;
    }
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceUninitialize();
}

void Destroy(void *service)
{
    if (g_shmService != nullptr) {
        ShmDestroy(g_shmService);
    }
    auto real = static_cast<ock::common::Service *>(service);
    delete real;
}