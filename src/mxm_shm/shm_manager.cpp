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

#include "shm_manager.h"

#include "ubs_mem_def.h"
#include "mls_repository.h"
#include "rack_mem_err.h"
#include "record_store.h"
#include "ubs_mem_monitor.h"
#include "ubse_mem_adapter.h"
#include "util/functions.h"

namespace ock::share::service {
using namespace ock::common;

int32_t SHMManager::Initial()
{
    return UBSM_OK;
}

int32_t SHMManager::Recovery()
{
    auto ret = GetInstance().RecoverFromRecord();
    if (ret != UBSM_OK) {
        DBG_LOGERROR("RecoverFromRecord failed ret is:" << ret);
        return ret;
    }
    ret = GetInstance().RecoverFromRefRecord();
    if (ret != UBSM_OK) {
        DBG_LOGERROR("RecoverFromRefRecord failed ret is:" << ret);
        return ret;
    }
    return UBSM_OK;
}

int32_t SHMManager::RecoverFromRefRecord()
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto refRecords = ubsm::RecordStore::GetInstance().ListShmRefRecordNameF();
    for (const auto &ref : refRecords) {
        auto namePos = attachedShareMemory_.find(ref.first);
        if (namePos == attachedShareMemory_.end()) {
            DBG_LOGERROR("RecoverFromRecord fail. name not exist: " << ref.first);
            return MXM_ERR_SHM_NOT_FOUND;
        }
        for (const auto &user : ref.second) {
            namePos->second.pids.insert(user.first);
            DBG_LOGINFO("[SHM RECOVER] name: " << ref.first << " pid: " << user.first
                                               << " refCount: " << namePos->second.pids.size());
        }
    }
    /*
     * 当 RemoveMemoryUserInfo 和 RemoveMemoryInfo 之间，进程故障，会出现没有用户的record，需要删除。
     */
    auto backup = attachedShareMemory_;
    for (const auto &[name, memory] : backup) {
        if (memory.state == ubsm::RecordState::PRE_ADD) {
            // PRE_ADD的record会在后台回收，其pids一定为空。这里需要跳过
            continue;
        }
        if (memory.pids.empty()) {
            DBG_LOGINFO("It needs to be deleted because it has no user. name=" << name);
            auto ret = ubsm::RecordStore::GetInstance().DelShmImportRecord(name);
            if (ret != 0) {
                DBG_LOGERROR("DelShmImportRecord fail. name=" << name);
                continue;
            }
            attachedShareMemory_.erase(name);
            DBG_LOGINFO("The record without users is deleted successfully. name=" << name);
        }
    }
    return UBSM_OK;
}

int32_t SHMManager::RecoverFromRecord()
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto attachRecords = ubsm::RecordStore::GetInstance().ListShmImportRecord();
    for (const auto &record : attachRecords) {
        if (attachedShareMemory_.find(record.name) != attachedShareMemory_.end()) {
            DBG_LOGERROR("RecoverFromRecord fail. name exist: " << record.name);
            return MXM_ERR_SHM_ALREADY_EXIST;
        }
        if (record.state == ubsm::RecordState::PRE_ADD) {
            bool needToDelete = false;
            RemovePreAddRecord(record, needToDelete);
            if (needToDelete) {
                DBG_LOGINFO("Remove pre add record successfully. name=" << record.name);
                continue;
            }
            DBG_LOGINFO("Memory will be returned in the background. name=" << record.name);
        }
        auto state = record.state;
        if (record.state == ubsm::RecordState::PRE_DEL) {
            auto ret = RollbackPreDeleteRecord(record, state);
            if (ret != 0) {
                DBG_LOGERROR("Rollback pre delete record failed. name=" << record.name << "ret=" << ret);
                continue;
            }
            DBG_LOGINFO("Rollback pre delete record successfully. name=" << record.name);
        }
        DBG_LOGINFO("[SHM RECOVER] name: " << record.name);
        AttachedShareMemory memory;
        memory.name = record.name;
        memory.size = record.size;
        memory.state = state;
        memory.result.unitSize = record.unitSize;
        memory.result.ownerGroupId = record.appContext.gid;
        memory.result.ownerUserId = record.appContext.uid;
        memory.result.memIds.assign(record.memIds.begin(), record.memIds.end());
        memory.result.createFlags = record.flag;
        memory.result.openFlag = record.oflag;
        attachedShareMemory_.emplace(record.name, memory);
    }
    return UBSM_OK;
}

int32_t SHMManager::AddMemoryUserInfo(const std::string &name, pid_t userProcessId)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    if (attachedShareMemory_.find(name) == attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " not found on attached memory.");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    auto ret = ubsm::RecordStore::GetInstance().AddShmRefRecord(userProcessId, name);
    if (ret != 0) {
        DBG_LOGERROR("AddShmRefRecord fail. name: " << name << " pid: " << userProcessId);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    attachedShareMemory_[name].pids.insert(userProcessId);
    DBG_LOGINFO("AddMemoryUserInfo Successfully. name: " << name
                                                         << " refCount: " << attachedShareMemory_[name].pids.size());
    return UBSM_OK;
}

int32_t SHMManager::RemoveMemoryUserInfo(const std::string &name, pid_t userProcessId)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter == attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " not found on attached memory.");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (memIter->second.pids.find(userProcessId) == memIter->second.pids.end()) {
        DBG_LOGERROR("pid: " << userProcessId << " not found on memory: " << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    auto ret = ubsm::RecordStore::GetInstance().DelShmRefRecord(userProcessId, name);
    if (ret != 0) {
        DBG_LOGERROR("DelShmRefRecord fail. name: " << name << " pid: " << userProcessId);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    memIter->second.pids.erase(userProcessId);
    DBG_LOGINFO("RemoveMemoryUserInfo Successfully. name: " << name
                                                            << " refCount: " << attachedShareMemory_[name].pids.size());
    return UBSM_OK;
}

int32_t SHMManager::PrepareAddShareMemoryInfo(const std::string &name, size_t size, pid_t processId)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter != attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " already exists.");
        return MXM_ERR_SHM_ALREADY_EXIST;
    }
    ock::ubsm::ShareMemImportInput input;
    input.name = name;
    input.size = size;
    input.pid = processId;

    auto ret = ubsm::RecordStore::GetInstance().AddShmImportInput(input);
    if (ret != 0) {
        DBG_LOGERROR("AddShmImportInput failed. ret=" << ret);
        return MXM_ERR_RECORD_ADD;
    }

    AttachedShareMemory attachedShareMemory{};
    attachedShareMemory.name = name;
    attachedShareMemory.size = size;
    attachedShareMemory.state = ubsm::RecordState::PRE_ADD;
    attachedShareMemory_.emplace(name, std::move(attachedShareMemory));
    return MXM_OK;
}

int32_t SHMManager::AddFullShareMemoryInfo(const std::string &name, pid_t pid,
                                           const ock::ubsm::ShareMemImportResult &result)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter == attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " not found.");
        return MXM_ERR_SHM_NOT_FOUND;
    }

    auto ret = ubsm::RecordStore::GetInstance().AddShmImportResult(name, result);
    if (ret != 0) {
        DBG_LOGERROR("AddShmImportInput failed. ret=" << ret);
        return MXM_ERR_RECORD_ADD;
    }
    ret = ubsm::RecordStore::GetInstance().AddShmRefRecord(pid, name);
    if (ret != 0) {
        DBG_LOGERROR("AddShmRefRecord fail. name: " << name << " pid: " << pid);
        ubsm::RecordStore::GetInstance().DelShmImportRecord(name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    memIter->second.state = ubsm::RecordState::FINISH;
    memIter->second.result = result;
    memIter->second.pids.insert(pid);
    return MXM_OK;
}

int32_t SHMManager::UpdateShareMemoryRecordState(const std::string &name, ubsm::RecordState state)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter == attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " not found.");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    auto ret = ock::ubsm::RecordStore::GetInstance().UpdateShmImportRecordState(name, state);
    if (ret != 0) {
        DBG_LOGERROR("UpdateShmImportRecordState failed. ret=" << ret);
        return MXM_ERR_RECORD_CHANGE_STATE;
    }
    memIter->second.state = state;
    return MXM_OK;
}

int32_t SHMManager::RemoveMemoryInfo(const std::string &name, bool force)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter == attachedShareMemory_.end()) {
        DBG_LOGERROR("memory: " << name << " not found on attached memory.");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (force && memIter->second.state == ubsm::RecordState::DELETED) {
        DBG_LOGINFO("Force to remove share memory record which is deleted. name=" << name);
        for (auto pid : memIter->second.pids) {
            auto ret = ubsm::RecordStore::GetInstance().DelShmRefRecord(pid, name);
            if (ret != 0) {
                DBG_LOGERROR("DelShmRefRecord fail. name: " << name);
                return MXM_ERR_SHM_NOT_FOUND;
            }
        }
        memIter->second.pids.clear();
    }
    if (!memIter->second.pids.empty()) {
        DBG_LOGERROR("memory: " << name << " is in use by " << memIter->second.pids.size() << " usr(s)");
        return MXM_ERR_SHM_IN_USING;
    }
    auto ret = ubsm::RecordStore::GetInstance().DelShmImportRecord(name);
    if (ret != 0) {
        DBG_LOGERROR("DelShmImportRecord fail. name: " << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    attachedShareMemory_.erase(memIter);
    DBG_LOGINFO("RemoveMemoryInfo Successfully. name: " << name);
    return UBSM_OK;
}

int32_t SHMManager::GetShareMemoryInfo(const std::string &name, AttachedShareMemory &memory)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    auto memIter = attachedShareMemory_.find(name);
    if (memIter == attachedShareMemory_.end()) {
        DBG_LOGWARN("memory: " << name << " not found on attached memory.");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    memory = memIter->second;
    return UBSM_OK;
}

std::vector<AttachedShareMemory> SHMManager::GetMemoryUsersByPid(uint32_t pid)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    std::vector<AttachedShareMemory> result;
    for (const auto &iter : attachedShareMemory_) {
        if (iter.second.pids.find(pid) != iter.second.pids.end()) {
            auto tmp = iter.second;
            result.push_back(tmp);
        }
    }
    return result;
}
int32_t SHMManager::GetMemoryUsersCountByName(const std::string &name, size_t &count)
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    if (attachedShareMemory_.find(name) == attachedShareMemory_.end()) {
        DBG_LOGINFO("memory: " << name << " not found on attached memory.");
        count = 0;
        return UBSM_OK;
    }
    count = attachedShareMemory_[name].pids.size();
    return UBSM_OK;
}

std::unordered_set<pid_t> SHMManager::GetAllUsers()
{
    std::unique_lock<std::mutex> lock(mapMutex_);
    std::unordered_set<pid_t> allPids;
    for (const auto &iter : attachedShareMemory_) {
        for (const auto &usr : iter.second.pids) {
            allPids.insert(static_cast<pid_t>(usr));
        }
    }
    return allPids;
}

int SHMManager::RemovePreAddRecord(const ubsm::ShareMemImportInfo &record, bool &needToDelete)
{
    needToDelete = false;
    ubs_mem_stage stage;
    auto ret = mxm::UbseMemAdapter::GetShareMemoryStage(record.name, stage);
    if (ret == MXM_ERR_SHM_NOT_EXIST) {
        needToDelete = true;
        ubsm::RecordStore::GetInstance().DelShmImportRecord(record.name);
        return MXM_OK;
    }
    if (ret != 0) {
        DBG_LOGERROR("GetShareMemoryStage failed. ret=" << ret << " name=" << record.name);
        return ret;
    }
    if (stage == UBSE_NOT_EXIST || stage == UBSE_DELETING) {
        needToDelete = true;
        ubsm::RecordStore::GetInstance().DelShmImportRecord(record.name);
        return MXM_OK;
    }

    bool timeOut = true;
    bool isLease = false;
    bool isNuma = false;
    bool changed = false;
    ubsm::DelayRemovedKey key{record.name, ubsm::ONE_MINUTE_MS, isLease, record.appContext, changed, isNuma, timeOut};
    key.isAttach = true;
    ubsm::UBSMemMonitor::GetInstance().AddDelayRemoveRecord(key);
    return MXM_OK;
}

int SHMManager::RollbackPreDeleteRecord(const ubsm::ShareMemImportInfo &record, ubsm::RecordState &state)
{
    state = record.state;
    ubs_mem_stage stage;
    auto ret = mxm::UbseMemAdapter::GetShareMemoryStage(record.name, stage);
    if (ret == MXM_ERR_SHM_NOT_EXIST) {
        ubsm::RecordStore::GetInstance().UpdateShmImportRecordState(record.name, ubsm::RecordState::DELETED);
        state = ubsm::RecordState::DELETED;
        return MXM_OK;
    }
    if (ret != 0) {
        DBG_LOGERROR("GetShareMemoryStage failed. ret=" << ret << " name=" << record.name);
        return ret;
    }
    if (stage == UBSE_NOT_EXIST || stage == UBSE_DELETING || stage == UBSE_ERR_WAIT_UNEXPORT) {
        ubsm::RecordStore::GetInstance().UpdateShmImportRecordState(record.name, ubsm::RecordState::DELETED);
        state = ubsm::RecordState::DELETED;
        return MXM_OK;
    }

    ubsm::RecordStore::GetInstance().UpdateShmImportRecordState(record.name, ubsm::RecordState::FINISH);
    state = ubsm::RecordState::FINISH;

    return 0;
}
} // namespace ock::share::service