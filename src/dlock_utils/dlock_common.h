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
#ifndef UBSM_DLOCK_DLOCK_COMMON_H
#define UBSM_DLOCK_DLOCK_COMMON_H

#include <cstdint>
#include <string>

namespace ock {
namespace dlock_utils {

enum class REINIT_STAGES { CLIENT_REINIT = 0, UPDATE_LOCK, CLIENT_REINIT_DONE };

const std::string &GetReinitStageName(REINIT_STAGES stage);

enum DLockLogLevel {
    DLOCK_LOG_LEVEL_EMERG = 0,
    DLOCK_LOG_LEVEL_ALERT = 1,
    DLOCK_LOG_LEVEL_CRIT = 2,
    DLOCK_LOG_LEVEL_ERR = 3,
    DLOCK_LOG_LEVEL_WARNING = 4,
    DLOCK_LOG_LEVEL_NOTICE = 5,
    DLOCK_LOG_LEVEL_INFO = 6,
    DLOCK_LOG_LEVEL_DEBUG = 7,
};

DLockLogLevel MapUbsmLogLevel2DLockLevel(int32_t level);

} // namespace dlock_utils
} // namespace ock
#endif // UBSM_DLOCK_DLOCK_COMMON_H
