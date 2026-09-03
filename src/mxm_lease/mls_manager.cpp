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
#include "mls_manager.h"
#include <ctime>
#include <sstream>
#include "mls_repository.h"
#include "ubs_common_config.h"

#include "ubs_mem_monitor.h"

namespace ock::lease::service {
using namespace ock::ubsm;
using namespace ock::common;

constexpr uint64_t NANOSECONDS = 1000000000ULL;

int32_t MLSManager::Init()
{
    return MLSRepository::GetInstance().Init();
}

int32_t MLSManager::Finalize()
{
    return GetInstance().DeleteAllBufferedMem();
}

int32_t MLSManager::Recovery()
{
    return GetInstance().RecoverFromRecord();
}

void MLSManager::SetBufferedNum(uint32_t num)
{
    maxBufferedMemNum_ = num;
}

int32_t MLSManager::RecoverFromRecord()
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto records = MLSRepository::GetInstance().RecoverMemRecord();
    for (auto &record : records) {
        if (record.first.state == RecordState::PRE_ADD) {
            bool needToDelete = false;
            auto ret = RemovePreAddRecord(record, needToDelete);
            if (ret != 0) {
                DBG_LOGERROR("Remove pre add record failed. name=" << record.first.name << " ret=" << ret);
                continue;
            }

            if (needToDelete) {
                DBG_LOGINFO("Remove pre add record successfully. name=" << record.first.name);
                continue;
            }
            DBG_LOGINFO("Memory will be returned in the background. name=" << record.first.name);
        }
        auto state = record.first.state;
        if (record.first.state == RecordState::PRE_DEL) {
            auto ret = RollbackPreDeleteRecord(record, state);
            if (ret != 0) {
                DBG_LOGERROR("Rollback pre delete record failed. name=" << record.first.name << "ret=" << ret);
                continue;
            }
            DBG_LOGINFO("Rollback pre delete record successfully. name=" << record.first.name);
        }
        MLSMemInfo info;
        info.name = record.first.name;
        info.appContext = record.first.appContext;
        info.size = record.first.size;
        info.numaId = record.second.numaId;
        info.memIds.assign(record.second.memIds.begin(), record.second.memIds.end());
        info.unitSize = record.second.unitSize;
        info.perflevel = record.first.distance;
        info.slotId = record.second.slotId;
        info.isNuma = !record.first.fdMode;
        info.state = state;
        if (info.appContext.pid == 0) {
            MLSMemKey key{info.size, info.name};
            if (info.isNuma) {
                numaBufferedMemory_.insert(std::make_pair(key, info));
                continue;
            }
            fdBufferedMemory_.insert(std::make_pair(key, info));
        } else {
            usedMemory_.insert(std::make_pair(info.name, info));
        }
    }
    isEnableLeaseBuffered_ = UbsCommonConfig::GetInstance().GetLeaseCachedSwitch();
    DBG_LOGINFO("Recovery from store finished.");
    return 0;
}

int32_t MLSManager::ReuseMemInSlotId(uint64_t size, uint16_t isNuma, const std::vector<uint32_t> &slotIds,
                                     const AppContext &context, MLSMemInfo &bufferInfo)
{
    MLSMemKey key{size};
    auto bufferMemoryMap = (isNuma == 0) ? &fdBufferedMemory_ : &numaBufferedMemory_;

    // debug 打印map里当前存放的key信息
    for (const auto &entry : *bufferMemoryMap) {
        DBG_LOGDEBUG("In buffer map, key size=" << entry.first.size << ", value size=" << entry.second.size);
    }

    auto it = bufferMemoryMap->lower_bound(key);
    if (it == bufferMemoryMap->end()) {
        DBG_LOGINFO("No buffered mem found for size=" << key.size << ", isNuma=" << isNuma);
        return -1;
    }

    bool foundMatch = false;
    while (it != bufferMemoryMap->end()) {
        bufferInfo = it->second;
        auto slotMatch = (std::find(slotIds.begin(), slotIds.end(), bufferInfo.slotId) != slotIds.end());
        if (slotMatch) {
            DBG_LOGINFO("Found buffered memory for size=" << key.size << ", name=" << bufferInfo.name);
            foundMatch = true;
            break;
        }
        ++it;
    }
    if (!foundMatch) {
        DBG_LOGINFO("No buffered mem found in slotIds, size=" << key.size << ", isNuma=" << isNuma);
        return -1;
    }
    DBG_LOGINFO("Found buffered mem found in slotIds, size=" << key.size << ", isNuma=" << isNuma);

    auto ret = MLSRepository::GetInstance().UpdateMemRecord(bufferInfo.name, context, size);
    if (ret != 0) {
        DBG_LOGERROR("Update lease record failed, name=" << bufferInfo.name << ", ret=" << ret);
        return ret;
    }
    bufferMemoryMap->erase(it);
    DBG_LOGINFO("UpdateMemRecord and remove from bufferMemoryMap, name=" << bufferInfo.name);
    return 0;
}

int32_t MLSManager::RemovePreAddRecord(ock::ubsm::MemLeaseInfo &record, bool &needToDelete)
{
    needToDelete = false;
    ubs_mem_stage stage;
    auto ret = mxm::UbseMemAdapter::GetLeaseMemoryStage(record.first.name, !record.first.fdMode, stage);
    if (ret == MXM_ERR_LEASE_NOT_EXIST) {
        MLSRepository::GetInstance().DeleteMemRecord(record.first.name);
        needToDelete = true;
        return MXM_OK;
    }
    if (ret != 0) {
        DBG_LOGERROR("GetLeaseMemoryStage failed. ret=" << ret << " name=" << record.first.name);
        return ret;
    }
    if (stage == UBSE_NOT_EXIST || stage == UBSE_DELETING) {
        MLSRepository::GetInstance().DeleteMemRecord(record.first.name);
        needToDelete = true;
        return MXM_OK;
    }

    bool isLease = true;
    bool isNuma = !record.first.fdMode;
    bool hasChangedPermission = true;
    bool ubseTimeOut = true;
    ubsm::DelayRemovedKey key{record.first.name,    ONE_MINUTE_MS, isLease,    record.first.appContext,
                              hasChangedPermission, isNuma,        ubseTimeOut};
    ubsm::UBSMemMonitor::GetInstance().AddDelayRemoveRecord(key);
    return MXM_OK;
}

int32_t MLSManager::RollbackPreDeleteRecord(const ock::ubsm::MemLeaseInfo &record, ock::ubsm::RecordState &recordState)
{
    ubs_mem_stage stage;
    recordState = record.first.state;
    auto ret = mxm::UbseMemAdapter::GetLeaseMemoryStage(record.first.name, !record.first.fdMode, stage);
    if (ret == MXM_ERR_LEASE_NOT_EXIST) {
        MLSRepository::GetInstance().UpdateMemRecordState(record.first.name, RecordState::DELETED);
        recordState = RecordState::DELETED;
        return MXM_OK;
    }
    if (ret != 0) {
        DBG_LOGERROR("GetLeaseMemoryStage failed. ret=" << ret << " name=" << record.first.name);
        return ret;
    }
    if (stage == UBSE_NOT_EXIST || stage == UBSE_DELETING || stage == UBSE_ERR_WAIT_UNEXPORT) {
        MLSRepository::GetInstance().UpdateMemRecordState(record.first.name, RecordState::DELETED);
        recordState = RecordState::DELETED;
        return MXM_OK;
    }

    MLSRepository::GetInstance().UpdateMemRecordState(record.first.name, RecordState::FINISH);
    recordState = RecordState::FINISH;
    return 0;
}

int32_t MLSManager::ReuseBufferedMem(uint64_t size, uint16_t isNuma, const std::string &regionName,
                                     const AppContext &context, MLSMemInfo &info)
{
    if (!isEnableLeaseBuffered_) {
        DBG_LOGINFO("Lease buffered disabled.");
        return -1;
    }

    std::vector<uint32_t> slotIds{};
    auto hr = ock::mxm::UbseMemAdapter::GetRegionInfo(regionName, slotIds);
    if (hr != 0) {
        DBG_LOGERROR("GetRegionInfo failed, regionName=" << regionName << ", ret=" << hr);
        return hr;
    }

    std::lock_guard<std::mutex> lock(mapMutex_);
    MLSMemInfo bufferInfo;
    auto ret = ReuseMemInSlotId(size, isNuma, slotIds, context, bufferInfo);
    if (ret != 0) {
        DBG_LOGWARN("No buffered mem found in slotIds, size=" << size << ", isNuma=" << isNuma);
        return -1;
    }

    bufferInfo.appContext = context;
    info.name = bufferInfo.name;
    info.appContext = context;
    info.size = size;
    info.numaId = bufferInfo.numaId;
    info.memIds.assign(bufferInfo.memIds.begin(), bufferInfo.memIds.end());
    info.unitSize = bufferInfo.unitSize;
    info.slotId = bufferInfo.slotId;
    info.perflevel = bufferInfo.perflevel;
    info.isNuma = isNuma;
    info.state = bufferInfo.state;

    usedMemory_.insert(std::make_pair(info.name, info));

    if (info.unitSize == 0) {
        DBG_LOGERROR("UnitSize is zero.");
        return -1;
    }

    if (info.unitSize > 0 && size > (UINT64_MAX - info.unitSize)) {
        DBG_LOGERROR("Size overflow in unitCount calculation");
        return -1;
    }

    auto unitCount = (size + info.unitSize - 1UL) / info.unitSize;
    info.memIds.resize(unitCount);

    DBG_LOGINFO("Reuse buffered mem success, name=" << info.name << ", slotId=" << info.slotId);
    return 0;
}

int32_t MLSManager::BufferUsedMemory(MLSMemInfo &usedInfo)
{
    AppContext context = usedInfo.appContext;
    context.pid = 0;
    auto newSize = usedInfo.unitSize * usedInfo.memIds.size();
    if (usedInfo.isNuma && numaBufferedMemory_.size() < maxBufferedMemNum_) {
        auto ret = MLSRepository::GetInstance().UpdateMemRecord(usedInfo.name, context, newSize);
        if (ret != 0) {
            DBG_LOGERROR("Update lease record failed, ret: " << ret);
            return ret;
        }
        MLSMemKey key{newSize, usedInfo.name};
        auto info = usedInfo;
        info.appContext = context;
        info.size = newSize;
        info.slotId = usedInfo.slotId;
        numaBufferedMemory_.insert(std::make_pair(key, info));
    } else if (!usedInfo.isNuma && fdBufferedMemory_.size() < maxBufferedMemNum_) {
        auto ret = MLSRepository::GetInstance().UpdateMemRecord(usedInfo.name, context, newSize);
        if (ret != 0) {
            DBG_LOGERROR("Update lease record failed, ret: " << ret);
            return ret;
        }
        MLSMemKey key{newSize, usedInfo.name};
        auto info = usedInfo;
        info.appContext = context;
        info.size = newSize;
        info.slotId = usedInfo.slotId;
        fdBufferedMemory_.insert(std::make_pair(key, info));
    } else {
        DBG_LOGINFO("The mem name(" << usedInfo.name << ") isNuma(" << usedInfo.numaId << "), and cache is full.");
        return 0;
    }
    return 1;
}

int32_t MLSManager::PreDeleteUsedMem(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos == usedMemory_.end()) {
        DBG_LOGERROR("Delete used mem failed, the name is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    DBG_LOGINFO("Delete used mem, name=" << name << ", isEnableLeaseBuffered_="
                                         << mxmd::ConvertBoolToString(isEnableLeaseBuffered_));
    if (isEnableLeaseBuffered_) {
        auto ret = BufferUsedMemory(pos->second);
        if (ret == 1) {
            usedMemory_.erase(pos);
            return ret;
        }
        if (ret != 0) {
            DBG_LOGERROR("Buffer used mem failed, ret: " << ret);
            return ret;
        }
    }

    auto ret = MLSRepository::GetInstance().UpdateMemRecordState(name, RecordState::PRE_DEL);
    if (ret != 0) {
        DBG_LOGERROR("Update memory record state failed, ret: " << ret);
        return ret;
    }
    DBG_LOGINFO("UpdateMemRecordState successfully, name=" << name);
    return 0;
}

int32_t MLSManager::DeleteUsedMem(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos == usedMemory_.end()) {
        DBG_LOGERROR("Delete used mem failed, the name is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    auto ret = MLSRepository::GetInstance().DeleteMemRecord(name);
    if (ret != 0) {
        DBG_LOGERROR("Delete lease record failed, ret: " << ret);
        return ret;
    }
    usedMemory_.erase(pos);
    DBG_LOGINFO("Delete used mem, name=" << name);
    return 0;
}

static int LeaseFreeMemory(const std::string &name, bool isNuma)
{
    auto ret = MLSRepository::GetInstance().UpdateMemRecordState(name, RecordState::PRE_DEL);
    if (ret != 0) {
        DBG_LOGERROR("Update memory record state to PRE_DEL failed, ret: " << ret);
        return ret;
    }
    ret = mxm::UbseMemAdapter::LeaseFree(name, isNuma);
    if (ret != 0 && ret != MXM_ERR_LEASE_NOT_EXIST) {
        DBG_LOGERROR("Get exception when LeaseFree: " << name << " ret: " << ret);
        MLSRepository::GetInstance().UpdateMemRecordState(name, RecordState::FINISH);
        return ret;
    }
    ret = MLSRepository::GetInstance().DeleteMemRecord(name);
    if (ret != 0) {
        DBG_LOGERROR("Delete lease record failed, ret: " << ret);
        return MXM_ERR_RECORD_DELETE;
    }
    return ret;
}

int32_t MLSManager::DeleteAllBufferedMem()
{
    DBG_LOGINFO("Start to deleteAllBufferedMem");
    if (!isEnableLeaseBuffered_) {
        DBG_LOGINFO("Lease buffered disabled.");
        return 0;
    }
    std::lock_guard<std::mutex> lock(mapMutex_);
    if (numaBufferedMemory_.empty() && fdBufferedMemory_.empty()) {
        DBG_LOGINFO("BufferedMemory is empty.");
        return 0;
    }

    for (auto pos = numaBufferedMemory_.begin(); pos != numaBufferedMemory_.end();) {
        auto ret = LeaseFreeMemory(pos->second.name, true);
        if (ret != 0) {
            DBG_LOGERROR("LeaseFreeMemory failed, ret: " << ret);
            ++pos;
            continue;
        }
        pos = numaBufferedMemory_.erase(pos);
    }

    for (auto pos = fdBufferedMemory_.begin(); pos != fdBufferedMemory_.end();) {
        auto ret = LeaseFreeMemory(pos->second.name, false);
        if (ret != 0) {
            DBG_LOGERROR("LeaseFreeMemory failed, ret: " << ret);
            ++pos;
            continue;
        }
        pos = fdBufferedMemory_.erase(pos);
    }

    return 0;
}

std::vector<MLSMemInfo> MLSManager::ListAllMem()
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    std::vector<MLSMemInfo> result{};
    if (usedMemory_.empty() && numaBufferedMemory_.empty() && fdBufferedMemory_.empty()) {
        return result;
    }

    for (auto &item : usedMemory_) {
        result.push_back(item.second);
    }
    if (!isEnableLeaseBuffered_) {
        return result;
    }
    for (auto &item : numaBufferedMemory_) {
        result.push_back(item.second);
    }
    for (auto &item : fdBufferedMemory_) {
        result.push_back(item.second);
    }

    return result;
}

int32_t MLSManager::GetUsedMemByName(const std::string &name, MLSMemInfo &info)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos == usedMemory_.end()) {
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    info = pos->second;
    return 0;
}

std::vector<MLSMemInfo> MLSManager::GetUsedMemByPid(uint32_t pid)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    std::vector<MLSMemInfo> result{};
    for (auto &item : usedMemory_) {
        if (item.second.appContext.pid == pid) {
            result.push_back(item.second);
        }
    }
    return result;
}
std::string MLSManager::GenerateMemName(uint64_t size)
{
    struct timespec ts {
    };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    auto timeStamp = static_cast<uint64_t>(ts.tv_sec) * NANOSECONDS + static_cast<uint64_t>(ts.tv_nsec);
    std::stringstream ss;
    ss << "N" << size << "_" << pthread_self() << "_" << timeStamp;
    return ss.str();
}

int MLSManager::PreAddUsedMem(const std::string &name, size_t size, const ubsm::AppContext &usr, bool isNuma,
                              uint16_t perfLevel)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos != usedMemory_.end()) {
        DBG_LOGERROR("Add used mem failed, the name is already exist.");
        return MXM_ERR_SHM_ALREADY_EXIST;
    }

    LeaseMallocInput input;
    input.name = name;
    input.size = size;
    input.appContext = usr;
    input.fdMode = !isNuma;
    input.distance = static_cast<uint8_t>(perfLevel);
    input.state = RecordState::PRE_ADD;
    auto ret = MLSRepository::GetInstance().PreAddMemRecord(input);
    if (ret != 0) {
        DBG_LOGERROR("Add lease record failed, ret: " << ret);
        return MXM_ERR_RECORD_ADD;
    }
    MLSMemInfo memory{};
    memory.name = name;
    memory.appContext = usr;
    memory.size = size;
    memory.perflevel = perfLevel;
    memory.state = RecordState::PRE_ADD;
    memory.isNuma = isNuma;
    usedMemory_.emplace(name, memory);
    DBG_LOGINFO("PreAddUsedMem successfully. name=" << memory.name);
    return MXM_OK;
}
int MLSManager::FinishAddUsedMem(const std::string &name, int64_t numaId, size_t unitSize, uint32_t slotId,
                                 const std::vector<uint64_t> &memIds)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos == usedMemory_.end()) {
        DBG_LOGERROR("Add used mem failed, the name is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    LeaseMallocResult result;
    result.memIds.assign(memIds.begin(), memIds.end());
    result.unitSize = unitSize;
    result.slotId = slotId;
    result.numaId = numaId;
    auto ret = MLSRepository::GetInstance().UpdateMemResultRecord(name, result);
    if (ret != 0) {
        DBG_LOGERROR("Add lease record failed, ret: " << ret);
        return MXM_ERR_RECORD_ADD;
    }
    auto &memory = pos->second;
    memory.numaId = numaId;
    memory.memIds.assign(memIds.begin(), memIds.end());
    memory.unitSize = unitSize;
    memory.slotId = slotId;
    memory.state = RecordState::FINISH;
    return MXM_OK;
}

int32_t MLSManager::UpdateMemRecordState(const std::string &name, ock::ubsm::RecordState state)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto pos = usedMemory_.find(name);
    if (pos == usedMemory_.end()) {
        DBG_LOGERROR("Update used mem record state failed, the name is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    auto ret = MLSRepository::GetInstance().UpdateMemRecordState(name, state);
    if (ret != 0) {
        DBG_LOGERROR("Update mem record state failed, ret: " << ret);
        return MXM_ERR_RECORD_CHANGE_STATE;
    }
    pos->second.state = state;
    DBG_LOGINFO("UpdateMemRecordState successfully. name=" << name << ", state=" << static_cast<int>(state));
    return MXM_OK;
}

} // namespace ock::lease::service