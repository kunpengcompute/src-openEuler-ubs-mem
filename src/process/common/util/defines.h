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
#ifndef OCK_COMMON_COMMON_UTIL_TYPES_H
#define OCK_COMMON_COMMON_UTIL_TYPES_H

#include <cstdint>
#include <string>

using HRESULT = uint32_t;
#ifndef HOK
#define HOK (HRESULT(0))
#define HFAIL (HRESULT(1))
#endif

#ifndef BLOCK_INDEX_SIZE
#define BLOCK_INDEX_SIZE (128)
#endif

namespace ock::common {
#define SAFE_DELETE(p) \
    do {               \
        delete (p);    \
        p = nullptr;   \
    } while (0)

#define SAFE_DELETE_ARRAY(p) \
    do {                     \
        delete[](p);         \
        p = nullptr;         \
    } while (0)

#define IS_DIGIT(s) ((s) >= '0' && (s) <= '9')
#define IS_LETTER(s) (((s) >= 'a' && (s) <= 'z') || ((s) >= 'A' && (s) <= 'Z'))

#ifndef LIKELY
#define LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#ifndef UNLIKELY
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#ifndef BM_UUID_LENGTH
const int8_t BM_UUID_LENGTH = 64;
const int8_t BM_UUID_REAL_LEN = 64;
#endif

const int LOGMASK = 7;
const int16_t MASK_16 = 0xff;
const int16_t SHIFT_1000 = 1000;
const int16_t SHIFT_10000 = 10000;
const uint64_t SHIFT_1000_3 = 1000000000;
const int8_t SHIFT_56 = 56;
const int8_t SHIFT_40 = 40;
const int8_t SHIFT_32 = 32;
const int8_t SHIFT_29 = 29;
const int8_t SHIFT_24 = 24;
const int8_t SHIFT_16 = 16;
const int8_t SHIFT_12 = 12;
const int8_t SHIFT_8 = 8;
const int8_t SHIFT_6 = 6;
const int8_t SHIFT_5 = 5;
const int8_t SHIFT_4 = 4;
const int8_t SHIFT_3 = 3;
const int8_t SHIFT_2 = 2;
const int8_t SHIFT_1 = 1;
const int8_t SHIFT_0 = 0;
const int8_t SORT_INDEX_TUPLE_SIZE = 3; // sort aggregator's index is a 3-tuple
const int8_t RPC_MAX_RETRY_TIME = 5;
const int8_t OPERATION_RETRY_INTERVAL = 5;
const int16_t HEART_BEAT_TIMEOUT = 600;
const int8_t HEART_BEAT_SEND_INTERVAL = 10;
const int8_t HEART_BEAT_WATCH_INTERVAL = 10;
const int LOG_BUFF_SIZE = 4096;
const int LOG_TO_FILE = 2;
const int SWAP_IN_ATTEMPT_NUM = 100;
const int SWAP_IN_PRINT_INTERVAL = 10;
constexpr uint32_t MAX_LENGTH_COMMON_PATH = 1024;
constexpr uint32_t MAX_LENGTH_COMMON_CONFIG = 4096;
const int UCX_CONNECT_TIMEOUT = 900000;
const uint32_t ONE_THOUSAND = 1000;
const uint32_t ONE = 1;

const int CLEAN_EXIT_CODE = 0;
const int ERROR_EXIT_CODE = -1;

const int MAX_CONF_FILE_SIZE = 10485760; // 10MB

} // namespace ock::common

#endif