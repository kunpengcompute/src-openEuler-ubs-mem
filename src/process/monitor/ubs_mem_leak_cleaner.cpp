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
#include "ubs_mem_leak_cleaner.h"
#include <sys/stat.h>
#include <sstream>
#include "ipc_handler.h"
#include "log.h"
#include "mls_manager.h"
#include "record_store.h"
#include "shm_manager.h"
#include "ubs_mem_monitor.h"
#include "ubse_mem_adapter.h"

namespace ock {
namespace ubsm {
using namespace ock::common;
using namespace ock::mxmd;
using namespace ock::lease::service;
using namespace ock::share::service;

bool ProcessAlive(const uint32_t pid)
{
    std::ostringstream oss;
    oss << "/proc/" << pid; // 该目录默认权限为 555
    std::string dir = oss.str();
    struct stat info {
    };
    if (stat(dir.c_str(), &info) != 0) {
        return false;
    }
    return S_ISDIR(info.st_mode);
}

UBSMemLeakCleaner &UBSMemLeakCleaner::GetInstance() noexcept
{
    static UBSMemLeakCleaner instance;
    return instance;
}

int UBSMemLeakCleaner::Start() noexcept
{
    auto ret = CleanLeaseMemoryLeakWhenStart();
    if (ret != UBSM_OK) {
        DBG_LOGERROR("StartCleanLeaseMemoryLeak fail. ret: " << ret);
        return ret;
    }
    ret = CleanShareMemoryLeakWhenStart();
    if (ret != UBSM_OK) {
        DBG_LOGERROR("StartCleanShareMemoryLeak fail. ret: " << ret);
        return ret;
    }
    return UBSM_OK;
}

int UBSMemLeakCleaner::CleanLeaseMemoryLeakWhenStart()
{
    auto lsMap = MLSManager::GetInstance().ListAllMem();
    for (const auto &ls : lsMap) {
        if (ls.appContext.pid == 0 || ProcessAlive(ls.appContext.pid)) {
            continue;
        }
        DBG_LOGINFO("Pid=" << ls.appContext.pid << " not alive. Start to clean leak lease memory.");
        DelayRemovedKey removedKey{ls.name, 0, true, ls.appContext, false, ls.isNuma, false};
        if (!UBSMemMonitor::GetInstance().GetDelayRemoveRecord(removedKey)) {
            DBG_LOGERROR("Add key to delayKeys, name=" << ls.name << " pid=" << ls.appContext.pid);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
        }
    }
    return UBSM_OK;
}

int UBSMemLeakCleaner::SHMProcessDeadProcess(pid_t pid)
{
    auto memoryArr = SHMManager::GetInstance().GetMemoryUsersByPid(pid);
    for (auto memory : memoryArr) {
        if (memory.pids.empty() || memory.pids.find(pid) == memory.pids.end()) {
            DBG_LOGERROR("User not in memory-users map. memory: " << memory.name << " userNum: " << memory.pids.size());
            continue;
        }

        if (memory.pids.size() > 1) {
            auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo(memory.name, pid);
            if (ret != 0) {
                DBG_LOGERROR("RemoveMemoryUserInfo fail, errCode=" << ret << " name=" << memory.name << " pid=" << pid);
            }
            continue;
        }
        if (memory.pids.size() == 1) {
            AppContext ctx{static_cast<uint32_t>(pid), memory.result.ownerUserId, memory.result.ownerGroupId};
            DelayRemovedKey removedKey{memory.name, 0, false, ctx, false, false, false};
            if (!UBSMemMonitor::GetInstance().GetDelayRemoveRecord(removedKey)) {
                DBG_LOGINFO("Clean removedKey, name=" << memory.name << " pid=" << ctx.pid
                                                      << ", expires time=" << removedKey.expiresTime);
                UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            }
        }
    }
    DBG_LOGINFO("SHMProcessDeadProcess end, pid: " << pid);
    return UBSM_OK;
}

int UBSMemLeakCleaner::CleanShareMemoryLeakWhenStart()
{
    auto users = SHMManager::GetInstance().GetAllUsers();
    std::vector<pid_t> deadProcess{};
    for (auto usr : users) {
        if (ProcessAlive(usr)) {
            continue;
        }
        deadProcess.push_back(usr);
    }
    if (deadProcess.empty()) {
        DBG_LOGDEBUG("StartCleanShareMemoryLeak successfully, no dead process need to clean.");
        return UBSM_OK;
    }

    for (const auto &pidIter : deadProcess) {
        auto ret = SHMProcessDeadProcess(pidIter);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("SHMProcessDeadProcess error. ret: " << ret);
            return ret;
        }
    }

    DBG_LOGINFO("StartCleanShareMemoryLeak successfully.");
    return UBSM_OK;
}

int UBSMemLeakCleaner::CleanLeaseMemoryLeakInner(const std::string &name, const AppContext &ctx, bool &changed,
                                                 bool isTimeOutScene, bool isNumaLease)
{
    DBG_LOGINFO("Start to clean leak lease memory. name=" << name << " pid=" << ctx.pid);
    if (isTimeOutScene) {
        ubs_mem_stage status = UBSE_END;
        bool isAttach = false;
        auto ret = mxm::UbseMemAdapter::GetTimeOutTaskStatus(name, true, isNumaLease, status, isAttach);
        if (ret == MXM_ERR_LEASE_NOT_EXIST || status == UBSE_NOT_EXIST) {
            auto hr = MLSManager::GetInstance().DeleteUsedMem(name); // ubse接口超时场景，需要删除PRE_ADD的record
            if (hr != 0) {
                DBG_LOGWARN("DeleteUsedMem failed. name=" << name << " ret=" << hr);
            }
            return MXM_OK;
        }
        if (ret != MXM_OK || (status == UBSE_CREATING || status == UBSE_DELETING)) {
            // 返错，外层会重新加入
            return MXM_TIME_OUT_TASK_NEED_RETRY;
        }
        // 调归还， 并删除
        auto hr = mxm::UbseMemAdapter::LeaseFree(name, isNumaLease);
        if (hr != 0 && hr != MXM_ERR_LEASE_NOT_EXIST) {
            DBG_LOGERROR("Get exception when LeaseFree. name: " << name << " ret: " << hr);
            return hr;
        }

        hr = MLSManager::GetInstance().DeleteUsedMem(name); // ubse接口超时场景，需要删除PRE_ADD的record
        if (hr != 0) {
            DBG_LOGWARN("DeleteUsedMem failed. name=" << name << " ret=" << hr);
        }
        return hr;
    }

    MLSMemInfo memory;
    auto ret = MLSManager::GetInstance().GetUsedMemByName(name, memory);
    if (ret != 0) {
        DBG_LOGERROR("Get lease record failed, " << name << " is not exist.");
        return MXM_ERR_PARAM_INVALID;
    }

    if (memory.state == RecordState::DELETED) {
        MLSManager::GetInstance().DeleteUsedMem(name);
        DBG_LOGINFO("Delete record successfully. name=" << name << " pid=" << ctx.pid);
        return MXM_OK;
    }

    if (!memory.isNuma && !changed) {
        AppContext appContext;
        appContext.uid = ctx.uid;
        appContext.gid = ctx.gid;
        appContext.pid = ctx.pid;
        mode_t mode = S_IRUSR | S_IWUSR;
        auto hr = ock::mxm::UbseMemAdapter::FdPermissionChange(name, appContext, mode);
        if (hr != 0) {
            DBG_LOGERROR("FdPermissionChange failed, name= " << name << ", appContext= " << appContext.GetString()
                                                             << "ret=" << hr);
            return hr;
        }
        changed = true;
    }

    ret = MLSManager::GetInstance().PreDeleteUsedMem(name);
    if (ret == 0) { // 0 means deleted, 1 means cached
        DBG_LOGINFO("App free start, name is " << name);
        auto hr = mxm::UbseMemAdapter::LeaseFree(name, memory.isNuma);
        if (hr != 0 && hr != MXM_ERR_LEASE_NOT_EXIST) {
            DBG_LOGERROR("Get exception when LeaseFree. name: " << name << " ret: " << hr);
            return hr;
        }
        hr = MLSManager::GetInstance().DeleteUsedMem(name);
        if (hr != 0) {
            DBG_LOGERROR("Delete lease record failed, " << name << " is not exist.");
            return MXM_ERR_RECORD_DELETE;
        }
    }

    if (ret != 1 && ret != 0) { // 0 means deleted, 1 means cached
        DBG_LOGERROR("DeleteUsedMem fail. name: " << name << " ret: " << ret);
        return ret;
    }

    DBG_LOGINFO("CleanLeaseMemoryLeak successfully. name: " << name);
    return 0;
}

int UBSMemLeakCleaner::TryRollBackTimeoutShmTask(const std::string &name, const AppContext &ctx, bool isAttach,
                                                 uint64_t createSeqNo)
{
    DBG_LOGINFO("Try to rollback timeout shm task.");

    ubs_mem_stage status = UBSE_END;
    uint64_t storedSeqNo = UINT32_MAX;
    auto ret = mxm::UbseMemAdapter::GetTimeOutShmTaskStatus(name, status, isAttach, storedSeqNo);
    DBG_LOGINFO("GetTimeOutTaskStatus end. name=" << name << " pid=" << ctx.pid << ", ret=" << ret
                                                  << ",status=" << static_cast<int>(status));
    if (ret == MXM_ERR_SHM_NOT_EXIST || status == UBSE_NOT_EXIST) { // attach -超时 -返回
        if (isAttach) {
            SHMManager::GetInstance().RemoveMemoryInfo(name);
        }
        return MXM_OK;
    }
    if (ret == MXM_ERR_SHM_IN_USING) {
        return MXM_OK;
    }
    if (ret != MXM_OK || (status == UBSE_CREATING || status == UBSE_DELETING)) {
        // 返错，外层会重新加入
        return MXM_TIME_OUT_TASK_NEED_RETRY;
    }
    // 调归还， 并删除
    DBG_LOGINFO("Start to rollback, name=" << name << " isAttach=" << isAttach);
    if (isAttach) {
        auto hr = mxm::UbseMemAdapter::ShmDetach(name);
        if (hr != 0) {
            DBG_LOGERROR("get exception when ShmDetach " << name << "ret: " << hr);
        }
        SHMManager::GetInstance().RemoveMemoryInfo(name);
        return hr;
    }
    if (createSeqNo != storedSeqNo) {
        DBG_LOGINFO("The createSeqNo is different. No need to delete. createSeqNo=" << createSeqNo
                                                                                    << " storedSeqNo=" << storedSeqNo);
        return MXM_OK;
    }
    auto hr = mxm::UbseMemAdapter::ShmDelete(name, ctx);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmDelete " << name << "ret: " << hr);
    }
    return hr;
}

int UBSMemLeakCleaner::CleanShareMemoryLeakInner(const std::string &name, const AppContext &ctx, bool isTimeOutScene,
                                                 bool isAttach, uint64_t createSeqNo)
{
    DBG_LOGINFO("Start to clean leak share memory. name=" << name << " pid=" << ctx.pid);
    if (name.empty()) {
        DBG_LOGERROR("Name is empty. name=" << name);
        return MXM_ERR_PARAM_INVALID;
    }

    if (isTimeOutScene) {
        return TryRollBackTimeoutShmTask(name, ctx, isAttach, createSeqNo);
    }
    auto ret = SHMManager::GetInstance().RemoveMemoryUserInfo(name, ctx.pid);
    if (ret != HOK && ret != MXM_ERR_SHM_NOT_FOUND) {
        DBG_LOGERROR("RemoveMemoryUserInfo fail. name=" << name << " pid=" << ctx.pid);
        return ret;
    }
    ret = SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::PRE_DEL);
    if (ret != HOK && ret != MXM_ERR_SHM_NOT_FOUND) {
        DBG_LOGERROR("UpdateShareMemoryRecordState fail. name=" << name << " pid=" << ctx.pid);
        return ret;
    }
    auto hr = mxm::UbseMemAdapter::ShmDetach(name);
    if (hr != 0 && hr != MXM_ERR_UBSE_NOT_ATTACH) {
        SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::FINISH);
        SHMManager::GetInstance().AddMemoryUserInfo(name, ctx.pid);
        DBG_LOGERROR("get exception when ShmUnimport " << name << "ret: " << hr);
        return hr;
    }

    ret = SHMManager::GetInstance().RemoveMemoryInfo(name);
    if (ret != HOK && hr != MXM_ERR_UBSE_NOT_ATTACH) {
        DBG_LOGERROR("RemoveMemoryInfo fail. name=" << name << " pid=" << ctx.pid);
        SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::DELETED);
        SHMManager::GetInstance().AddMemoryUserInfo(name, ctx.pid);
        return ret;
    }
    return UBSM_OK;
}
} // namespace ubsm
} // namespace ock