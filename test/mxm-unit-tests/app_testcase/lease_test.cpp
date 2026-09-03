/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#include <dlfcn.h>
#include <mockcpp/mokc.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <ctime>
#include "app_ipc_stub.h"
#include "gtest/gtest.h"
#include "mls_manager.h"
#include "system_adapter.h"

using namespace ock::mxmd;
using namespace ock::ubsm;
namespace UT {

class LeaseTest : public testing::Test {
protected:
    void SetUp() override
    {
        void *notNullPtr = reinterpret_cast<void *>(1);
        MOCKER(SystemAdapter::DlOpen).stubs().will(returnValue(notNullPtr));
        MOCKER(SystemAdapter::DlClose).stubs().will(returnValue(0));
        MOCKER(SystemAdapter::DlSym).stubs().will(returnValue(notNullPtr));
    }

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
    }
};

struct CtxInfo {
    sem_t sem;
    int ret;
    void *addr;
};

static void LeaseTestAsyncFreeCallBack(intptr_t ctx, int result)
{
    CtxInfo *info = reinterpret_cast<CtxInfo *>(ctx);
    info->ret = result;
    sem_post(&info->sem);
}

} // namespace UT