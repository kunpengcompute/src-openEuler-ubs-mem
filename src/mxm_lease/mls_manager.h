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
#ifndef OCK_MEM_LEASE_MANAGER_H
#define OCK_MEM_LEASE_MANAGER_H

#include <unistd.h>
#include <map>
#include <mutex>
#include <string>
#include "log.h"
#include "record_store.h"
#include "ubs_common_types.h"
#include "ubse_mem_adapter.h"

namespace ock::lease::service {

const uint32_t DEFAULT_MAX_BUFFER_MEM_NUM = 5u;

/**
 * @brief 内存借用信息的用于比较的key，主要用于缓存中，以size作第一比较
 */
struct MLSMemKey {
    const uint64_t size;
    const std::string name;

    explicit MLSMemKey(uint64_t sz) noexcept : size{sz} {}

    MLSMemKey(uint64_t sz, std::string nm) noexcept : size{sz}, name{std::move(nm)} {}

    bool operator<(const MLSMemKey &other) const noexcept
    {
        if (size != other.size) {
            return size < other.size;
        }
        return name < other.name;
    }
};

struct MLSMemInfo {
    std::string name;
    ock::ubsm::AppContext appContext;
    uint64_t size;
    int64_t numaId;
    std::vector<uint64_t> memIds;
    uint64_t unitSize;
    uint16_t perflevel;
    uint32_t slotId; // 记录借出内存节点
    bool isNuma;
    ubsm::RecordState state;
};

class MLSManager {
public:
    static MLSManager &GetInstance()
    {
        static MLSManager instance;
        return instance;
    }

    static int32_t Init();
    static int32_t Finalize();
    static int32_t Recovery();
    void SetBufferedNum(uint32_t num);
    int32_t RecoverFromRecord();
    int32_t ReuseBufferedMem(uint64_t size, uint16_t isNuma, const std::string &regionName,
                             const ock::ubsm::AppContext &context, MLSMemInfo &info);
    // 如果缓存空间有剩余，直接放到缓存空间，否则将record状态置为PRE_DEL
    int32_t PreDeleteUsedMem(const std::string &name);
    int32_t DeleteUsedMem(const std::string &name);
    int32_t DeleteAllBufferedMem();
    std::vector<MLSMemInfo> ListAllMem();
    int32_t GetUsedMemByName(const std::string &name, MLSMemInfo &info);
    std::vector<MLSMemInfo> GetUsedMemByPid(uint32_t pid);
    std::string GenerateMemName(uint64_t size);

    int PreAddUsedMem(const std::string &name, size_t size, const ubsm::AppContext &usr, bool isNuma,
                      uint16_t perfLevel);
    int FinishAddUsedMem(const std::string &name, int64_t numaId, size_t unitSize, uint32_t slotId,
                         const std::vector<uint64_t> &memIds);

    int32_t UpdateMemRecordState(const std::string &name, ock::ubsm::RecordState state);

private:
    MLSManager() = default;
    MLSManager(const MLSManager &other) = delete;
    MLSManager(MLSManager &&other) = delete;
    MLSManager &operator=(const MLSManager &other) = delete;
    MLSManager &operator=(MLSManager &&other) noexcept = delete;
    int32_t BufferUsedMemory(MLSMemInfo &usedInfo);
    int32_t ReuseMemInSlotId(uint64_t size, uint16_t isNuma, const std::vector<uint32_t> &slotIds,
                             const ubsm::AppContext &context, MLSMemInfo &bufferInfo);

    int32_t RemovePreAddRecord(ock::ubsm::MemLeaseInfo &record, bool &needToDelete);
    int32_t RollbackPreDeleteRecord(const ock::ubsm::MemLeaseInfo &record, ock::ubsm::RecordState &recordState);
    bool isEnableLeaseBuffered_{true};
    uint32_t maxBufferedMemNum_{DEFAULT_MAX_BUFFER_MEM_NUM};

    std::mutex mapMutex_{};
    std::map<std::string, MLSMemInfo> usedMemory_{};
    std::map<MLSMemKey, MLSMemInfo> numaBufferedMemory_{};
    std::map<MLSMemKey, MLSMemInfo> fdBufferedMemory_{};
};

} // namespace ock::lease::service
#endif // OCK_MEM_LEASE_MANAGER_H