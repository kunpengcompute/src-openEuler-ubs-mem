/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include <gtest/gtest.h>
#include <cstdio>
#include "secodefuzz/secodeFuzz.h"
#include "test_env_init.h"
#ifdef DEBUG_MEM_FUZZ
int main(int argc, char **argv)
{
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