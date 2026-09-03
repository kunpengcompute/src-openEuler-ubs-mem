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

#ifndef RACK_MEMORYFABRIC_MACROS_H
#define RACK_MEMORYFABRIC_MACROS_H

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
#ifndef ROUND_UP
#define ROUND_UP(x, align) \
    ((static_cast<uint64_t>(x) + static_cast<uint64_t>(align) - 1) & ~static_cast<uint64_t>((align)-1))
#endif

#endif