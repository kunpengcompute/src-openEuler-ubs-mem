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
#ifndef OCK_MEM_SHARE_MANAGER_H
#define OCK_MEM_SHARE_MANAGER_H

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "log.h"
#include "record_store_def.h"
#include "ubs_common_types.h"

namespace ock::share::service {

struct AttachedShareMemory {
    std::string name;
    size_t size;
    ubsm::RecordState state;
    std::unordered_set<pid_t> pids;
    ock::ubsm::ShareMemImportResult result;
};

class SHMManager {
public:
    static SHMManager &GetInstance()
    {
        static SHMManager instance;
        return instance;
    }

    static int32_t Initial();

    /*
     * 增加应用引用计数
     */
    int32_t AddMemoryUserInfo(const std::string &name, pid_t userProcessId);
    /*
     * 减少应用应用计数
     */
    int32_t RemoveMemoryUserInfo(const std::string &name, pid_t userProcessId);

    int32_t PrepareAddShareMemoryInfo(const std::string &name, size_t size, pid_t processId);

    int32_t AddFullShareMemoryInfo(const std::string &name, pid_t pid, const ock::ubsm::ShareMemImportResult &result);

    int32_t UpdateShareMemoryRecordState(const std::string &name, ubsm::RecordState state);
    /*
     * 删除共享内存导入对象
     */
    int32_t RemoveMemoryInfo(const std::string &name, bool force = false);

    /*
     * 获取共享内存导入对象
     */
    int32_t GetShareMemoryInfo(const std::string &name, AttachedShareMemory &memory);

    /*
     * 获取pid对应的所有共享内存
     */
    std::vector<AttachedShareMemory> GetMemoryUsersByPid(uint32_t pid);

    /*
     * 获取name对应的共享内存引用计数, 如果没有这个共享内存的导入对象也返回成功，count为0
     */
    int32_t GetMemoryUsersCountByName(const std::string &name, size_t &count);

    static int32_t Recovery();

    std::unordered_set<pid_t> GetAllUsers();

private:
    int RemovePreAddRecord(const ubsm::ShareMemImportInfo &record, bool &needToDelete);
    int RollbackPreDeleteRecord(const ubsm::ShareMemImportInfo &record, ubsm::RecordState &state);
    SHMManager() = default;

    int32_t RecoverFromRecord();
    int32_t RecoverFromRefRecord();
    SHMManager(const SHMManager &other) = delete;
    SHMManager(SHMManager &&other) = delete;
    SHMManager &operator=(const SHMManager &other) = delete;
    SHMManager &operator=(SHMManager &&other) noexcept = delete;

    std::mutex mapMutex_{};
    std::unordered_map<std::string, AttachedShareMemory> attachedShareMemory_{};
};
} // namespace ock::share::service
#endif // OCK_MEM_SHARE_MANAGER_H