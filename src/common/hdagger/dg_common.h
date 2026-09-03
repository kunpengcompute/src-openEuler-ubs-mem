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
#ifndef HDAGGER_DEFINES_H
#define HDAGGER_DEFINES_H

#ifndef DAGGER_LIKELY
#define DAGGER_LIKELY(x) (__builtin_expect(!!(x), 1) != 0)
#endif

#ifndef DAGGER_UNLIKELY
#define DAGGER_UNLIKELY(x) (__builtin_expect(!!(x), 0) != 0)
#endif

#ifndef DAGGER_DEFINE_REF_COUNT_FUNCTIONS
#define DAGGER_DEFINE_REF_COUNT_FUNCTIONS                  \
public:                                                    \
    inline void IncreaseRef()                              \
    {                                                      \
        __sync_fetch_and_add(&mRefCount, 1);               \
    }                                                      \
                                                           \
    inline void DecreaseRef()                              \
    {                                                      \
        int32_t tmp = __sync_sub_and_fetch(&mRefCount, 1); \
        if (tmp == 0) {                                    \
            delete this;                                   \
        }                                                  \
    }                                                      \
                                                           \
    inline int32_t GetRef()                                \
    {                                                      \
        return __sync_sub_and_fetch(&mRefCount, 0);        \
    }
#endif

#ifndef DAGGER_DEFINE_REF_COUNT_VARIABLE
#define DAGGER_DEFINE_REF_COUNT_VARIABLE \
private:                                 \
    int32_t mRefCount = 0;
#endif

#define DAGGER_POWER_OF_2(x) ((((x)-1) & (x)) == 0)

#include <cstdint>

constexpr uint32_t DG_8192 = 8192;
constexpr uint32_t DG_4096 = 4096;
constexpr uint32_t DG_1024 = 1024;
constexpr uint32_t DG_256 = 256;
constexpr uint32_t DG_128 = 128;
constexpr uint32_t DG_48 = 48;
constexpr uint32_t DG_32 = 32;
constexpr uint32_t DG_16 = 16;
constexpr uint32_t DG_12 = 12;
constexpr uint32_t DG_8 = 8;
constexpr uint32_t DG_6 = 6;
constexpr uint32_t DG_5 = 5;
constexpr uint32_t DG_4 = 4;
constexpr uint32_t DG_3 = 3;
constexpr uint32_t DG_2 = 2;
constexpr uint32_t DG_1 = 1;

constexpr int32_t DG_0 = 0;
constexpr int32_t DG_M1 = -1;

#endif // HDAGGER_DEFINES_H
