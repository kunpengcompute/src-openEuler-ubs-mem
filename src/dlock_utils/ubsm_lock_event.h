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
#ifndef UBSM_LOCK_EVENT_H
#define UBSM_LOCK_EVENT_H
#include <cstdint>
#include "election_module.h"
#include "zen_discovery.h"

namespace ock {
namespace dlock_utils {
using EventType = zendiscovery::ElectionModule::ZenElectionEventType;
class UbsmLockEvent {
public:
    static void HandleElectionEvent(EventType event, const std::string &masterId, const std::string &clientId);

    // 选主前的准备工作
    static void DoPreElection();

    static void OnMasterElected(const std::string &masterId);

    // 故障恢复: client节点初始化
    static void OnDLockClientInit(const std::string &masterId);

    // 故障恢复: 节点降级事件
    static void OnDLockDemoted();

    // 故障恢复: 节点锁恢复
    static void OnDLockServerRecovery(const std::string &masterId, const std::string &clientId);

private:
    static int32_t DoRecovery(const rpc::RpcNode &masterNode, const rpc::RpcNode &reinitNode);
    // 选主成功以后初始化锁服务
    static void DoDLockServerInit(const std::string &serverIp);
};
} // namespace dlock_utils
} // namespace ock
#endif // UBSM_LOCK_EVENT_H