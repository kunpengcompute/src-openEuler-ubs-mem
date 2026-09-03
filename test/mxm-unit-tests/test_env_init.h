/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#ifndef MXM_TEST_ENV_INIT_H
#define MXM_TEST_ENV_INIT_H

#include <sys/mman.h>
#include <iostream>
#include <string>
#include "rack_mem_lib.h"
#include "record_store.h"
#include "ubsm_ptracer.h"

namespace UT {
using namespace ock::ubsm::tracer;
int RecordEnvInit()
{
    std::string shmName = "mxm_record_test";
    auto fd = memfd_create(shmName.c_str(), MFD_CLOEXEC);
    if (fd < 0) {
        printf("memfd_create with name(%s) failed, error(%s)\n", shmName.c_str(), strerror(errno));
        return -1;
    }
    auto ret = ock::ubsm::RecordStore::GetInstance().Initialize(fd);
    if (ret != 0) {
        printf("RecordStore init failed, ret(%d)\n", ret);
        close(fd);
        return ret;
    }
    return 0;
}

inline int TestEnvInit()
{
    UbsmPtracer::Init();
    return RecordEnvInit();
}

inline void PoolExit()
{
    UbsmPtracer::UnInit();
    sleep(2u);
}
} // namespace UT

#endif