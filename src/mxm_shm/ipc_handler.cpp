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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include "dlock_utils/ubsm_lock.h"
#include "record_store.h"
#include "region_manager.h"
#include "region_repository.h"
#include "rpc_config.h"
#include "rpc_server.h"
#include "shm_manager.h"
#include "ubs_mem_monitor.h"
#include "ubse_mem_adapter.h"
#include "ubsm_ptracer.h"
#include "zen_discovery.h"

namespace ock::share::service {
using namespace ock::mxmd;
using namespace ock::com;

std::array<std::mutex, MUTEX_HASH_SIZE> MxmServerMsgHandle::mutexArray;

int MxmServerMsgHandle::ShmLookRegionList(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmLookRegionListRequest *>(req);
    auto response = dynamic_cast<ShmLookRegionListResponse *>(rsp);

    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of ShmLookRegionList, region type=" << request->regionType_ << ", uid=" << udsInfo.uid
                                                                 << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_LOGINFO("Looking up regions list, region type=" << request->regionType_ << ", uid=" << udsInfo.uid
                                                        << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    SHMRegions regions;
    int hr{0};
    hr = mxm::UbseMemAdapter::LookupRegionList(regions);
    if (hr != 0) {
        DBG_LOGERROR("Get exception when calling LookupRegionList, hr=" << hr);
        DBG_AUDITINFO("user info of ShmLookRegionList, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                             << udsInfo.pid << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }

    response->errCode_ = 0;
    response->regions_ = regions;
    DBG_LOGINFO("Look region list info message from agent successfully, number=" << regions.num);
    for (int i = 0; i < regions.num; ++i) {
        DBG_LOGINFO("Serial number=" << i << ", number=" << regions.region[i].num);
        for (int j = 0; j < regions.num; ++j) {
            DBG_LOGINFO("Node id=" << regions.region[i].nodeId[j] << ", host name=" << regions.region[i].hostName[j]
                                   << ", affinity=" << regions.region[i].affinity[j]);
        }
    }
    DBG_AUDITINFO("user info of ShmLookRegionList, region type=" << request->regionType_ << ", uid=" << udsInfo.uid
                                                                 << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    return 0;
}

int MxmServerMsgHandle::ShmCreate(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    TP_TRACE_BEGIN(TP_UBEM_IPC_HANDLER_CREATE_SHMEM);
    auto request = dynamic_cast<const ShmCreateRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);
    DBG_LOGINFO("Create shared memory, region_name=" << request->regionName_ << ", shared_name=" << request->name_
                                                     << ", size=" << request->size_ << ", mode=" << request->mode_
                                                     << ", flags=" << request->flags_);

    if (request->regionDesc_.num > MEM_TOPOLOGY_MAX_HOSTS) {
        DBG_LOGERROR("Region num=" << request->regionDesc_.num << " exceeds maximum=" << MEM_TOPOLOGY_MAX_HOSTS);
        response->errCode_ = MXM_ERR_PARAM_INVALID;
        return MXM_ERR_PARAM_INVALID;
    }

    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }

    mxm::CreateShmParam createParam;
    int count = 0;
    if (request->regionName_ == "default") {
        SHMRegionDesc regionInfo = request->regionDesc_;
        for (int i = 0; i < regionInfo.num; i++) {
            DBG_LOGINFO("Create region affinity nodeId=" << regionInfo.nodeId[i]);
            uint32_t nodeId{1u};
            if (!StrUtil::StrToUint(std::string(regionInfo.nodeId[i]), nodeId)) {
                DBG_LOGERROR("Invalid node ID, nodeId[" << i << "]=" << regionInfo.nodeId[i]);
                DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                             << ", pid=" << udsInfo.pid << ", ret="
                                                             << ConvertErrorToString(MXM_ERR_PARAM_INVALID));
                response->errCode_ = MXM_ERR_PARAM_INVALID;
                return MXM_ERR_PARAM_INVALID;
            }
            createParam.privider.slot_ids[count] = nodeId;
            count++;
        }
    } else {
        RegionInfo regionInfo{};
        TP_TRACE_BEGIN(TP_UBEM_GET_REGION);
        auto ret = ock::share::service::RegionManager::GetInstance().GetRegionInfo(request->regionName_, regionInfo);
        TP_TRACE_END(TP_UBEM_GET_REGION, ret);
        if (!ret) {
            DBG_LOGERROR("Get exception when calling GetRegionInfo, region_name=" << request->regionName_);
            DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                         << ", pid=" << udsInfo.pid
                                                         << ", ret=" << ConvertErrorToString(MXM_ERR_REGION_NOT_EXIST));
            response->errCode_ = MXM_ERR_REGION_NOT_EXIST;
            return MXM_ERR_REGION_NOT_EXIST;
        }
        auto reqRegion = request->regionDesc_;
        for (int i = 0; i < regionInfo.region.num; i++) {
            for (int j = 0; j < reqRegion.num; ++j) {
                if (std::strcmp(regionInfo.region.hostName[i], reqRegion.hostName[j]) == 0 &&
                    regionInfo.region.affinity[i]) {
                    DBG_LOGINFO("Create region affinity nodeId=" << regionInfo.region.nodeId[i]);
                    uint32_t nodeId{1u};
                    if (!StrUtil::StrToUint(std::string(regionInfo.region.nodeId[i]), nodeId)) {
                        DBG_LOGERROR("Invalid node ID, nodeId[" << i << "]=" << regionInfo.region.nodeId[i]);
                        response->errCode_ = MXM_ERR_PARAM_INVALID;
                        return MXM_ERR_PARAM_INVALID;
                    }
                    DBG_LOGINFO("Integer nodeId=" << nodeId);
                    createParam.privider.slot_ids[count] = nodeId;
                    count++;
                }
            }
        }
    }
    createParam.privider.node_cnt = count;
    createParam.name = request->name_;
    createParam.size = request->size_;
    createParam.appContext.uid = udsInfo.uid;
    createParam.appContext.gid = udsInfo.gid;
    createParam.appContext.pid = udsInfo.pid;
    createParam.baseNid = request->baseNid_;
    createParam.desc = request->regionDesc_;
    createParam.flag = request->flags_;
    createParam.mode = request->mode_;

    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE);
    auto hr = mxm::UbseMemAdapter::ShmCreate(createParam);
    TP_TRACE_END(TP_UBSM_SHM_CREATE, hr);
    if (hr != 0) {
        TP_TRACE_END(TP_UBEM_IPC_HANDLER_CREATE_SHMEM, hr);
        DBG_LOGERROR("Get exception when IpcCallShmCreate, name=" << request->name_);
        DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }

    response->errCode_ = 0;
    DBG_LOGINFO("Creating share ubsmd receive message from agent successfully, name=" << request->name_);
    DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    TP_TRACE_END(TP_UBEM_IPC_HANDLER_CREATE_SHMEM, 0);
    return 0;
}

int MxmServerMsgHandle::ShmCreateWithProvider(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    TP_TRACE_BEGIN(TP_UBEM_IPC_HANDLER_CREATE_WITH_PROVIDER_SHMEM);
    auto request = dynamic_cast<const ShmCreateWithProviderRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of ShmCreate, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);
    DBG_LOGINFO("Create shared memory with provider, shared name=" << request->name_ << ", size=" << request->size_
                                                                   << ", mode=" << request->mode_
                                                                   << ", flags=" << request->flags_);

    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }
    mxm::CreateShmWithProviderParam createParam(request->hostName_, request->socketId_, request->numaId_,
                                                request->portId_);
    createParam.name = request->name_;
    createParam.size = request->size_;
    createParam.appContext.uid = udsInfo.uid;
    createParam.appContext.gid = udsInfo.gid;
    createParam.appContext.pid = udsInfo.pid;
    createParam.flag = request->flags_;
    createParam.mode = request->mode_;

    TP_TRACE_BEGIN(TP_UBSM_SHM_CREATE_WITH_PROVIDER);
    auto hr = mxm::UbseMemAdapter::ShmCreateWithProvider(createParam);
    TP_TRACE_END(TP_UBSM_SHM_CREATE_WITH_PROVIDER, hr);
    if (hr != 0) {
        TP_TRACE_END(TP_UBEM_IPC_HANDLER_CREATE_WITH_PROVIDER_SHMEM, hr);
        DBG_LOGERROR("Get exception when IpcCallShmCreate, name=" << request->name_);
        DBG_AUDITINFO("user info of ShmCreateWithProvider, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                                 << udsInfo.pid << ", name=" << request->name_
                                                                 << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }

    response->errCode_ = 0;
    DBG_LOGINFO("Creating share ubsmd receive message from agent successfully, name=" << request->name_);
    DBG_AUDITINFO("user info of ShmCreateWithProvider, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                             << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                             << ", success.");
    TP_TRACE_END(TP_UBEM_IPC_HANDLER_CREATE_WITH_PROVIDER_SHMEM, 0);
    return 0;
}

static int ShmPermissionCheck(const std::string &name, const MxmComUdsInfo &udsInfo) noexcept
{
    mxm::ubse_user_info_t ubsUserInfo{};
    auto ret = mxm::UbseMemAdapter::ShmGetUserData(name, ubsUserInfo);
    if (ret != 0) {
        DBG_LOGERROR("ShmGetUserData fail. ret: " << ret << " name: " << name);
        return ret;
    }

    if ((udsInfo.uid != ubsUserInfo.udsInfo.uid) || (udsInfo.gid != ubsUserInfo.udsInfo.gid)) {
        DBG_LOGERROR("ShmPermissionCheck, uid=" << udsInfo.uid << " udsInfo.gid: " << udsInfo.gid
                                                << " owner uid: " << ubsUserInfo.udsInfo.uid
                                                << " owner gid: " << ubsUserInfo.udsInfo.gid);
        return MXM_ERR_SHM_PERMISSION_DENIED;
    }
    DBG_LOGINFO("ShmPermissionCheck success, user.uid=" << udsInfo.uid << " user.gid=" << udsInfo.gid
                                                        << " owner.uid=" << ubsUserInfo.udsInfo.uid
                                                        << " owner.gid=" << ubsUserInfo.udsInfo.gid);
    return UBSM_OK;
}

int MxmServerMsgHandle::ShmDelete(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmDeleteRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);
    DBG_LOGINFO("ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                  << ", name=" << request->name_);

    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }

    int res = ShmPermissionCheck(request->name_, udsInfo);
    if (res != 0) {
        DBG_LOGERROR("Failed to check shm Permission, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                            << ", ret=" << res);
        DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", ret=" << ConvertErrorToString(res));
        response->errCode_ = res;
        return res;
    }

    AppContext appContext;
    appContext.uid = udsInfo.uid;
    appContext.gid = udsInfo.gid;
    appContext.pid = udsInfo.pid;
    size_t usrNum = 0;
    auto ret = SHMManager::GetInstance().GetMemoryUsersCountByName(request->name_, usrNum);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("GetMemoryUsersCountByName fail; " << request->name_ << "ret: " << ret);
        DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    if (usrNum > 0) {
        DBG_LOGERROR("memory: " << request->name_ << " is in use by " << usrNum << " usr(s)");
        DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(MXM_ERR_SHM_IN_USING));
        response->errCode_ = MXM_ERR_SHM_IN_USING;
        return MXM_ERR_SHM_IN_USING;
    }
    auto hr = mxm::UbseMemAdapter::ShmDelete(request->name_, appContext);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmDelete " << request->name_ << "ret: " << hr);
        DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }

    response->errCode_ = 0;
    DBG_LOGINFO("Delete map message from agent successfully " << request->name_);
    DBG_AUDITINFO("user info of ShmDelete, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    return 0;
}

int GetMaxOflag(int provided_perms)
{
    if ((provided_perms & PROT_READ) && (provided_perms & PROT_WRITE)) {
        return O_RDWR;
    } else if (provided_perms & PROT_READ) {
        return O_RDONLY;
    } else if (provided_perms & PROT_EXEC) {
        return O_RDWR;
    }

    return -1;
}

int CalculateProvidedPermissions(uid_t caller_uid, gid_t caller_gid, uid_t owner_uid, gid_t owner_gid, mode_t mode)
{
    bool is_owner = (caller_uid == owner_uid);
    bool in_group = (caller_gid == owner_gid);

    DBG_LOGINFO("Permission check - caller[uid=" << caller_uid << ", gid=" << caller_gid << "], owner[uid=" << owner_uid
                                                 << ", gid=" << owner_gid << "], is_owner=" << is_owner << ", in_group="
                                                 << in_group << ", mode=0" << std::oct << mode << std::dec);

    int provided_perms = PROT_NONE;

    if (is_owner) {
        if (mode & S_IRUSR) {
            provided_perms |= PROT_READ;
        }
        if (mode & S_IWUSR) {
            provided_perms |= PROT_WRITE;
        }
        if (mode & S_IXUSR) {
            provided_perms |= PROT_EXEC;
        }
        DBG_LOGINFO("Owner permissions: " << provided_perms);
    } else if (in_group) {
        if (mode & S_IRGRP) {
            provided_perms |= PROT_READ;
        }
        if (mode & S_IWGRP) {
            provided_perms |= PROT_WRITE;
        }
        if (mode & S_IXGRP) {
            provided_perms |= PROT_EXEC;
        }
        DBG_LOGINFO("Group permissions: " << provided_perms);
    } else {
        if (mode & S_IROTH) {
            provided_perms |= PROT_READ;
        }
        if (mode & S_IWOTH) {
            provided_perms |= PROT_WRITE;
        }
        if (mode & S_IXOTH) {
            provided_perms |= PROT_EXEC;
        }
        DBG_LOGINFO("Other permissions: " << provided_perms);
    }

    return provided_perms;
}

int CheckPermission(uid_t caller_uid, gid_t caller_gid, mxm::ubse_user_info_t ubsUserInfo, int port, int &oflag)
{
    uid_t owner_uid = ubsUserInfo.udsInfo.uid;
    gid_t owner_gid = ubsUserInfo.udsInfo.gid;
    mode_t mode = ubsUserInfo.mode;

    int provided_perms = CalculateProvidedPermissions(caller_uid, caller_gid, owner_uid, owner_gid, mode);
    bool has_sufficient_perms = true;
    oflag = GetMaxOflag(provided_perms);
    if (oflag == -1) {
        DBG_LOGERROR("Required permission not provided, permissions: " << provided_perms);
        has_sufficient_perms = false;
    }

    DBG_LOGINFO("GetMaxOflag success, oflag " << oflag);

    if ((port & PROT_READ) && !(provided_perms & PROT_READ)) {
        DBG_LOGERROR("Required READ permission not provided");
        has_sufficient_perms = false;
    }
    if ((port & PROT_WRITE) && !(provided_perms & PROT_WRITE)) {
        DBG_LOGERROR("Required WRITE permission not provided");
        has_sufficient_perms = false;
    }
    if ((port & PROT_EXEC) && !(provided_perms & PROT_EXEC)) {
        DBG_LOGERROR("Required EXEC permission not provided");
        has_sufficient_perms = false;
    }

    if (has_sufficient_perms) {
        DBG_LOGINFO("Permission granted for port " << port);
        return 0;
    }

    DBG_LOGERROR("Permission denied. Provided: " << provided_perms << ", Required: " << port);
    return MXM_ERR_SHM_PERMISSION_DENIED;
}

static void RollBackAttachShareMemory(const std::string &name)
{
    auto ret = SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::PRE_DEL);
    if (ret != 0) {
        DBG_LOGERROR("UpdateShareMemoryRecordState failed. name=" << name);
        return;
    }
    ret = mxm::UbseMemAdapter::ShmDetach(name);
    if (ret != 0 && ret != MXM_ERR_UBSE_NOT_ATTACH) {
        DBG_LOGERROR("ShmDetach failed. name=" << name);
        SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::FINISH);
        return;
    }
    ret = SHMManager::GetInstance().RemoveMemoryInfo(name);
    if (ret != 0) {
        DBG_LOGERROR("RemoveMemoryInfo failed. name=" << name);
        return;
    }
}

static int ShmMmapInner(const mxm::ImportShmParam &importParam, mxm::AttachShmResult &result)
{
    mxm::ImportShmParam importOwnerParam = importParam;
    mxm::ubse_user_info_t ubsUserInfo{};
    int rt = mxm::UbseMemAdapter::ShmGetUserData(importParam.name, ubsUserInfo);
    if (rt != 0) {
        DBG_LOGERROR("ShmGetUserData fail. ret: " << rt << " name: " << importParam.name);
        return rt;
    }
    DBG_LOGINFO("importParam prot is " << importParam.prot);

    int oflag{};
    int32_t res =
        CheckPermission(importParam.appContext.uid, importParam.appContext.gid, ubsUserInfo, importParam.prot, oflag);
    result.oflag = oflag;
    if (res != UBSM_OK) {
        DBG_LOGERROR("CheckPermission error. ret: " << res << " name: " << importParam.name);
        return res;
    }

    importOwnerParam.name = importParam.name;
    importOwnerParam.length = importParam.length;
    importOwnerParam.appContext.uid = ubsUserInfo.udsInfo.uid;
    importOwnerParam.appContext.gid = ubsUserInfo.udsInfo.gid;
    importOwnerParam.appContext.pid = importParam.appContext.pid;
    importOwnerParam.mode = ubsUserInfo.mode;
    ubsUserInfo.udsInfo.pid = importParam.appContext.pid;
    auto hr = SHMManager::GetInstance().PrepareAddShareMemoryInfo(importParam.name, importParam.length,
                                                                  importParam.appContext.pid);
    if (hr != 0) {
        DBG_LOGERROR("Failed to add share memory record. name=" << importParam.name << " ret=" << hr);
        return hr;
    }
    hr = mxm::UbseMemAdapter::ShmAttach(importParam.name, ubsUserInfo, result);
    if (hr != 0) {
        DBG_LOGERROR("Get exception when ShmImport " << importParam.name << "ret: " << hr);
        SHMManager::GetInstance().RemoveMemoryInfo(importParam.name);
        return hr;
    }
    ShareMemImportResult attachResult;
    attachResult.memIds.assign(result.memIds.begin(), result.memIds.end());
    attachResult.ownerUserId = importOwnerParam.appContext.uid;
    attachResult.ownerGroupId = importOwnerParam.appContext.gid;
    attachResult.unitSize = result.unitSize;
    attachResult.createFlags = result.flag;
    attachResult.openFlag = result.oflag;
    hr = SHMManager::GetInstance().AddFullShareMemoryInfo(importParam.name, importParam.appContext.pid, attachResult);
    if (hr != 0) {
        DBG_LOGERROR("Failed to add full share memory record. name=" << importParam.name << " ret=" << hr);
        RollBackAttachShareMemory(importParam.name);
        return hr;
    }
    return UBSM_OK;
}

int HandleExistingShm(const ShmMapRequest *request, ShmMapResponse *response, const MxmComUdsInfo &udsInfo,
                      const AttachedShareMemory &memory)
{
    if (memory.pids.find(udsInfo.pid) != memory.pids.end()) {
        DBG_LOGERROR("Share memory is already in use by process: " << udsInfo.pid);
        DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                  << ", name=" << request->name_
                                                  << ", ret=" << ConvertErrorToString(MXM_ERR_SHM_ALREADY_EXIST));
        response->errCode_ = MXM_ERR_SHM_ALREADY_EXIST;
        return MXM_ERR_SHM_ALREADY_EXIST;
    }
    auto ret = SHMManager::GetInstance().AddMemoryUserInfo(request->name_, udsInfo.pid);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("AddMemoryUserInfo error. ret: " << ConvertErrorToString(ret) << " name: " << request->name_);
        DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                  << ", name=" << request->name_
                                                  << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->memIds_.assign(memory.result.memIds.begin(), memory.result.memIds.end());
    response->shmSize_ = memory.size;
    response->unitSize_ = memory.result.unitSize;
    response->flag_ = memory.result.createFlags;
    response->oflag_ = memory.result.openFlag;
    DBG_LOGINFO("Shm name=" << request->name_ << " exists, flag=" << response->flag_ << ", oflag=" << response->oflag_);
    DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                              << ", name=" << request->name_ << ", success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::ShmMap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    TP_TRACE_BEGIN(TP_UBEM_IPC_HANDLER_MAP);
    auto request = dynamic_cast<const ShmMapRequest *>(req);
    auto response = dynamic_cast<ShmMapResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                              << ", name=" << request->name_);
    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }

    AttachedShareMemory memory;
    response->errCode_ = UBSM_OK;
    std::unique_lock<std::mutex> uniqueLock{GetMutexByName(request->name_)};
    TP_TRACE_BEGIN(TP_UBSM_GET_SHARED_MEMORY_INFO);
    auto ret = SHMManager::GetInstance().GetShareMemoryInfo(request->name_, memory);
    TP_TRACE_END(TP_UBSM_GET_SHARED_MEMORY_INFO, 0);
    if (ret == UBSM_OK && memory.state == RecordState::FINISH) {
        TP_TRACE_BEGIN(TP_UBSM_REUSE_SHARED_MEMORY);
        ret = HandleExistingShm(request, response, udsInfo, memory);
        TP_TRACE_END(TP_UBSM_REUSE_SHARED_MEMORY, ret);
        TP_TRACE_END(TP_UBEM_IPC_HANDLER_MAP, ret);
        return ret;
    }
    if (memory.state == RecordState::DELETED) {
        // 删除后重新调用attach接口映射
        ret = SHMManager::GetInstance().RemoveMemoryInfo(memory.name, true);
        if (ret != 0) {
            DBG_LOGERROR("RemoveMemoryInfo error. ret=" << ConvertErrorToString(ret));
            return ret;
        }
    }
    mxm::ImportShmParam importParam;
    mxm::AttachShmResult result;
    importParam.name = request->name_;
    importParam.length = request->size_;
    importParam.appContext.uid = udsInfo.uid;
    importParam.appContext.gid = udsInfo.gid;
    importParam.appContext.pid = udsInfo.pid;
    importParam.mode = request->mode_;
    importParam.prot = request->prot_;
    ret = ShmMmapInner(importParam, result);
    if (ret != UBSM_OK) {
        TP_TRACE_END(TP_UBEM_IPC_HANDLER_MAP, ret);
        DBG_LOGERROR("ShmMmapInner error. ret=" << ConvertErrorToString(ret) << " name=" << importParam.name << ", uid="
                                                << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
        DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                  << ", name=" << request->name_
                                                  << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->memIds_.assign(result.memIds.begin(), result.memIds.end());
    response->shmSize_ = result.shmSize;
    response->unitSize_ = result.unitSize;
    response->flag_ = result.flag;
    response->oflag_ = result.oflag;

    DBG_LOGINFO("Shm map message success, name=" << request->name_ << ", oflag_=" << response->oflag_
                                                 << ", flag=" << response->flag_ << ", uid=" << udsInfo.uid
                                                 << ", gid=" << udsInfo.gid);
    DBG_AUDITINFO("user info of ShmMap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                              << ", name=" << request->name_ << ", success.");
    TP_TRACE_END(TP_UBEM_IPC_HANDLER_MAP, 0);
    return 0;
}

static int ShmUnMapInner(const std::string &name, const MxmComUdsInfo &usrInfo)
{
    auto ret = SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::PRE_DEL);
    if (ret != MXM_OK) {
        DBG_LOGERROR("PrepareRemoveShareMemoryInfo fail. name=" << name);
        return ret;
    }

    ret = mxm::UbseMemAdapter::ShmDetach(name);
    if (ret != MXM_OK && ret != MXM_ERR_UBSE_NOT_ATTACH) {
        DBG_LOGERROR("get exception when ShmUnimport " << name << "ret=" << ret);
        SHMManager::GetInstance().UpdateShareMemoryRecordState(name, RecordState::FINISH);
        return ret;
    }
    ret = SHMManager::GetInstance().RemoveMemoryUserInfo(name, usrInfo.pid);
    if (ret != MXM_OK) {
        DBG_LOGERROR("RemoveMemoryUserInfo fail. name=" << name << " pid=" << usrInfo.pid);
        return ret;
    }
    ret = SHMManager::GetInstance().RemoveMemoryInfo(name);
    if (ret != MXM_OK) {
        SHMManager::GetInstance().AddMemoryUserInfo(name, usrInfo.pid);
        DBG_LOGERROR("RemoveMemoryInfo fail. name=" << name << " pid=" << usrInfo.pid);
        return ret;
    }
    return MXM_OK;
}

int MxmServerMsgHandle::ShmUnmap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmUnmapRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_LOGINFO("Shm unmap name=" << request->name_ << ", uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                  << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                << ", name=" << request->name_);
    AttachedShareMemory memory;
    std::unique_lock<std::mutex> uniqueLock{GetMutexByName(request->name_)};
    auto ret = SHMManager::GetInstance().GetShareMemoryInfo(request->name_, memory);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("GetShareMemoryInfo fail. name=" << request->name_);
        DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << ", name=" << request->name_
                                                    << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    if (memory.state == RecordState::DELETED) {
        SHMManager::GetInstance().RemoveMemoryInfo(request->name_, true);
        if (ret != 0) {
            DBG_LOGERROR("RemoveMemoryInfo fail. name=" << request->name_);
            return ret;
        }
        response->errCode_ = MXM_OK;
        return MXM_OK;
    }
    size_t userNum = 0;
    ret = SHMManager::GetInstance().GetMemoryUsersCountByName(request->name_, userNum);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("GetMemoryUsersCountByName fail. name=" << request->name_);
        DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << ", name=" << request->name_
                                                    << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    if (userNum > 1) {
        DBG_LOGINFO("Start to decrease shm ref count. name=" << request->name_ << " pid=" << udsInfo.pid);
        ret = SHMManager::GetInstance().RemoveMemoryUserInfo(request->name_, udsInfo.pid);
        if (ret != UBSM_OK) {
            DBG_LOGERROR("RemoveMemoryUserInfo fail. name=" << request->name_ << " pid=" << udsInfo.pid);
            DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                        << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                        << ", ret=" << ConvertErrorToString(ret));
            response->errCode_ = ret;
            return ret;
        }
        response->errCode_ = 0;
        return 0;
    }
    ret = ShmUnMapInner(request->name_, udsInfo);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("ShmUnMapInner fail. name=" << request->name_ << " pid=" << udsInfo.pid);
        DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << ", name=" << request->name_
                                                    << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->errCode_ = 0;
    DBG_LOGINFO("Shm unmap message from agent successfully " << request->name_);
    DBG_AUDITINFO("user info of ShmUnmap, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                << ", name=" << request->name_ << ", success.");
    return 0;
}

int MxmServerMsgHandle::RegionLookupRegionList(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    SHMRegions regions{};
    auto request = dynamic_cast<const ShmLookRegionListRequest *>(req);
    auto response = dynamic_cast<ShmLookRegionListResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_LOGINFO("Look region list, region type=", request->regionType_);
    DBG_AUDITINFO("user info of RegionLookupRegionList, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                              << ", pid=" << udsInfo.pid);
    int hr{0};
    hr = mxm::UbseMemAdapter::LookupRegionList(regions);
    if (hr != 0) {
        DBG_LOGERROR("Get exception when IpcCallShmLookRegionList, region type=" << request->regionType_);
        DBG_AUDITINFO("user info of RegionLookupRegionList, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                                  << ", pid=" << udsInfo.pid
                                                                  << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }

    response->errCode_ = MXM_OK;
    response->regions_ = regions;
    DBG_LOGINFO("Look region list info message from agent successfully.");
    DBG_AUDITINFO("user info of RegionLookupRegionList, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                              << ", pid=" << udsInfo.pid << ", success.");
    return MXM_OK;
}

int MxmServerMsgHandle::RegionCreateRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmCreateRegionRequest *>(req);
    auto response = dynamic_cast<ShmCreateRegionResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of RegionCreateRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                          << udsInfo.pid << ", region_name=" << request->regionName_);
    DBG_LOGINFO("Start to create region, name=" << request->regionName_);
    if (request->region_.num > MEM_TOPOLOGY_MAX_HOSTS) {
        DBG_LOGERROR("Region num=" << request->region_.num << " exceeds maximum=" << MEM_TOPOLOGY_MAX_HOSTS);
        response->errCode_ = MXM_ERR_PARAM_INVALID;
        return MXM_ERR_PARAM_INVALID;
    }
    for (int i = 0; i < request->region_.num; ++i) {
        DBG_LOGINFO("Host name=" << request->region_.hostName[i] << ", affinity=" << request->region_.affinity[i]);
    }
    RegionInfo regionInfo{request->regionName_, 0, request->region_};
    TP_TRACE_BEGIN(TP_UBSM_CREATE_REGIONS);
    auto ret = ock::share::service::RegionManager::GetInstance().CreateRegionInfo(regionInfo);
    TP_TRACE_END(TP_UBSM_CREATE_REGIONS, ret);
    if (!ret) {
        DBG_LOGERROR("Get exception when calling CreateRegionInfo, ret=" << ret);
        DBG_AUDITINFO("user info of RegionCreateRegion, uid="
                      << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid << ", region_name="
                      << request->regionName_ << ", ret=" << ConvertErrorToString(MXM_ERR_REGION_EXIST));
        response->errCode_ = MXM_ERR_REGION_EXIST;
        return ret;
    }

    response->errCode_ = 0;
    DBG_LOGINFO("Create region successfully, name=" << request->regionName_);
    DBG_AUDITINFO("user info of RegionCreateRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                          << ", pid=" << udsInfo.pid
                                                          << ", region_name=" << request->regionName_ << ", success.");
    return 0;
}

int MxmServerMsgHandle::RegionLookupRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmLookupRegionRequest *>(req);
    auto response = dynamic_cast<ShmLookupRegionResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("user info of RegionLookupRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                          << udsInfo.pid << ", region_name=" << request->regionName_);
    DBG_LOGINFO("Looking up region, region_name=" << request->regionName_);
    RegionInfo regionInfo{};
    auto ret = ock::share::service::RegionManager::GetInstance().GetRegionInfo(request->regionName_, regionInfo);
    if (!ret) {
        DBG_LOGERROR("Get exception when GetRegionInfo, regionName is " << request->regionName_);
        DBG_AUDITINFO("user info of RegionLookupRegion, uid="
                      << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid << ", region_name="
                      << request->regionName_ << ", ret=" << ConvertErrorToString(MXM_ERR_REGION_NOT_EXIST));
        response->errCode_ = MXM_ERR_REGION_NOT_EXIST;
        return MXM_ERR_REGION_NOT_EXIST;
    }

    response->region_ = regionInfo.region;
    response->errCode_ = 0;
    DBG_LOGINFO("Get region info successfully, region_name=" << regionInfo.name << ", size=" << regionInfo.size
                                                             << ", number=" << regionInfo.region.num);
    DBG_AUDITINFO("user info of RegionLookupRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                          << ", pid=" << udsInfo.pid
                                                          << ", region_name=" << request->regionName_ << ", success.");
    for (int i = 0; i < regionInfo.region.num; ++i) {
        DBG_LOGDEBUG("NodeId=" << regionInfo.region.nodeId[i] << ", host_name=" << regionInfo.region.hostName[i]
                               << ", affinity=" << regionInfo.region.affinity[i]);
    }
    return 0;
}

int MxmServerMsgHandle::RegionDestroyRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmDestroyRegionRequest *>(req);
    auto response = dynamic_cast<ShmDestroyRegionResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("user info of RegionDestroyRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                           << udsInfo.pid << ", region_name=" << request->regionName_);

    DBG_LOGINFO("Start to delete region info successfully, region_name=" << request->regionName_);
    TP_TRACE_BEGIN(TP_UBSM_DESTORY_REGION);
    auto ret = ock::share::service::RegionManager::GetInstance().DeleteRegionInfo(request->regionName_);
    TP_TRACE_END(TP_UBSM_DESTORY_REGION, ret);
    if (!ret) {
        DBG_LOGERROR("Get exception when calling DeleteRegionInfo, ret=" << ConvertErrorToString(ret));
        DBG_AUDITINFO("user info of RegionDestroyRegion, uid="
                      << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid << ", region_name="
                      << request->regionName_ << ", ret=" << ConvertErrorToString(MXM_ERR_REGION_NOT_EXIST));
        response->errCode_ = MXM_ERR_REGION_NOT_EXIST;
        return ret;
    }

    response->errCode_ = 0;
    DBG_LOGINFO("Delete region info successfully, region_name=" << request->regionName_);
    DBG_AUDITINFO("user info of RegionDestroyRegion, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                           << ", pid=" << udsInfo.pid
                                                           << ", region_name=" << request->regionName_ << ", success.");
    return 0;
}

int MxmServerMsgHandle::RpcQueryNodeInfo(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<RpcQueryInfoResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("RpcQueryNodeInfo, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_LOGINFO("Rpc query node info, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    auto rpcReq = std::make_shared<CommonRequest>();
    if (rpcReq == nullptr) {
        DBG_LOGERROR("rpcReq is nullptr.");
        return MXM_ERR_MALLOC_FAIL;
    }
    for (auto &node : ock::rpc::NetRpcConfig::GetInstance().GetRemoteNodes()) {
        auto rpcRsp = std::make_shared<RpcQueryInfoResponse>();
        if (rpcRsp == nullptr) {
            DBG_LOGERROR("rpcRsp is nullptr.");
            return MXM_ERR_MALLOC_FAIL;
        }
        auto ret = ock::rpc::service::RpcServer::GetInstance().SendMsg(RPC_AGENT_QUERY_NODE_INFO, rpcReq.get(),
                                                                       rpcRsp.get(), node.name);
        response->errCode_ = rpcRsp->errCode_;
        response->name_.append(", ").append(rpcRsp->name_);
        if (ret != 0) {
            DBG_LOGERROR("node: " << node.name << " query info failed, ret = " << ConvertErrorToString(ret));
            DBG_AUDITINFO("user info of RpcQueryNodeInfo, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                                << ", pid=" << udsInfo.pid
                                                                << ", ret=" << ConvertErrorToString(ret));
            return ret;
        }
        DBG_LOGINFO("RpcQueryNodeInfo success, node_name=" << node.name << ", node_ip=" << node.ip);
    }
    DBG_AUDITINFO("RpcQueryNodeInfo success, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ",pid=" << udsInfo.pid);
    return MXM_OK;
}

static int CleanNotUsedRecordShareMemory(const AttachedShareMemory &memory, const MxmComUdsInfo &udsInfo)
{
    std::shared_ptr<ShmUnmapRequest> request;
    std::shared_ptr<CommonResponse> response;

    try {
        request = std::make_shared<ShmUnmapRequest>("default", memory.name);
        response = std::make_shared<CommonResponse>();
    } catch (std::exception &e) {
        DBG_LOGERROR("CleanNotUsedRecordShareMemory: Exception caught." << e.what());
        return MXM_ERR_NULLPTR;
    } catch (...) {
        DBG_LOGERROR("CleanNotUsedRecordShareMemory: Unknown exception caught.");
        return MXM_ERR_UNKNOWN;
    }

    return MxmServerMsgHandle::ShmUnmap(request.get(), response.get(), udsInfo);
}

int MxmServerMsgHandle::AppCheckShareMemoryMap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    DBG_AUDITINFO("AppCheckShareMemoryMap start. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                       << ", pid=" << udsInfo.pid);
    if (req == nullptr || rsp == nullptr) {
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CheckShareMemoryMapRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("requset or response is nullptr.");
        return MXM_ERR_NULLPTR;
    }
    auto records = SHMManager::GetInstance().GetMemoryUsersByPid(udsInfo.pid);

    std::unordered_set<std::string> names(request->names_.begin(), request->names_.end());

    for (auto &record : records) {
        if (names.find(record.name) == names.end()) {
            DBG_LOGINFO("record shm name " << record.name << " not found in user map.");
            auto ret = CleanNotUsedRecordShareMemory(record, udsInfo);
            if (ret != MXM_OK) {
                DBG_LOGERROR("CleanNotUsedRecordMemory failed. name=" << record.name
                                                                      << ", ret=" << ConvertErrorToString(ret));
                continue;
            }
            DBG_LOGINFO("CleanNotUsedRecordMemory successfully. name=" << record.name);
        }
    }

    response->errCode_ = MXM_OK;
    DBG_AUDITINFO("AppCheckShareMemoryMap end. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid
                                                     << " ret=" << ConvertErrorToString(response->errCode_));
    return MXM_OK;
}

int MxmServerMsgHandle::LookupLocalSlotId(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    DBG_AUDITINFO("LookupLocalSlotId start. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    if (req == nullptr || rsp == nullptr) {
        DBG_AUDITINFO("LookupLocalSlotId end. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << " ret=" << ConvertErrorToString(MXM_ERR_NULLPTR));
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const CommonRequest *>(req);
    auto response = dynamic_cast<LookupSlotIdResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("requset or response is nullptr.");
        DBG_AUDITINFO("LookupLocalSlotId end. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << " ret=" << ConvertErrorToString(MXM_ERR_NULLPTR));
        return MXM_ERR_NULLPTR;
    }
    uint32_t slotId = 0;
    auto ret = mxm::UbseMemAdapter::GetLocalNodeId(slotId);
    if (ret != MXM_OK) {
        DBG_LOGERROR("GetLocalNodeId failed. ret=" << ret);
        response->errCode_ = ret;
        DBG_AUDITINFO("LookupLocalSlotId end. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                    << " ret=" << ConvertErrorToString(response->errCode_));
        return ret;
    }
    response->slotId_ = slotId;
    response->errCode_ = MXM_OK;
    DBG_AUDITINFO("LookupLocalSlotId end. uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                << " ret=" << ConvertErrorToString(response->errCode_));
    return MXM_OK;
}

int MxmServerMsgHandle::ShmWriteLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmWriteLockRequest *>(req);
    auto response = dynamic_cast<ShmWriteLockResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("ShmWriteLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_LOGINFO("ShmWriteLock request region_name=" << request->regionName_ << ", shm_name=" << request->name_
                                                    << " pid=" << udsInfo.pid << " uid=" << udsInfo.uid
                                                    << " gid=" << udsInfo.gid);
    dlock_utils::LockUdsInfo info;
    info.pid = udsInfo.pid;
    info.uid = udsInfo.uid;
    info.gid = udsInfo.gid;
    auto ret = ock::dlock_utils::UbsmLock::Instance().Lock(request->name_, true, info);
    if (ret != 0) {
        DBG_LOGERROR("WriteLock failed, name=" << request->name_ << " ret is " << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->errCode_ = MXM_OK;
    DBG_LOGINFO("ShmWriteLock successfully, region_name=" << request->regionName_ << ", name=" << request->name_);
    return MXM_OK;
}

int MxmServerMsgHandle::ShmReadLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmReadLockRequest *>(req);
    auto response = dynamic_cast<ShmReadLockResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("user info of ShmReadLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                   << ", name=" << request->name_);
    DBG_LOGINFO("ShmReadLock request region_name=" << request->regionName_ << ", shm_name=" << request->name_ << " pid="
                                                   << udsInfo.pid << " uid=" << udsInfo.uid << " gid=" << udsInfo.gid);
    dlock_utils::LockUdsInfo info;
    info.pid = udsInfo.pid;
    info.uid = udsInfo.uid;
    info.gid = udsInfo.gid;
    auto ret = ock::dlock_utils::UbsmLock::Instance().Lock(request->name_, false, info);
    if (ret != 0) {
        DBG_LOGINFO("ShmReadLock failed, name=" << request->name_ << " pid=" << udsInfo.pid << " uid=" << udsInfo.uid
                                                << " gid=" << udsInfo.gid << " ret=" << ret);
        DBG_AUDITINFO("ShmReadLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                          << ", name=" << request->name_ << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->errCode_ = MXM_OK;
    DBG_LOGINFO("ShmReadLock successfully, name=" << request->name_ << " pid=" << udsInfo.pid << " uid=" << udsInfo.uid
                                                  << " gid=" << udsInfo.gid);
    DBG_AUDITINFO("user info of ShmReadLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                   << ", name=" << request->name_ << ", success.");
    return MXM_OK;
}

int MxmServerMsgHandle::ShmUnLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmUnLockRequest *>(req);
    auto response = dynamic_cast<ShmUnLockResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("user info of ShmUnLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);
    DBG_LOGINFO("ShmUnLock request region_name=" << request->regionName_ << ", shm_name=" << request->name_ << " pid="
                                                 << udsInfo.pid << " uid=" << udsInfo.uid << " gid=" << udsInfo.gid);
    dlock_utils::LockUdsInfo info;
    info.pid = udsInfo.pid;
    info.uid = udsInfo.uid;
    info.gid = udsInfo.gid;
    auto ret = ock::dlock_utils::UbsmLock::Instance().Unlock(request->name_, info);
    if (ret != 0) {
        DBG_LOGERROR("UnLock failed, name=" << request->name_ << " ret=" << ret);
        DBG_AUDITINFO("ShmUnLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                        << ", name=" << request->name_ << ", ret=" << ConvertErrorToString(ret));
        response->errCode_ = ret;
        return ret;
    }
    response->errCode_ = MXM_OK;
    DBG_LOGINFO("ShmUnLock successfully, name=" << request->name_ << " pid=" << udsInfo.pid << " uid=" << udsInfo.uid
                                                << " gid=" << udsInfo.gid);
    DBG_AUDITINFO("user info of ShmUnLock, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    return MXM_OK;
}

int MxmServerMsgHandle::ShmQueryNode(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const QueryNodeRequest *>(req);
    auto response = dynamic_cast<QueryNodeResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }

    DBG_AUDITINFO("ShmQueryNode, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    std::string nodeId;
    std::string nodeType = request->queryMasterNodeFlag_ ? "master node" : "client node";
    DBG_LOGINFO("Shm Query Node type=" << nodeType << "uid=" << udsInfo.uid << ", pid=" << udsInfo.pid);
    auto zen = zendiscovery::ZenDiscovery::GetInstance();
    // 选主模块未初始化
    if (zen == nullptr) {
        response->errCode_ = MXM_ERR_LOCK_NOT_READY;
        response->nodeIsReady_ = false;
        return 0;
    }
    response->errCode_ = 0;
    if (request->queryMasterNodeFlag_) {
        nodeId = zen->GetCurrentMaster();
    } else {
        zen->GetVoteOnlyNode(nodeId);
    }
    // 节点未选出
    if (nodeId.empty()) {
        response->nodeIsReady_ = false;
        DBG_LOGWARN("nodeId is empty, node is not ready"
                    << ", uid=" << udsInfo.uid << ", pid=" << udsInfo.pid);
        return 0;
    }
    response->nodeIsReady_ = true;
    response->nodeId_ = nodeId;
    DBG_LOGINFO("ShmQueryNode successfully"
                << ", uid=" << udsInfo.uid << ", pid=" << udsInfo.pid);
    return 0;
}

int MxmServerMsgHandle::ShmQueryDlockStatus(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("ShmQueryDlockStatus, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    auto response = dynamic_cast<QueryDlockStatusResponse *>(rsp);
    if (response == nullptr) {
        DBG_LOGERROR("invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto isReady = (dlock_utils::UbsmLock::Instance().IsUbsmLockInit() &&
                    !dlock_utils::UbsmLock::Instance().IsUbsmLockInElection());
    response->isReady_ = isReady;
    response->errCode_ = 0;
    DBG_LOGINFO("ShmQueryDlockStatus successfully, ubsm lock=" << isReady);
    return 0;
}

int MxmServerMsgHandle::ShmAttach(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmAttachRequest *>(req);
    auto response = dynamic_cast<ShmAttachResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    response->errCode_ = UBSM_OK;

    DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);

    mxm::ubse_user_info_t ubsUserInfo{};
    mxm::AttachShmResult result;
    int rt = mxm::UbseMemAdapter::ShmGetUserData(request->name_, ubsUserInfo);
    if (rt != 0) {
        DBG_LOGERROR("ShmGetUserData fail, name=" << request->name_ << ", ret: " << rt);
        DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(rt));
        response->errCode_ = rt;
        return rt;
    }
    int providedPerms = CalculateProvidedPermissions(udsInfo.uid, udsInfo.gid, ubsUserInfo.udsInfo.uid,
                                                     ubsUserInfo.udsInfo.gid, ubsUserInfo.mode);
    int oflag = GetMaxOflag(providedPerms);
    if (oflag == -1) {
        DBG_LOGERROR("Required permission not provided");
        DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", name=" << request->name_ << ", ret="
                                                     << ConvertErrorToString(MXM_ERR_SHM_PERMISSION_DENIED));
        response->errCode_ = MXM_ERR_SHM_PERMISSION_DENIED;
        return MXM_ERR_SHM_PERMISSION_DENIED;
    }
    DBG_LOGINFO("GetMaxOflag success, oflag=" << oflag << ", name=" << request->name_);
    result.oflag = oflag;
    uid_t owner_uid = ubsUserInfo.udsInfo.uid;
    gid_t owner_gid = ubsUserInfo.udsInfo.gid;
    if (udsInfo.uid != owner_uid || udsInfo.gid != owner_gid) {
        DBG_LOGERROR("ShmAttach failed, permission denied, name=" << request->name_);
        DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", name=" << request->name_ << ", ret="
                                                     << ConvertErrorToString(MXM_ERR_SHM_PERMISSION_DENIED));
        response->errCode_ = MXM_ERR_SHM_PERMISSION_DENIED;
        return MXM_ERR_SHM_PERMISSION_DENIED;
    }
    auto hr = mxm::UbseMemAdapter::ShmAttach(request->name_, ubsUserInfo, result);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmAttach " << request->name_ << "ret: " << ConvertErrorToString(hr));
        DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }
    response->memIds_.assign(result.memIds.begin(), result.memIds.end());
    response->shmSize_ = result.shmSize;
    response->unitSize_ = result.unitSize;
    response->flag_ = result.flag;
    response->oflag_ = result.oflag;
    DBG_LOGINFO("Shm attach message from agent successfully " << request->name_ << ", response->oflag_ "
                                                              << response->oflag_);
    DBG_AUDITINFO("user info of ShmAttach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::ShmDetach(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmDetachRequest *>(req);
    auto response = dynamic_cast<CommonResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    DBG_AUDITINFO("user info of ShmDetach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);

    DelayRemovedKey queryBusyKey{request->name_};
    if (UBSMemMonitor::GetInstance().GetDelayRemoveRecord(queryBusyKey)) {
        DBG_LOGERROR("Name " << request->name_ << " is busy.");
        response->errCode_ = MXM_ERR_NAME_BUSY;
        return MXM_ERR_NAME_BUSY;
    }

    response->errCode_ = UBSM_OK;
    mxm::ubse_user_info_t ubsUserInfo{};
    int rt = mxm::UbseMemAdapter::ShmGetUserData(request->name_, ubsUserInfo);
    if (rt != 0) {
        DBG_LOGERROR("ShmGetUserData fail. ret=" << rt << " name=" << request->name_);
        DBG_AUDITINFO("user info of ShmDetach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(rt));
        return rt;
    }
    uid_t owner_uid = ubsUserInfo.udsInfo.uid;
    gid_t owner_gid = ubsUserInfo.udsInfo.gid;
    if (udsInfo.uid != owner_uid || udsInfo.gid != owner_gid) {
        DBG_LOGERROR("ShmDetach failed, permission denied, name=" << request->name_);
        DBG_AUDITINFO("user info of ShmDetach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", name=" << request->name_ << ", ret="
                                                     << ConvertErrorToString(MXM_ERR_SHM_PERMISSION_DENIED));
        response->errCode_ = MXM_ERR_SHM_PERMISSION_DENIED;
        return MXM_ERR_SHM_PERMISSION_DENIED;
    }
    auto hr = mxm::UbseMemAdapter::ShmDetach(request->name_);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmDetach " << request->name_ << "ret: " << ConvertErrorToString(hr));
        DBG_AUDITINFO("user info of ShmDetach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }
    DBG_LOGINFO("Shm detach message from agent successfully, name=" << request->name_);
    DBG_AUDITINFO("user info of ShmDetach, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::ShmListLookup(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmListLookupRequest *>(req);
    auto response = dynamic_cast<ShmListLookupResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    response->errCode_ = UBSM_OK;

    DBG_AUDITINFO("user info of ShmListLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                     << udsInfo.pid << ", prefix name=" << request->prefix_);

    auto hr = mxm::UbseMemAdapter::ShmListLookup(request->prefix_, response->shmNames_);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmListLookup " << request->prefix_ << "ret: " << ConvertErrorToString(hr));
        DBG_AUDITINFO("user info of ShmListLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid="
                                                         << udsInfo.pid << ", prefix name=" << request->prefix_
                                                         << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }
    DBG_LOGINFO("Shm listlookup message from agent successfully " << request->prefix_);
    DBG_AUDITINFO("user info of ShmListLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", prefix name=" << request->prefix_
                                                     << ", success.");
    return UBSM_OK;
}

int MxmServerMsgHandle::ShmLookup(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo)
{
    if (req == nullptr || rsp == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    auto request = dynamic_cast<const ShmLookupRequest *>(req);
    auto response = dynamic_cast<ShmLookupResponse *>(rsp);
    if (request == nullptr || response == nullptr) {
        DBG_LOGERROR("Invalid param.");
        return MXM_ERR_NULLPTR;
    }
    response->errCode_ = UBSM_OK;

    DBG_AUDITINFO("user info of ShmLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_);
    DBG_LOGINFO("ShmLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                  << ", name=" << request->name_);

    auto hr = mxm::UbseMemAdapter::ShmLookup(request->name_, response->shmInfo_);
    if (hr != 0) {
        DBG_LOGERROR("get exception when ShmLookup " << request->name_ << "ret: " << hr);
        DBG_AUDITINFO("user info of ShmLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid
                                                     << ", pid=" << udsInfo.pid << ", name=" << request->name_
                                                     << ", ret=" << ConvertErrorToString(hr));
        response->errCode_ = hr;
        return hr;
    }
    DBG_LOGINFO("Shm lookup message from agent successfully, name="
                << request->name_ << ", uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid);
    DBG_AUDITINFO("user info of ShmLookup, uid=" << udsInfo.uid << ", gid=" << udsInfo.gid << ", pid=" << udsInfo.pid
                                                 << ", name=" << request->name_ << ", success.");
    return UBSM_OK;
}

std::mutex &MxmServerMsgHandle::GetMutexByName(const std::string &name) noexcept
{
    std::hash<std::string> hasher;
    auto index = (hasher(name) % MUTEX_HASH_SIZE);
    return mutexArray[index];
}
} // namespace ock::share::service