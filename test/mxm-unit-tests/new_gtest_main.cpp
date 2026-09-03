/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include <gtest/gtest.h>
#include <cstdint>

#include "test_env_init.h"

#ifdef DEBUG_MEM_UT
int main(int argc, char **argv)
{
    //         pool的UT环境初始化
    auto ret = UT::TestEnvInit();
    if (ret != 0) {
        return ret;
    }
    testing::InitGoogleTest(&argc, argv);
    ret = RUN_ALL_TESTS();
    UT::PoolExit();
    return ret;
}
#endif