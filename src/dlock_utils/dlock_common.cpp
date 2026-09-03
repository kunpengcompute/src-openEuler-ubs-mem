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

#include "dlock_common.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "log.h"

namespace ock {
namespace dlock_utils {
const std::string &GetReinitStageName(REINIT_STAGES stage)
{
    static const std::vector<std::string> names = {"CLIENT_REINIT", "UPDATE_LOCK", "CLIENT_REINIT_DONE"};
    static const std::string unknownStage = "UNKNOWN_STAGE";
    if (static_cast<int>(stage) >= names.size()) {
        return unknownStage;
    }
    return names[static_cast<int>(stage)];
}

DLockLogLevel MapUbsmLogLevel2DLockLevel(int32_t level)
{
    static const std::unordered_map<int32_t, DLockLogLevel> levelMap = {{DBG_LOG_DEBUG, DLOCK_LOG_LEVEL_DEBUG},
                                                                        {DBG_LOG_INFO, DLOCK_LOG_LEVEL_INFO},
                                                                        {DBG_LOG_WARN, DLOCK_LOG_LEVEL_WARNING},
                                                                        {DBG_LOG_ERROR, DLOCK_LOG_LEVEL_ERR},
                                                                        {DBG_LOG_CRITICAL, DLOCK_LOG_LEVEL_CRIT}};
    auto it = levelMap.find(level);
    if (it == levelMap.end()) {
        DBG_LOGWARN("Cannot find loglevel=" << level << ", use dlock info loglevel=" << DLOCK_LOG_LEVEL_INFO);
        return DLOCK_LOG_LEVEL_INFO;
    }
    return it->second;
}
} // namespace dlock_utils
} // namespace ock
