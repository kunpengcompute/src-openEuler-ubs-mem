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
#include "ipc_handler.h"
#include <securec.h>
#include <sys/types.h>
#include <unistd.h>
#include "mls_manager.h"
#include "rack_mem_functions.h"
#include "record_store_def.h"
#include "ubs_mem_monitor.h"
#include "ubse_mem_adapter.h"
#include "ubsm_ptracer.h"

namespace ock::lease::service {
using namespace ock::mxmd;
using namespace ock::com;
using namespace ock::ubsm;

static int RollBackReuseCacheMemory(const std::string &name, bool isNuma)
{
    auto hr = MLSManager::GetInstance().PreDeleteUsedMem(name);
    if (hr == 0) {
        // 0 means not cached
        auto ret = mxm::UbseMemAdapter::LeaseFree(name, isNuma);
        if (ret != 0 && ret != MXM_ERR_LEASE_NOT_EXIST) {
            DBG_LOGERROR("Failed to call ubse interface to free lease memory, name=" << name << " ret="
                                                                                     << ConvertErrorToString(ret));
            MLSManager::GetInstance().UpdateMemRecordState(name, RecordState::FINISH);
            return ret;
        }
        ret = MLSManager::GetInstance().DeleteUsedMem(name);
        if (ret != 0) {
            DBG_LOGERROR("Failed to delete lease memory record. name=" << name);
            return ret;
        }
    }
    return MXM_OK;
}

static int ReuseCachedLeaseMemory(size_t size, const std::string &regionName, bool isNuma, const AppContext &user,
                                  AppMallocMemoryResponse &response)
{
    MLSMemInfo memory;
    auto ret = MLSManager::GetInstance().ReuseBufferedMem(size, isNuma, regionName, user, memory);
    if (ret != 0) {
        DBG_LOGWARN("Failed to reuse lease memory. ret=" << ConvertErrorToString(ret));
        return ret;
    }
    DBG_LOGINFO("Reuse from buffered memory success, name=" << memory.name);
    response.memIds_.assign(memory.memIds.begin(), memory.memIds.end());
    response.numaId_ = memory.numaId;
    response.isNuma_ = memory.isNuma;
    response.unitSize_ = memory.unitSize;
    response.name_ = memory.name;
    response.errCode_ = 0;

    if (!isNuma) {
        TP_TRACE_BEGIN(TP_UBSM_SET_PERMISSION);
        mode_t mode = S_IRUSR | S_IWUSR;
        auto hr = ock::mxm::UbseMemAdapter::FdPermissionChange(memory.name, user, mode);
        TP_TRACE_END(TP_UBSM_SET_PERMISSION, hr);
        if (hr != 0) {
            DBG_LOGERROR("FdPermissionChange failed, name= " << memory.name << ", ret=" << ConvertErrorToString(hr));
            response.errCode_ = hr;
            DBG_LOGINFO("Try to free memory, name=" << response.name_);
            (void)RollBackReuseCacheMemory(memory.name, response.isNuma_);
            return hr;
        }
    }

    return MXM_OK;
}

static void RollBackBorrowNewMemory(const std::string &name, bool isNuma)
{
    auto ret = MLSManager::GetInstance().UpdateMemRecordState(name, RecordState::PRE_DEL);
    if (ret != 0) {
        DBG_LOGERROR("UpdateMemRecordState failed. name=" << name);
        return;
    }
    ret = mxm::UbseMemAdapter::LeaseFree(name, isNuma);
    if (ret != 0 && ret != MXM_ERR_LEASE_NOT_EXIST) {
        DBG_LOGERROR("LeaseFree failed. name=" << name);
        return;
    }
    ret = MLSManager::GetInstance().DeleteUsedMem(name);
    if (ret != 0) {
        DBG_LOGERROR("LeaseFree failed. name=" << name);
        return;
    }
}

static int BorrowNewMemory(const AppMallocMemoryRequest &request, AppMallocMemoryResponse &response,
                           const MxmComUdsInfo &udsInfo)
{
    ock::mxm::LeaseMallocParam param;
    param.name = MLSManager::GetInstance().GenerateMemName(request.size_);
    param.regionName = request.regionName_;
    param.size = request.size_;
    param.isNuma = request.isNuma_;
    param.distance = static_cast<ock::mxm::MemNodeDistance>(request.perflevel_);
    param.appContext = {udsInfo.pid, udsInfo.uid, udsInfo.gid};
    ock::ubsm::LeaseMallocResult result{};
    auto hr = MLSManager::GetInstance().PreAddUsedMem(param.name, param.size, param.appContext, param.isNuma,
                                                      request.perflevel_);
    if (hr != 0) {
        DBG_LOGERROR("PreAddUsedMem failed, res is " << ConvertErrorToString(hr));
        response.errCode_ = hr;
        return hr;
    }
    TP_TRACE_BEGIN(TP_UBSM_LEASE_MALLOC);
    hr = ock::mxm::UbseMemAdapter::LeaseMalloc(param, result);
    TP_TRACE_END(TP_UBSM_LEASE_MALLOC, hr);

    if (hr != 0) {
        DBG_LOGERROR("Get exception when LeaseMalloc " << param.name);
        MLSManager::GetInstance().DeleteUsedMem(param.name);
        response.errCode_ = hr;
        return hr;
    }
    // ubs_mem_numa_create没有返回memId，需将unitSize置为requestSize用于计算缓存。
    size_t unitSize = request.isNuma_ ? request.size_ : result.unitSize;
    TP_TRACE_BEGIN(TP_UBSM_ADD_USED_MEM);
    hr = MLSManager::GetInstance().FinishAddUsedMem(param.name, result.numaId, unitSize, result.slotId, result.memIds);
    TP_TRACE_END(TP_UBSM_ADD_USED_MEM, hr);
    if (hr != 0) {
        DBG_LOGERROR("Add used mem failed, res is " << ConvertErrorToString(hr));
        response.errCode_ = hr;
        DBG_LOGINFO("Try to free memory, name=" << param.name);
        RollBackBorrowNewMemory(param.name, param.isNuma);
        return hr;
    }

    response.memIds_.assign(result.memIds.begin(), result.memIds.end());
    response.numaId_ = result.numaId;
    response.isNuma_ = result.numaId != -1;
    response.unitSize_ = result.numaId != -1 ? request.size_ : result.unitSize;
    response.name_ = param.name;
    response.errCode_ = 0;
    return MXM_OK;
}

static int BorrowMemory(const AppMallocMemoryRequest &request, AppMallocMemoryResponse &response,
                        const MxmComUdsInfo &udsInfo)
{
    AppContext user = {udsInfo.pid, udsInfo.uid, udsInfo.gid};
    auto ret = ReuseCachedLeaseMemory(request.size_, request.regionName_, request.isNuma_, user, response);
    if (ret == MXM_OK) {
        DBG_LOGINFO("Reuse cached lease memory successfully. name=" << response.name_ << " size=" << request.size_);
        return ret;
    }
    ret = BorrowNewMemory(request, response, udsInfo);
    if (ret != MXM_OK) {
        DBG_LOGERROR("BorrowNewMemory failed, res is " << ConvertErrorToString(ret));
        return ret;
    }
    return MXM_OK;
}

int MxmServerMsgHandle::AppMallocMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    TP_TRACE_BEGIN(TP_UBEM_IPC_HANDLER_MALLOC);
    auto request = dynamic_cast<const AppMallocMemoryRequest *>(req);
    auto response = dynamic_cast<AppMallocMemoryResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of AppMallocMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                       << ", pid=" << udsInfo.pid);
    DBG_LOGINFO("App Malloc Memory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                          << ", size=" << request->size_ << ", isNuma=" << request->isNuma_);

    auto ret = BorrowMemory(*request, *response, udsInfo);
    if (ret != MXM_OK) {
        DBG_LOGERROR("BorrowMemory failed, ret=" << ret);
        DBG_AUDITINFO("AppMallocMemory failed, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << response->name_
                                                     << ", ret=" << ConvertErrorToString(ret));
        return ret;
    }
    DBG_LOGINFO("App malloc success, name=" << response->name_ << ", uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                            << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("AppMallocMemory successfully, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                       << ", pid=" << udsInfo.pid << ", name=" << response->name_
                                                       << ", ret=" << ConvertErrorToString(response->errCode_));
    TP_TRACE_END(TP_UBEM_IPC_HANDLER_MALLOC, 0);
    return 0;
}

static int BorrowMemoryWithLender(const AppMallocMemoryWithLocRequest &request, AppMallocMemoryResponse &response,
                                  const MxmComUdsInfo &udsInfo)
{
    ock::mxm::LeaseMallocWithLocParam param;
    param.name = MLSManager::GetInstance().GenerateMemName(request.size_);
    param.regionName = "default";
    param.size = request.size_;
    param.isNuma = request.isNuma_;
    param.appContext = {udsInfo.pid, udsInfo.uid, udsInfo.gid};
    param.slotId = request.slotId_;
    param.socketId = request.socketId_;
    param.numaId = request.numaId_;
    param.portId = request.portId_;
    ock::ubsm::LeaseMallocResult result{};

    auto ret = MLSManager::GetInstance().PreAddUsedMem(param.name, param.size, param.appContext, param.isNuma,
                                                       MEM_DISTANCE_L0);
    if (ret != MXM_OK) {
        DBG_LOGERROR("PreAddUsedMem failed, ret=" << ret << " name=" << param.name);
        response.errCode_ = ret;
        return ret;
    }
    TP_TRACE_BEGIN(TP_UBSM_LEASE_MALLOC_WITH_LOC);
    ret = ock::mxm::UbseMemAdapter::LeaseMallocWithLoc(param, result);
    TP_TRACE_END(TP_UBSM_LEASE_MALLOC_WITH_LOC, ret);
    DBG_LOGINFO("LeaseMallocWithLoc name=" << param.name << ", numa=" << result.numaId << ", size=" << result.unitSize
                                           << ", ret=" << ret);
    if (ret != 0) {
        DBG_LOGERROR("Get exception when LeaseMallocWithLoc, name=" << param.name
                                                                    << ", ret=" << ConvertErrorToString(ret));
        MLSManager::GetInstance().DeleteUsedMem(param.name);
        response.errCode_ = ret;
        return ret;
    }
    ret = MLSManager::GetInstance().FinishAddUsedMem(param.name, result.numaId, result.unitSize, result.slotId,
                                                     result.memIds);
    if (ret != 0) {
        DBG_LOGERROR("Add used mem failed, ret=" << ConvertErrorToString(ret) << " name=" << param.name);
        response.errCode_ = ret;
        RollBackBorrowNewMemory(param.name, param.isNuma);
        return ret;
    }
    response.memIds_.assign(result.memIds.begin(), result.memIds.end());
    response.numaId_ = result.numaId;
    response.isNuma_ = result.numaId != -1;
    response.unitSize_ = result.numaId != -1 ? request.size_ : result.unitSize;
    response.name_ = param.name;
    response.errCode_ = 0;
    return MXM_OK;
}

int MxmServerMsgHandle::AppMallocMemoryWithLoc(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const AppMallocMemoryWithLocRequest *>(req);
    auto response = dynamic_cast<AppMallocMemoryResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    MLSMemInfo memory;
    AppContext appContext(udsInfo.pid, udsInfo.uid, udsInfo.gid);
    DBG_AUDITINFO("user info of AppMallocMemoryWithLoc, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                              << ", pid=" << udsInfo.pid);
    DBG_LOGINFO("App Malloc Memory with loc, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                   << ", size=" << request->size_ << ", isNuma=" << request->isNuma_);

    auto ret = BorrowMemoryWithLender(*request, *response, udsInfo);
    if (ret != MXM_OK) {
        DBG_LOGERROR("BorrowMemoryWithLender failed, ret=" << ret);
        DBG_AUDITINFO("user info of AppMallocMemoryWithLoc, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                                  << udsInfo.pid << ", name=" << response->name_
                                                                  << ", ret=" << ConvertErrorToString(ret));
        return ret;
    }

    DBG_LOGINFO("App malloc with loc success, name=" << response->name_ << ", uid=" << udsInfo.uid
                                                     << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("user info of AppMallocMemoryWithLoc, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                              << ", pid=" << udsInfo.pid << ", name=" << response->name_
                                                              << ", ret=" << ConvertErrorToString(response->errCode_));
    return 0;
}

int MxmServerMsgHandle::AppFreeMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const AppFreeMemoryRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    if (request->name_.empty()) {
        DBG_LOGERROR("AppFreeMemory request Name is empty.");
        response->errCode_ = MXM_ERR_PARAM_INVALID;
        return MXM_ERR_PARAM_INVALID;
    }
    DBG_LOGINFO("App Free Memory name=" << request->name_);
    DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid);

    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }

    MLSMemInfo memory;
    auto ret = MLSManager::GetInstance().GetUsedMemByName(request->name_, memory);
    if (ret != 0) {
        DBG_LOGERROR("Get lease record failed, " << request->name_ << " is not exist.");
        DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                         << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                         << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    if (memory.state == RecordState::DELETED) {
        response->errCode_ = MXM_OK;
        MLSManager::GetInstance().DeleteUsedMem(request->name_);
        DBG_LOGINFO("Free memory successfully. name=" << request->name_);
        DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                         << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                         << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = MXM_OK;
        return MXM_OK;
    }
    bool isNuma = memory.isNuma;
    DBG_LOGINFO("AppFreeMemory name=" << request->name_ << ", numaId= " << memory.numaId << ", isNuma=" << isNuma);
    if (!isNuma) {
        AppContext appContext;
        appContext.uid = getuid();
        appContext.gid = getgid();
        appContext.pid = getpid();
        mode_t mode = S_IRUSR | S_IWUSR;
        TP_TRACE_BEGIN(TP_UBSM_SET_PERMISSION);
        auto hr = ock::mxm::UbseMemAdapter::FdPermissionChange(request->name_, appContext, mode);
        TP_TRACE_END(TP_UBSM_SET_PERMISSION, hr);
        if (hr != 0) {
            DBG_LOGERROR("FdPermissionChange failed, name= " << request->name_ << ", ret=" << hr);
            DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                             << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                             << ", ret=" << ConvertErrorToString(hr));
            response->errCode_ = hr;
            return hr;
        }
    }

    TP_TRACE_BEGIN(TP_UBSM_DELETE_REUSE_MEM);
    ret = MLSManager::GetInstance().PreDeleteUsedMem(request->name_);
    TP_TRACE_END(TP_UBSM_DELETE_REUSE_MEM, ret);
    if (ret == 0) {
        DBG_LOGINFO("App free start, name is " << request->name_);
        AppContext appContext;
        appContext.uid = udsInfo.uid;
        appContext.gid = udsInfo.gid;
        appContext.pid = udsInfo.pid;
        TP_TRACE_BEGIN(TP_UBSM_LEASE_FREE);
        auto hr = mxm::UbseMemAdapter::LeaseFree(request->name_, isNuma);
        TP_TRACE_END(TP_UBSM_LEASE_FREE, ret);
        if (hr != 0 && hr != MXM_ERR_LEASE_NOT_EXIST) {
            DBG_LOGERROR("Get exception when LeaseFree, name=" << request->name_ << ", ret=" << hr);
            DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                             << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                             << ", ret=" << ConvertErrorToString(hr));
            MLSManager::GetInstance().UpdateMemRecordState(memory.name, ubsm::RecordState::FINISH);
            response->errCode_ = hr;
            return hr;
        }
        hr = MLSManager::GetInstance().DeleteUsedMem(request->name_);
        if (hr != 0) {
            DBG_LOGERROR("Delete used memory record. ret=" << ConvertErrorToString(hr));
            response->errCode_ = hr;
            return hr;
        }
    }
    response->errCode_ = 0;
    DBG_LOGINFO("App free success, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                         << ", name=" << request->name_);
    DBG_AUDITINFO("user info of AppFreeMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(0));
    return 0;
}

int MxmServerMsgHandle::AppQueryClusterInfo(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    DBG_LOGINFO("AppQueryClusterInfo start");
    DBG_AUDITINFO("user info of AppQueryClusterInfo, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                           << ", pid=" << udsInfo.pid);
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("param is invalid.");
        return MXM_ERR_NULLPTR;
    }
    auto response = dynamic_cast<AppQueryClusterInfoResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto hr = ock::mxm::UbseMemAdapter::LookUpClusterStatistic(response->info_);
    if (hr != 0) {
        DBG_LOGERROR("LookUpClusterStatistic failed, ret:" << hr);
        DBG_AUDITINFO("user info of AppQueryClusterInfo, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                               << udsInfo.pid << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }
    response->errCode_ = 0;
    DBG_LOGINFO("AppQueryClusterInfo success");
    DBG_AUDITINFO("user info of AppQueryClusterInfo, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                           << udsInfo.pid << ", ret=" << ConvertErrorToString(hr));
    return 0;
}

int MxmServerMsgHandle::AppForceFreeCachedMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("AppForceFreeCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("user info of AppForceFreeCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                                << ", pid=" << udsInfo.pid);
    auto ret = MLSManager::GetInstance().DeleteAllBufferedMem();
    if (ret != 0) {
        DBG_LOGERROR("DeleteAllBufferedMem failed, res is " << ConvertErrorToString(ret));
    }
    DBG_LOGINFO("AppForceFreeCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", ret=" << ret);
    DBG_AUDITINFO("user info of AppForceFreeCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                                << ", pid=" << udsInfo.pid
                                                                << ", ret=" << ConvertErrorToString(ret));
    response->errCode_ = ret;
    return ret;
}

int MxmServerMsgHandle::AppQueryCachedMemory(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<AppQueryCachedMemoryResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("AppQueryCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("user info of AppQueryCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                            << ", pid=" << udsInfo.pid);
    auto memoryVec = MLSManager::GetInstance().ListAllMem();
    if (memoryVec.empty()) {
        DBG_LOGINFO("No buffered cache found.");
        DBG_AUDITINFO("AppQueryCachedMemory No buffered cache found.");
        response->errCode_ = 0;
        return 0;
    }

    for (auto &memory : memoryVec) {
        std::string name = memory.name;
        std::string group = memory.appContext.GetString();
        std::string unitSize = std::to_string(memory.unitSize);
        std::string numaId = std::to_string(memory.numaId);
        std::string memIds = "";
        for (auto &memId : memory.memIds) {
            memIds += std::to_string(memId) + ",";
        }
        std::string record = name + "|" + group + "|" + unitSize + "|" + numaId + "|" + memIds;
        response->records_.push_back(record);
    }
    DBG_LOGINFO("AppQueryCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                             << ", records size=" << response->records_.size());
    DBG_AUDITINFO("user info of AppQueryCachedMemory, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                            << ", pid=" << udsInfo.pid
                                                            << ", records size=" << response->records_.size());
    response->errCode_ = 0;
    return 0;
}

static int32_t CleanNotUsedRecordLeaseMemory(const MLSMemInfo &memory)
{
    std::shared_ptr<AppFreeMemoryRequest> request;
    std::shared_ptr<CommonResponse> response;
    try {
        request = std::make_shared<AppFreeMemoryRequest>(memory.memIds, "default", memory.name);
        response = std::make_shared<CommonResponse>();
    } catch (std::exception &e) {
        DBG_LOGERROR("CleanNotUsedRecordLeaseMemory: Exception caught." << e.what());
        return MXM_ERR_NULLPTR;
    } catch (...) {
        DBG_LOGERROR("CleanNotUsedRecordLeaseMemory: Unknown exception caught.");
        return MXM_ERR_UNKNOWN;
    }

    MxmComUdsInfo udsInfo{.pid = memory.appContext.pid, .uid = memory.appContext.uid, .gid = memory.appContext.gid};
    return MxmServerMsgHandle::AppFreeMemory(request.get(), response.get(), udsInfo);
}

int MxmServerMsgHandle::AppCheckMemoryLease(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CheckMemoryLeaseRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("request or response is nullptr.");
        return MXM_ERR_NULLPTR;
    }

    DBG_LOGINFO("AppCheckMemoryLease start. size=" << request->names_.size());
    auto records = MLSManager::GetInstance().GetUsedMemByPid(udsInfo.pid);
    std::unordered_set<std::string> names(request->names_.begin(), request->names_.end());
    for (const auto &record : records) {
        if (names.find(record.name) == names.end()) { // ubsmd记录了该应用未使用的内存借用记录，需要释放
            DBG_LOGINFO("record lease name " << record.name << " not found in user map.");
            auto ret = CleanNotUsedRecordLeaseMemory(record);
            if (ret != MXM_OK) {
                DBG_LOGERROR("CleanNotUsedRecordLeaseMemory failed. name=" << record.name << ", ret=" << ret);
                continue;
            }
            DBG_LOGINFO("CleanNotUsedRecordLeaseMemory successfully. name=" << record.name);
        }
    }
    response->errCode_ = MXM_OK;
    return MXM_OK;
}
} // namespace ock::lease::service