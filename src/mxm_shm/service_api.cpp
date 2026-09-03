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
#include <iostream>
#include "mem_share_service.h"
#include "shm_service_api.h"

void *ShmCreate()
{
    auto *leaseService = new (std::nothrow) ock::share::service::MemShareService();
    return leaseService;
}

int ShmServiceProcessArgs(void *service, int argc, char *argv[])
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceProcessArgs(argc, argv);
}

int ShmServiceInitialize(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceInitialize();
}

int ShmServiceStart(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceStart();
}

int ShmServiceHealthy(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceHealthy();
}

int ShmServiceShutdown(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceShutdown();
}

int ShmServiceUninitialize(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    if (real == nullptr) {
        return -1;
    }
    return real->ServiceUninitialize();
}

void ShmDestroy(void *service)
{
    auto real = static_cast<ock::common::Service *>(service);
    delete real;
}