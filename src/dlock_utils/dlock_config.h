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
#ifndef UBSM_DLOCK_DLOCK_CONFIG_H
#define UBSM_DLOCK_DLOCK_CONFIG_H

#include "dlock_common.h"
#include "dlock_types.h"

namespace ock {
namespace dlock_utils {

constexpr auto MAX_CLIENT_NUM = 500;
constexpr auto DEFAULT_SERVER_NUM = 1;
constexpr auto DEFAULT_SERVER_ID = 1;
constexpr auto DEFAULT_CMD_CPU_SET = "0-3";
constexpr auto DEFAULT_DLOCK_CLIENT_NUM = 8;
constexpr auto DEFAULT_TRY_LOCK_COUNT = 5;
constexpr auto DEFAULT_TRY_COUNT = 3;
constexpr auto DEFAULT_RETRY_INTERVAL = 1;
constexpr auto DEFAULT_HEART_BEAT_TIMEOUT = 1;
constexpr auto SCRLOCK_DEV_EID_SIZE = 16;

class DLockConfig {
public:
    unsigned int numOfReplica = 0;
    unsigned int recoveryClientNum = 0;
    bool sleepMode = false;
    uint32_t dlockClientNum = DEFAULT_DLOCK_CLIENT_NUM;
    bool isDlockServer = false;
    bool isDlockClient = false;
    int dlockLogLevel = DLOCK_LOG_LEVEL_DEBUG;
    unsigned int maxServerNum = DEFAULT_SERVER_NUM;
    int serverId = DEFAULT_SERVER_ID;
    std::string serverIp;
    std::string clientIp;
    std::string dlockDevName;
    std::string dlockDevEid;
    uint16_t serverPort = 21616; // default port: 21616
    std::string cmdCpuSet = DEFAULT_CMD_CPU_SET;
    uint32_t leaseTime = UINT32_MAX;
    unsigned int lockType = dlock::DLOCK_FAIR;
    unsigned int lockExpireTime = 300; // 默认锁有效时间300s, 配置可修改
    bool enableTls = false;
    bool ubToken = true; // dlock的ub_token_disable为false时, 开启ub_token
};
} // namespace dlock_utils
} // namespace ock
#endif // UBSM_DLOCK_DLOCK_CONFIG_H