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
#ifndef UBS_MEM_LEAK_CLEANER_H
#define UBS_MEM_LEAK_CLEANER_H
#include "ubs_mem_monitor.h"

namespace ock {
namespace ubsm {
/* 该类用于清理UBSMD故障期间用户进程故障而导致的远端内存泄漏 */
bool ProcessAlive(const uint32_t pid);
class UBSMemLeakCleaner {
public:
    static UBSMemLeakCleaner &GetInstance() noexcept;

    ~UBSMemLeakCleaner() = default;

    int Start() noexcept;
    int CleanLeaseMemoryLeakInner(const std::string &name, const AppContext &ctx, bool &changed, bool isTimeOutScene,
                                  bool isNumaLease);
    int CleanShareMemoryLeakInner(const std::string &name, const AppContext &ctx, bool isTimeOutScene, bool isAttach,
                                  uint64_t createSeqNo);
    int SHMProcessDeadProcess(pid_t pid);

    int TryRollBackTimeoutShmTask(const std::string &name, const AppContext &ctx, bool isAttach, uint64_t createSeqNo);

private:
    UBSMemLeakCleaner() noexcept = default;
    int CleanLeaseMemoryLeakWhenStart();
    int CleanShareMemoryLeakWhenStart();
};

} // namespace ubsm
} // namespace ock

#endif // UBS_MEM_LEAK_CLEANER_H
