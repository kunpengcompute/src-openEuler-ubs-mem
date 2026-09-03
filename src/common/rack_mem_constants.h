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

#ifndef RACK_MEMORYFABRIC_CONSTANTS_H
#define RACK_MEMORYFABRIC_CONSTANTS_H

#include <regex.h>
#include <securec.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <list>
#include <random>
#include <set>
#include <string>
#include <vector>

#define INVALID_MEM_ID 0

namespace ock::mxmd {
constexpr int ONE_M = 1024 * 1024;
constexpr uint32_t CONNECT_NUM = 256;

// 数字常量宏，代表数字1，2，3，4...
constexpr int16_t NO_0 = 0;
constexpr int16_t NO_1 = 1;
constexpr int16_t NO_2 = 2;
constexpr int16_t NO_3 = 3;
constexpr int16_t NO_4 = 4;
constexpr int16_t NO_16 = 16;
constexpr int16_t NO_32 = 32;
constexpr int16_t NO_64 = 64;
constexpr int16_t NO_128 = 128;
constexpr int16_t NO_1000 = 1000;
constexpr int16_t NO_1024 = 1024;
constexpr int16_t NO_2048 = 2048;
constexpr int32_t NO_60000 = 60000;
} // namespace ock::mxmd
#endif