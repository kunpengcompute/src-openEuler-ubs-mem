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
#include "ubse_mem_adapter.h"
#include <dlfcn.h>
#include <algorithm>
#include "ubs_mem_def.h"
#include "log.h"
#include "numa_cpu_utils.h"
#include "region_manager.h"
#include "system_adapter.h"
#include "ubs_common_types.h"
#include "ubs_error.h"
#include "ubs_mem_monitor.h"
#include "ubsm_ptracer.h"

#include "record_store.h"

namespace ock {
namespace mxm {
using namespace ock::common;
using namespace ock::share::service;
using namespace ock::ubsm;
using namespace ock::ubsm::tracer;
bool UbseMemAdapter::initialized_{false};
std::mutex UbseMemAdapter::gMutex;
std::atomic<uint32_t> UbseMemAdapter::nodeId_{UINT32_MAX};

/************** api ************/
UbseClientInitializeFunc UbseMemAdapter::pUbseClientInitialize = nullptr;
UbseClientFinalizeFunc UbseMemAdapter::pUbseClientFinalize = nullptr;
UbseNodeListFunc UbseMemAdapter::pUbseNodeList = nullptr;
UbseNodeLocalGetFunc UbseMemAdapter::pUbseNodeLocalGet = nullptr;
UbseTopLinkListFunc UbseMemAdapter::pUbseTopLinkList = nullptr;
UbseNumaStatGetFunc UbseMemAdapter::pUbseNumaStatGet = nullptr;
UbseMemFdCreateFunc UbseMemAdapter::pUbseMemFdCreate = nullptr;
UbseMemFdCreateWithLenderFunc UbseMemAdapter::pUbseMemFdCreateWithLender = nullptr;
UbseMemFdCreateWithCandidateFunc UbseMemAdapter::pUbseMemFdCreateWithCandidate = nullptr;
UbseMemFdPermissionFunc UbseMemAdapter::pUbseMemFdPermission = nullptr;
UbseMemFdGetFunc UbseMemAdapter::pUbseMemFdGet = nullptr;
UbseMemFdDeleteFunc UbseMemAdapter::pUbseMemFdDelete = nullptr;
UbseMemNumaCreateFunc UbseMemAdapter::pUbseMemNumaCreate = nullptr;
UbseMemNumaCreateWithLenderFunc UbseMemAdapter::pUbseMemNumaCreateWithLender = nullptr;
UbseMemNumaCreateWithCandidateFunc UbseMemAdapter::pUbseMemNumaCreateWithCandidate = nullptr;
UbseMemNumaGetFunc UbseMemAdapter::pUbseMemNumaGet = nullptr;
UbseMemNumaDeleteFunc UbseMemAdapter::pUbseMemNumaDelete = nullptr;
UbseMemShmCreateFunc UbseMemAdapter::pUbseMemShmCreate = nullptr;
UbseMemShmCreateWithLenderFunc UbseMemAdapter::pUbseMemShmCreateWithLender = nullptr;
UbseMemShmCreateWithAffinityFunc UbseMemAdapter::pUbseMemShmCreateWithAffinity = nullptr;
UbseMemShmAttachFunc UbseMemAdapter::pUbseMemShmAttach = nullptr;
UbseMemShmGetFunc UbseMemAdapter::pUbseMemShmGet = nullptr;
UbseMemShmListFunc UbseMemAdapter::pUbseMemShmList = nullptr;
UbseMemShmDetachFunc UbseMemAdapter::pUbseMemShmDetach = nullptr;
UbseMemShmDeleteFunc UbseMemAdapter::pUbseMemShmDelete = nullptr;
UbseLogCallBackRegisterFunc UbseMemAdapter::pUbseLogCallBackRegister = nullptr;

int UbseMemAdapter::Initialize()
{
    /*
     * (1) 通过dlopen打开mxe的so，找到各个函数符号
     * (2) 获取拓扑信息，生成本地节点id和集群中hostname -> nodeId的映射
     * (3) 设置初始化完成状态
     */
    std::lock_guard<std::mutex> guard(gMutex);
    if (initialized_) {
        return MXM_OK;
    }
#ifdef DEBUG_MEM_UT
    auto soPath = "libubse-client.so";
#else
    auto soPath = ARCH_LIB_DIR "/libubse-client.so.1";
#endif
    auto ret = DlopenLibUbse(soPath);
    if (ret != 0) {
        DBG_LOGERROR("Failed to dlopen library of ubse, path=" << soPath << ", ret=" << ret);
        return MXM_ERR_UBSE_LIB;
    }
    if (pUbseLogCallBackRegister != nullptr) {
        DBG_LOGINFO("Set ubse log func: ubs_engine_log_callback_register.");
        pUbseLogCallBackRegister(UseLog);
    }
    initialized_ = true;
    return MXM_OK;
}

int32_t UbseMemAdapter::DlopenLibUbse(const std::string &controllerPath)
{
    DBG_LOGINFO("Start to dlopen library of ubse, path=" << controllerPath);
    auto handle = SystemAdapter::DlOpen(controllerPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        DBG_LOGERROR("The controller lib path is invalid. error info=" << dlerror());
        return MXM_ERR_UBSE_LIB;
    }

    pUbseClientInitialize = (UbseClientInitializeFunc)SystemAdapter::DlSym(handle, "ubs_engine_client_initialize");
    if (pUbseClientInitialize == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_engine_client_initialize, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseClientFinalize = (UbseClientFinalizeFunc)SystemAdapter::DlSym(handle, "ubs_engine_client_finalize");
    if (pUbseClientFinalize == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_engine_client_finalize, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseNodeList = (UbseNodeListFunc)SystemAdapter::DlSym(handle, "ubs_topo_node_list");
    if (pUbseNodeList == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_topo_node_list, error info=" << dlerror());
        ResetLibUbseDl();
        dlclose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseNodeLocalGet = (UbseNodeLocalGetFunc)SystemAdapter::DlSym(handle, "ubs_topo_node_local_get");
    if (pUbseNodeLocalGet == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_topo_node_local_get, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseTopLinkList = (UbseTopLinkListFunc)SystemAdapter::DlSym(handle, "ubs_topo_link_list");
    if (pUbseTopLinkList == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_topo_link_list, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseNumaStatGet = (UbseNumaStatGetFunc)SystemAdapter::DlSym(handle, "ubs_mem_numastat_get");
    if (pUbseNumaStatGet == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numastat_get, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdCreate = (UbseMemFdCreateFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_create");
    if (pUbseMemFdCreate == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_create, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdCreateWithLender =
        (UbseMemFdCreateWithLenderFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_create_with_lender");
    if (pUbseMemFdCreateWithLender == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_create_with_lender, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdCreateWithCandidate =
        (UbseMemFdCreateWithCandidateFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_create_with_candidate");
    if (pUbseMemFdCreateWithCandidate == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_create_with_candidate, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdPermission = (UbseMemFdPermissionFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_permission");
    if (pUbseMemFdPermission == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_permission, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdGet = (UbseMemFdGetFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_get");
    if (pUbseMemFdGet == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_get, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemFdDelete = (UbseMemFdDeleteFunc)SystemAdapter::DlSym(handle, "ubs_mem_fd_delete");
    if (pUbseMemFdDelete == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_fd_delete, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemNumaCreate = (UbseMemNumaCreateFunc)SystemAdapter::DlSym(handle, "ubs_mem_numa_create");
    if (pUbseMemNumaCreate == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numa_create, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemNumaCreateWithLender =
        (UbseMemNumaCreateWithLenderFunc)SystemAdapter::DlSym(handle, "ubs_mem_numa_create_with_lender");
    if (pUbseMemNumaCreateWithLender == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numa_create_with_lender, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemNumaCreateWithCandidate =
        (UbseMemNumaCreateWithCandidateFunc)SystemAdapter::DlSym(handle, "ubs_mem_numa_create_with_candidate");
    if (pUbseMemNumaCreateWithCandidate == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numa_create_with_candidate, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemNumaGet = (UbseMemNumaGetFunc)SystemAdapter::DlSym(handle, "ubs_mem_numa_get");
    if (pUbseMemNumaGet == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numa_get, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemNumaDelete = (UbseMemNumaDeleteFunc)SystemAdapter::DlSym(handle, "ubs_mem_numa_delete");
    if (pUbseMemNumaDelete == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_numa_delete, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmCreate = (UbseMemShmCreateFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_create");
    if (pUbseMemShmCreate == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_create, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmCreateWithLender =
        (UbseMemShmCreateWithLenderFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_create_with_lender");
    if (pUbseMemShmCreateWithLender == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_create_with_lender, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmCreateWithAffinity =
        (UbseMemShmCreateWithAffinityFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_create_with_affinity");
    if (pUbseMemShmCreateWithAffinity == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_create_with_affinity, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmAttach = (UbseMemShmAttachFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_attach");
    if (pUbseMemShmAttach == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_attach, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmGet = (UbseMemShmGetFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_get");
    if (pUbseMemShmGet == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_get, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmList = (UbseMemShmListFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_list");
    if (pUbseMemShmList == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubse_mem_shm_list, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmDetach = (UbseMemShmDetachFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_detach");
    if (pUbseMemShmDetach == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_detach, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseMemShmDelete = (UbseMemShmDeleteFunc)SystemAdapter::DlSym(handle, "ubs_mem_shm_delete");
    if (pUbseMemShmDelete == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_mem_shm_delete, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }

    pUbseLogCallBackRegister =
        (UbseLogCallBackRegisterFunc)SystemAdapter::DlSym(handle, "ubs_engine_log_callback_register");
    if (pUbseLogCallBackRegister == nullptr) {
        DBG_LOGERROR("Failed to load symbol ubs_engine_log_callback_register, error info=" << dlerror());
        ResetLibUbseDl();
        SystemAdapter::DlClose(handle);
        return MXM_ERR_UBSE_LIB;
    }
    return MXM_OK;
}

void UbseMemAdapter::ResetLibUbseDl()
{
    pUbseClientInitialize = nullptr;
    pUbseClientFinalize = nullptr;
    pUbseNodeList = nullptr;
    pUbseNodeLocalGet = nullptr;
    pUbseTopLinkList = nullptr;
    pUbseNumaStatGet = nullptr;
    pUbseMemFdCreate = nullptr;
    pUbseMemFdCreateWithLender = nullptr;
    pUbseMemFdCreateWithCandidate = nullptr;
    pUbseMemFdPermission = nullptr;
    pUbseMemFdGet = nullptr;
    pUbseMemFdDelete = nullptr;
    pUbseMemNumaCreate = nullptr;
    pUbseMemNumaCreateWithLender = nullptr;
    pUbseMemNumaCreateWithCandidate = nullptr;
    pUbseMemNumaGet = nullptr;
    pUbseMemNumaDelete = nullptr;
    pUbseMemShmCreate = nullptr;
    pUbseMemShmCreateWithLender = nullptr;
    pUbseMemShmCreateWithAffinity = nullptr;
    pUbseMemShmAttach = nullptr;
    pUbseMemShmGet = nullptr;
    pUbseMemShmList = nullptr;
    pUbseMemShmDetach = nullptr;
    pUbseMemShmDelete = nullptr;
    pUbseLogCallBackRegister = nullptr;
}

void UbseMemAdapter::Destroy()
{
    /*
     * 关闭dlopen句柄，清除函数指针，清除初始化状态
     */
    std::lock_guard<std::mutex> guard(gMutex);
    pUbseClientFinalize();
    ResetLibUbseDl();
    initialized_ = false;
}

int FillUserInfo(const AppContext &ownner, uint64_t flags, mode_t mode, ubse_user_info_t &ubsUserInfo, uint64_t &seqNo)
{
    ubsUserInfo.udsInfo.uid = ownner.uid;
    ubsUserInfo.udsInfo.gid = ownner.gid;
    ubsUserInfo.udsInfo.pid = ownner.pid;
    ubsUserInfo.flag = flags;
    ubsUserInfo.mode = mode;

    uint32_t nodeId = UINT32_MAX;
    auto ret = UbseMemAdapter::EnsureGetLocalNodeId(nodeId);
    if (ret != HOK) {
        DBG_LOGERROR("Failed to get local nodeId, ret=" << ret);
        return ret;
    }
    ubsUserInfo.seqNo = UbseMemAdapter::GenerateCreateSeqNo(nodeId);
    seqNo = ubsUserInfo.seqNo;
    DBG_LOGINFO("GenerateCreateSeqNo successfully. seqNo=" << seqNo);
    return HOK;
}

int UbseMemAdapter::GetRegionInfo(const std::string &regionName, std::vector<uint32_t> &slotIds)
{
    RegionInfo regionInfo{};
    if (regionName == "default") {
        SHMRegions regions;
        auto hr = mxm::UbseMemAdapter::LookupRegionList(regions);
        if (hr != 0 || regions.num == 0) {
            DBG_LOGERROR("LookupRegionList failed, ret=" << hr);
            return hr;
        }
        regionInfo.region = regions.region[0]; // 默认域取第一个域
        regionInfo.name = "default";
        for (int i = 0; i < regionInfo.region.num; i++) {
            regionInfo.region.affinity[i] = true;
        }
    } else {
        auto ret = RegionManager::GetInstance().GetRegionInfo(regionName, regionInfo);
        if (!ret) {
            DBG_LOGERROR("Get exception when GetRegionInfo, regionName=" << regionName);
            return MXM_ERR_REGION_NOT_FOUND;
        }
    }

    for (int i = 0; i < regionInfo.region.num; i++) {
        if (regionInfo.region.affinity[i]) {
            uint32_t tempSlotId{1u};

            if (!StrUtil::StrToUint(std::string(regionInfo.region.nodeId[i]), tempSlotId)) {
                DBG_LOGERROR("Get slotIds[" << i << "] failed, nodeId= " << regionInfo.region.nodeId[i]);
                return MXM_ERR_PARAM_INVALID;
            }
            slotIds.emplace_back(tempSlotId);
        }
    }
    if (slotIds.empty()) {
        DBG_LOGERROR("slotId is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    return MXM_OK;
}

int32_t UbseMemAdapter::MemFdCreateWithCandidate(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner,
                                                 mode_t mode, const uint32_t *slot_ids, uint32_t slot_cnt,
                                                 ubs_mem_fd_desc_t *fd_desc)
{
    if (pUbseMemFdCreateWithCandidate == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    return pUbseMemFdCreateWithCandidate(name, size, owner, mode, slot_ids, slot_cnt, fd_desc);
}

int UbseMemAdapter::LeaseMalloc(const ock::mxm::LeaseMallocParam &param, ock::ubsm::LeaseMallocResult &result)
{
    auto rt = Initialize();
    if (rt != 0) {
        DBG_LOGERROR("Initialize failed, name is " << param.name << ", ret: " << rt);
        return rt;
    }

    std::vector<uint32_t> slotIds{};
    auto hr = GetRegionInfo(param.regionName, slotIds);
    if (hr != 0) {
        DBG_LOGERROR("GetRegionInfo failed, regionName=" << param.regionName << ", ret=" << hr);
        return hr;
    }

    uint32_t slotCnt = slotIds.size();
    auto *slotIdsPtr = new (std::nothrow) uint32_t[slotCnt];
    if (slotIdsPtr == nullptr) {
        DBG_LOGERROR("slotIdsPtr is nullptr");
        return MXM_ERR_MALLOC_FAIL;
    }
    for (int i = 0; i < slotCnt; ++i) {
        slotIdsPtr[i] = slotIds[i];
    }

    DBG_LOGINFO("Start to use pUbseNodeLocalGet func.");
    ubs_topo_node_t localNode{};
    TP_TRACE_BEGIN(TP_UBSM_GET_LOCAL_NODE_ID);
    hr = pUbseNodeLocalGet(&localNode);
    TP_TRACE_END(TP_UBSM_GET_LOCAL_NODE_ID, hr);
    if (hr != 0) {
        DBG_LOGINFO("pUbseNodeLocalGet failed, ret: " << hr);
        SAFE_DELETE_ARRAY(slotIdsPtr);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("localNode slot_id is " << localNode.slot_id << " socket_id 0 is " << localNode.socket_id[0]
                                        << " socket_id 1 is " << localNode.socket_id[1] << " host name "
                                        << localNode.host_name);
    if (param.isNuma) {
        DBG_LOGINFO("LeaseMalloc with numa");
        ubs_mem_numa_desc_t numaDesc;
        TP_TRACE_BEGIN(TP_UBSM_CREATE_NUMA_MEM);
        hr = pUbseMemNumaCreateWithCandidate(param.name.c_str(), static_cast<int64_t>(param.size), slotIdsPtr, slotCnt,
                                             &numaDesc);
        TP_TRACE_END(TP_UBSM_CREATE_NUMA_MEM, hr);
        if (hr != UBS_SUCCESS && hr != UBS_ERR_TIMED_OUT) {
            DBG_LOGERROR("pUbseMemNumaCreateWithCandidate failed, ret: " << hr);
            SAFE_DELETE_ARRAY(slotIdsPtr);
            if (hr == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("pUbseMemNumaCreateWithCandidate not supported.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
        if (hr == UBS_ERR_TIMED_OUT) {
            DelayRemovedKey removedKey{
                param.name, ONE_MINUTE_MS, true, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
                false,      true,          true};
            DBG_LOGERROR("Numa malloc failed, errCode=" << hr << " name=" << param.name
                                                        << " pid=" << param.appContext.pid
                                                        << ", expires time=" << removedKey.expiresTime);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            SAFE_DELETE_ARRAY(slotIdsPtr);
            return UBS_ERR_TIMED_OUT;
        }
        result.numaId = numaDesc.numaid;
        result.memIds.emplace_back(0);
        result.slotId = numaDesc.export_node.slot_id;
    } else {
        DBG_LOGINFO("LeaseMalloc with fd");
        ubs_mem_fd_owner_t owner;
        owner.uid = param.appContext.uid;
        owner.gid = param.appContext.gid;
        owner.pid = param.appContext.pid;
        mode_t mode = S_IRUSR | S_IWUSR;
        ubs_mem_fd_desc_t fdDesc;
        TP_TRACE_BEGIN(TP_UBSM_CREATE_FD_MEM);
        hr = MemFdCreateWithCandidate(param.name.c_str(), static_cast<int64_t>(param.size), &owner, mode, slotIdsPtr,
                                      slotCnt, &fdDesc);
        TP_TRACE_END(TP_UBSM_CREATE_FD_MEM, hr);
        if (hr != UBS_SUCCESS && hr != UBS_ERR_TIMED_OUT) {
            DBG_LOGERROR("MemFdCreateWithCandidate failed, ret: " << hr);
            SAFE_DELETE_ARRAY(slotIdsPtr);
            if (hr == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("MemFdCreateWithCandidate not supported.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
        if (hr == UBS_ERR_TIMED_OUT) {
            DelayRemovedKey removedKey{
                param.name, ONE_MINUTE_MS, true, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
                false,      false,         true};
            DBG_LOGERROR("Fd malloc failed, errCode=" << hr << " name=" << param.name << " pid=" << param.appContext.pid
                                                      << ", expires time=" << removedKey.expiresTime);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            SAFE_DELETE_ARRAY(slotIdsPtr);
            return UBS_ERR_TIMED_OUT;
        }
        for (int i = 0; i < fdDesc.memid_cnt; ++i) {
            result.memIds.emplace_back(fdDesc.memids[i]);
        }
        result.numaId = param.isNuma ? param.isNuma : -1;
        result.unitSize = fdDesc.unit_size;
        result.slotId = fdDesc.export_node.slot_id;
    }
    SAFE_DELETE_ARRAY(slotIdsPtr);
    DBG_LOGINFO("Malloc res is " << hr << ", name is " << param.name << ", numa is " << result.numaId << ", size is "
                                 << result.unitSize);
    return hr;
}

int32_t UbseMemAdapter::MemFdCreateWithLender(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                              const ubs_mem_lender_t *lender, uint32_t lender_cnt,
                                              ubs_mem_fd_desc_t *fd_desc)
{
    if (pUbseMemFdCreateWithLender == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    return pUbseMemFdCreateWithLender(name, owner, mode, lender, lender_cnt, fd_desc);
}

int UbseMemAdapter::LeaseMallocWithLoc(const LeaseMallocWithLocParam &param, ock::ubsm::LeaseMallocResult &result)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, name=" << param.name << ", ret=" << ret);
        return ret;
    }

    ubs_mem_lender_t lender;
    lender.slot_id = param.slotId;
    lender.socket_id = param.socketId;
    lender.numa_id = param.numaId;
    lender.port_id = param.portId;
    lender.lender_size = param.size;
    DBG_LOGINFO("lender info: slot_id=" << lender.slot_id << ", socket_id=" << lender.socket_id
                                        << ", numa_id=" << lender.numa_id << ", port_id=" << lender.port_id
                                        << ", lender_size=" << lender.lender_size);
    if (param.isNuma) {
        DBG_LOGINFO("LeaseMallocWithLoc with numa");
        ubs_mem_numa_desc_t numaDesc;
        TP_TRACE_BEGIN(TP_UBSM_CREATE_NUMA_MEM_WITH_LOC);
        ret = pUbseMemNumaCreateWithLender(param.name.c_str(), &lender, 1, &numaDesc);
        TP_TRACE_END(TP_UBSM_CREATE_NUMA_MEM_WITH_LOC, ret);
        if (ret != UBS_SUCCESS && ret != UBS_ERR_TIMED_OUT) {
            DBG_LOGERROR("pUbseMemNumaCreateWithLender failed, ret=" << ret);
            if (ret == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("pUbseMemNumaCreateWithLender not supported.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
        if (ret == UBS_ERR_TIMED_OUT) {
            DelayRemovedKey removedKey{
                param.name, ONE_MINUTE_MS, true, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
                false,      true,          true};
            DBG_LOGERROR("Numa malloc failed, errCode=" << ret << " name=" << param.name
                                                        << " pid=" << param.appContext.pid
                                                        << ", expires time=" << removedKey.expiresTime);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            return UBS_ERR_TIMED_OUT;
        }
        result.numaId = numaDesc.numaid;
        result.memIds.emplace_back(0);
        result.slotId = numaDesc.export_node.slot_id;
    } else {
        DBG_LOGINFO("LeaseMallocWithLoc with fd");
        ubs_mem_fd_owner_t owner;
        owner.uid = param.appContext.uid;
        owner.gid = param.appContext.gid;
        owner.pid = param.appContext.pid;
        mode_t mode = S_IRUSR | S_IWUSR;
        ubs_mem_fd_desc_t fdDesc;
        TP_TRACE_BEGIN(TP_UBSM_CREATE_FD_MEM_WITH_LOC);
        ret = MemFdCreateWithLender(param.name.c_str(), &owner, mode, &lender, 1, &fdDesc);
        TP_TRACE_END(TP_UBSM_CREATE_FD_MEM_WITH_LOC, ret);
        if (ret != UBS_SUCCESS && ret != UBS_ERR_TIMED_OUT) {
            DBG_LOGERROR("MemFdCreateWithLender failed, ret=" << ret);
            if (ret == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("MemFdCreateWithLender not supported.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
        if (ret == UBS_ERR_TIMED_OUT) {
            DelayRemovedKey removedKey{
                param.name, ONE_MINUTE_MS, true, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
                false,      false,         true};
            DBG_LOGERROR("Fd malloc failed, errCode=" << ret << " name=" << param.name
                                                      << " pid=" << param.appContext.pid
                                                      << ", expires time=" << removedKey.expiresTime);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            return UBS_ERR_TIMED_OUT;
        }
        for (int i = 0; i < fdDesc.memid_cnt; ++i) {
            result.memIds.emplace_back(fdDesc.memids[i]);
        }
        result.numaId = param.isNuma ? param.isNuma : -1;
        result.unitSize = fdDesc.unit_size;
        result.slotId = fdDesc.export_node.slot_id;
    }
    DBG_LOGINFO("LeaseMallocWithLoc ret=" << ret << ", name=" << param.name << ", numa=" << result.numaId
                                          << ", size=" << result.unitSize << ", slotId=" << result.slotId);
    return ret;
}

uint32_t FillRegionDesc(SHMRegionDesc &regionDesc, std::set<uint32_t> &intersection)
{
    regionDesc.type = ALL2ALL_SHARE;
    regionDesc.perfLevel = PerfLevel::L0;
    regionDesc.num = intersection.size();
    int index = 0;
    for (auto item : intersection) {
        if (index >= MEM_TOPOLOGY_MAX_HOSTS) {
            DBG_LOGERROR("index: << " << index << " is out of range: " << MEM_TOPOLOGY_MAX_HOSTS);
            return MXM_ERR_REGION_PARAM_INVALID;
        }
        auto ret = strcpy_s(regionDesc.nodeId[index], MEM_MAX_ID_LENGTH, std::to_string(item).c_str());
        if (ret != EOK) {
            DBG_LOGERROR("nodeId copy error, ret=" << ret);
            return MXM_ERR_MEMORY;
        }

        regionDesc.nodeId[index][MEM_MAX_ID_LENGTH - 1] = '\0';           // core问题处理
        regionDesc.hostName[index][MAX_HOST_NAME_DESC_LENGTH - 1] = '\0'; // core问题处理
        index++;
    }
    return MXM_OK;
}

int32_t UbseMemAdapter::PopulateHostNameMap(SHMRegions &regions)
{
    if (pUbseNodeList == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    ubs_topo_node_t *nodeList;
    uint32_t nodeCnt;
    TP_TRACE_BEGIN(TP_UBSM_GET_NODE_LIST);
    auto ret = pUbseNodeList(&nodeList, &nodeCnt);
    TP_TRACE_END(TP_UBSM_GET_NODE_LIST, ret);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseNodeList failed, ret=" << ret);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("Ubse NodeList get success, node cnt=" << nodeCnt);

    if (nodeList == nullptr || nodeCnt == 0 || (nodeCnt > UBS_MEM_MAX_SLOT_NUM)) {
        DBG_LOGERROR("pUbseNodeList failed, ret=" << ret << ", nodeCnt=" << nodeCnt);
        return MXM_ERR_UBSE_INNER;
    }

    std::unordered_map<uint32_t, std::string> slotToHostName;
    for (uint32_t i = 0; i < nodeCnt; ++i) {
        DBG_LOGINFO("node info of slot_id[" << i << "]=" << nodeList[i].slot_id
                                            << ", hostname=" << nodeList[i].host_name);
        DBG_LOGINFO(" socket_id[0]=" << nodeList[i].socket_id[0] << ", numa[0][0]=" << nodeList[i].numa_ids[0][0]
                                     << " numa[0][1]=" << nodeList[i].numa_ids[0][1] << ", socket_id[1]="
                                     << nodeList[i].socket_id[1] << ", numa[1][0]=" << nodeList[i].numa_ids[1][0]
                                     << " numa[1][1]=" << nodeList[i].numa_ids[1][1]);
        slotToHostName[nodeList[i].slot_id] = nodeList[i].host_name;
    }
    if (nodeList != nullptr) {
        free(nodeList);
        nodeList = nullptr;
    }

    if (nodeCnt < MIN_NODE_NUM) {
        DBG_LOGERROR("pUbseNodeList found node list less than two, node cnt: " << nodeCnt);
        return MXM_ERR_UBSE_INNER;
    }

    for (int i = 0; i < regions.num; ++i) {
        SHMRegionDesc &region = regions.region[i];
        for (int j = 0; j < region.num; ++j) {
            DBG_LOGINFO("region nodeId[" << j << "]=" << region.nodeId[j]);
            uint32_t nodeId{1u};
            if (!StrUtil::StrToUint(std::string(region.nodeId[j]), nodeId)) {
                DBG_LOGERROR("Invalid node ID, nodeId[" << j << "]=" << region.nodeId[j]);
                return MXM_ERR_PARAM_INVALID;
            }
            auto it = slotToHostName.find(nodeId);
            if (it == slotToHostName.end()) {
                DBG_LOGWARN("nodeId[" << j << "]:" << region.nodeId[j] << " has no hostname in slotToHostName map");
                continue;
            }

            ret =
                strncpy_s(region.hostName[j], MAX_HOST_NAME_DESC_LENGTH, it->second.c_str(), MAX_HOST_NAME_DESC_LENGTH);
            if (ret != EOK) {
                DBG_LOGERROR("hostName copy error, ret=" << ret);
                return ret;
            }
            region.hostName[j][MAX_HOST_NAME_DESC_LENGTH - 1] = '\0'; // 确保字符串以 '\0' 结尾
        }
    }
    // 打印调试信息
    for (int i = 0; i < regions.num; i++) {
        DBG_LOGINFO("Regions[" << i << "]=" << regions.region[i].type << ", num=" << regions.region[i].num);
        for (int j = 0; j < regions.region[i].num; j++) {
            DBG_LOGINFO("Regions num[" << j << "]=" << regions.region[i].nodeId[j]
                                       << ", hostname=" << regions.region[i].hostName[j]);
        }
    }
    return MXM_OK;
}

int32_t UbseMemAdapter::InitializeRegionList(SHMRegions &regions)
{
    if (pUbseNodeLocalGet == nullptr || pUbseTopLinkList == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    ubs_topo_node_t localNode;
    TP_TRACE_BEGIN(TP_UBSM_GET_LOCAL_NODE_ID);
    auto res = pUbseNodeLocalGet(&localNode);
    TP_TRACE_END(TP_UBSM_GET_LOCAL_NODE_ID, res);
    if (res != 0) {
        DBG_LOGERROR("Get local node failed, ret=" << res);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("Local node slot_id=" << localNode.slot_id << ", socket_id[0]=" << localNode.socket_id[0]
                                      << ", socket_id[1]=" << localNode.socket_id[1]
                                      << ", host_name=" << localNode.host_name);

    ubs_topo_link_t *linkPtr = nullptr;
    uint32_t cpuLinkCnt{0};
    res = pUbseTopLinkList(&linkPtr, &cpuLinkCnt);
    if (res != 0 || linkPtr == nullptr) {
        DBG_LOGERROR("Failed to call UbseTopLinkList, ret=" << res);
        return MXM_ERR_UBSE_INNER;
    }
    if (cpuLinkCnt > UBS_TOPO_MAX_CPU_LINK_NUM) {
        DBG_LOGERROR("pUbseTopLinkList cpuLinkCnt is invalid, cnt=" << cpuLinkCnt);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("pUbseTopLinkList success, cpuLinkCnt=" << cpuLinkCnt);
    std::unordered_map<uint32_t, std::set<uint32_t>> neiBorNode{};
    std::set<uint32_t> intersection1;
    std::set<uint32_t> intersection2;
    // 构建邻居节点集合
    for (int i = 0; i < cpuLinkCnt; ++i) {
        DBG_LOGINFO("ubs_topo_link_t slot_id=" << linkPtr[i].slot_id << ", socket_id=" << linkPtr[i].socket_id
                                               << ", port_id=" << linkPtr[i].port_id
                                               << ", peer_slot_id=" << linkPtr[i].peer_slot_id
                                               << ", peer_socket_id=" << linkPtr[i].peer_socket_id
                                               << ", peer_port_id=" << linkPtr[i].peer_port_id);
        // 将 slot_id 作为键，添加到 neiBorNode
        if (neiBorNode.find(linkPtr[i].slot_id) == neiBorNode.end()) {
            std::set<uint32_t> tmpSet{};
            tmpSet.insert(linkPtr[i].peer_slot_id);
            neiBorNode[linkPtr[i].slot_id] = tmpSet;
        } else {
            neiBorNode[linkPtr[i].slot_id].insert(linkPtr[i].peer_slot_id);
        }
        // 将 peer_slot_id 作为键，添加到 neiBorNode
        if (neiBorNode.find(linkPtr[i].peer_slot_id) == neiBorNode.end()) {
            std::set<uint32_t> tmpSet{};
            tmpSet.insert(linkPtr[i].slot_id);
            neiBorNode[linkPtr[i].peer_slot_id] = tmpSet;
        } else {
            neiBorNode[linkPtr[i].peer_slot_id].insert(linkPtr[i].slot_id);
        }
    }
    if (neiBorNode.find(localNode.slot_id) == neiBorNode.end()) {
        DBG_LOGERROR("local not in topo.");
        free(linkPtr);
        return MXM_ERR_REGION_PARAM_INVALID;
    }
    uint32_t firstNeiBorNode = *neiBorNode[localNode.slot_id].begin();
    // 计算交集和差集
    std::set_intersection(neiBorNode[localNode.slot_id].begin(), neiBorNode[localNode.slot_id].end(),
                          neiBorNode[firstNeiBorNode].begin(), neiBorNode[firstNeiBorNode].end(),
                          std::inserter(intersection1, intersection1.begin()));
    intersection1.insert(localNode.slot_id); // 增加本节点
    intersection1.insert(firstNeiBorNode);   // 增加邻居节点
    std::set_difference(neiBorNode[localNode.slot_id].begin(), neiBorNode[localNode.slot_id].end(),
                        intersection1.begin(), intersection1.end(),
                        std::inserter(intersection2, intersection2.begin()));
    intersection2.insert(localNode.slot_id);
    // 填充区域描述符
    regions.num = 0;
    if (intersection1.size() > 1) {
        SHMRegionDesc regionDesc{};
        auto ret = FillRegionDesc(regionDesc, intersection1);
        if (ret != 0) {
            DBG_LOGERROR("Failed to fill region desc, ret=" << ret);
            free(linkPtr);
            return ret;
        }
        regions.region[regions.num] = regionDesc;
        regions.num++;
    }
    if (intersection2.size() > 1) {
        SHMRegionDesc regionDesc{};
        auto ret = FillRegionDesc(regionDesc, intersection2);
        if (ret != 0) {
            DBG_LOGERROR("Failed to fill region desc, ret=" << ret);
            free(linkPtr);
            return ret;
        }
        regions.region[regions.num] = regionDesc;
        regions.num++;
    }
    if (regions.num == 0) {
        DBG_LOGERROR("Failed to get region, Regions number=0");
        free(linkPtr);
        return MXM_ERR_REGION_PARAM_INVALID;
    }
    for (int i = 0; i < regions.num; i++) {
        DBG_LOGINFO("Regions[" << i << "] is=" << regions.region[i].type << ", num=" << regions.region[i].num);
        for (int j = 0; j < regions.region[i].num; j++) {
            DBG_LOGINFO("Regions num[" << j << "]=" << regions.region[i].nodeId[j]
                                       << ", hostname=" << regions.region[i].hostName[j]);
        }
    }
    free(linkPtr);
    return UBSM_OK;
}

int32_t UbseMemAdapter::CheckAndCopyRegionStatus(SHMRegions &staticRegions, SHMRegions &activeRegion)
{
    if (staticRegions.num > MAX_REGIONS_NUM) {
        DBG_LOGERROR("Static region is error, number of static regions=" << staticRegions.num);
        return MXM_ERR_PARAM_INVALID;
    }
    int regionsCnt = 0;
    for (int i = 0; i < staticRegions.num; ++i) {
        const auto &region = staticRegions.region[i];
        int nodeCnt = 0;
        activeRegion.region[regionsCnt].type = region.type;
        activeRegion.region[regionsCnt].perfLevel = region.perfLevel;
        for (int j = 0; j < region.num; ++j) {
            if (strlen(region.nodeId[j]) == 0 || strlen(region.hostName[j]) == 0) {
                DBG_LOGWARN("nodeId or hostname is empty, cnt=" << j << ", id=" << region.nodeId[j]
                                                                << ", hostname=" << region.hostName[j]);
                continue;
            }
            int ret = memcpy_s(activeRegion.region[regionsCnt].nodeId[nodeCnt], MEM_MAX_ID_LENGTH, region.nodeId[j],
                               MEM_MAX_ID_LENGTH);
            if (ret != 0) {
                DBG_LOGERROR("Failed to copy hostId, static region id=" << region.nodeId[j]);
                return MXM_ERR_MALLOC_FAIL;
            }
            ret = memcpy_s(activeRegion.region[regionsCnt].hostName[nodeCnt], MAX_HOST_NAME_DESC_LENGTH,
                           region.hostName[j], MAX_HOST_NAME_DESC_LENGTH);
            if (ret != 0) {
                DBG_LOGERROR("Failed to copy hostId, static region name=" << region.hostName[j]);
                return MXM_ERR_MALLOC_FAIL;
            }
            nodeCnt++;
        }
        activeRegion.region[regionsCnt].num = nodeCnt;
        if (nodeCnt != 0) {
            regionsCnt++;
        }
    }
    activeRegion.num = regionsCnt;
    for (int i = 0; i < activeRegion.num; i++) {
        DBG_LOGINFO("Regions[" << i << "]=" << activeRegion.region[i].type << ", num=" << activeRegion.region[i].num);
        for (int j = 0; j < activeRegion.region[i].num; j++) {
            DBG_LOGINFO("Regions num[" << j << "]=" << activeRegion.region[i].nodeId[j]
                                       << ", hostname=" << activeRegion.region[i].hostName[j]);
        }
    }
    return UBSM_OK;
}

int32_t UbseMemAdapter::LookupRegionList(SHMRegions &regions)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Failed to initialize, ret=" << ret);
        return ret;
    }

    SHMRegions staticRegions{};
    // 初始化拓扑信息
    ret = InitializeRegionList(staticRegions);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to initialize regionList, ret=" << ret);
        return ret;
    }

    // 获取节点列表并填充主机名映射
    ret = PopulateHostNameMap(staticRegions);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to populate host name map, ret=" << ret);
        return ret;
    }

    ret = CheckAndCopyRegionStatus(staticRegions, regions);
    if (ret != UBSM_OK) {
        DBG_LOGERROR("Failed to convert to dynamic regions, ret=" << ret);
        return ret;
    }

    return UBSM_OK;
}

int UbseMemAdapter::LookUpClusterStatistic(ubsmemClusterInfo &clusterInfo)
{
    if (pUbseNumaStatGet == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    ubs_topo_node_t *nodeList;
    uint32_t nodeCnt;
    DBG_LOGINFO("LookUpClusterStatistic start.");
    auto ret = pUbseNodeList(&nodeList, &nodeCnt);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseNodeList failed, ret: " << ret);
        return MXM_ERR_UBSE_INNER;
    }
    if (nodeList == nullptr || nodeCnt == 0 || (nodeCnt > UBS_MEM_MAX_SLOT_NUM)) {
        DBG_LOGERROR("pUbseNodeList failed, ret=" << ret << ", nodeCnt=" << nodeCnt);
        return MXM_ERR_UBSE_INNER;
    }

    ubs_mem_numastat_t *numaMems;
    uint32_t numaMemCnt;
    clusterInfo.host_num = 0;
    for (int i = 0; i < nodeCnt; ++i) {
        DBG_LOGINFO("Query nodeId=" << std::to_string(nodeList[i].slot_id) << ", nodeCnt=" << nodeCnt);
        ret = pUbseNumaStatGet(nodeList[i].slot_id, &numaMems, &numaMemCnt);
        if (ret != UBS_SUCCESS || numaMems == nullptr || numaMemCnt == 0 || numaMemCnt > UBS_TOPO_NUMA_NUM) {
            DBG_LOGERROR("pUbseNumaStatGet failed, ret: " << ret);
            continue;
        }

        DBG_LOGINFO("NodeId=" << std::to_string(nodeList[i].slot_id) << " has numaMemCnt=" << numaMemCnt);
        for (int j = 0; j < numaMemCnt; ++j) {
            if (strlen(nodeList[i].host_name) >= MAX_HOST_NAME_DESC_LENGTH) {
                DBG_LOGERROR("hostname too long: name=" << nodeList[i].host_name);
                free(numaMems);
                free(nodeList);
                return MXM_ERR_UBSE_INNER;
            }
            ret = memcpy_s(clusterInfo.host[clusterInfo.host_num].host_name, MAX_HOST_NAME_DESC_LENGTH,
                           nodeList[i].host_name, MAX_HOST_NAME_DESC_LENGTH);
            if (ret != 0) {
                DBG_LOGERROR("memcpy_s failed, host name is " << nodeList[i].host_name << ", ret: " << ret);
                free(numaMems);
                free(nodeList);
                return MXM_ERR_MALLOC_FAIL;
            }
            clusterInfo.host[clusterInfo.host_num].numa[j].slot_id = numaMems[j].slot_id;
            clusterInfo.host[clusterInfo.host_num].numa[j].numa_id = numaMems[j].numa_id;
            clusterInfo.host[clusterInfo.host_num].numa[j].socket_id = numaMems[j].socket_id;
            clusterInfo.host[clusterInfo.host_num].numa[j].mem_total = numaMems[j].mem_total;
            clusterInfo.host[clusterInfo.host_num].numa[j].mem_free = numaMems[j].mem_free;
            clusterInfo.host[clusterInfo.host_num].numa[j].mem_borrow = numaMems[j].mem_borrow;
            clusterInfo.host[clusterInfo.host_num].numa[j].mem_lend = numaMems[j].mem_lend;
            clusterInfo.host[clusterInfo.host_num].numa[j].mem_lend_ratio = numaMems[j].mem_lend_ratio;
            clusterInfo.host[clusterInfo.host_num].numa_num++;
            DBG_LOGINFO(
                "host nodeid: " << numaMems[j].slot_id << ",host_name "
                                << clusterInfo.host[clusterInfo.host_num].host_name
                                << ", numaid: " << numaMems[j].numa_id << ", socket_id: " << numaMems[j].socket_id
                                << ", mem_total: " << numaMems[j].mem_total << ", mem_free: " << numaMems[j].mem_free
                                << ", mem_borrow: " << numaMems[j].mem_borrow << ", mem_lend: " << numaMems[j].mem_lend
                                << ", mem_lend_ratio: " << numaMems[j].mem_lend_ratio);
        }
        clusterInfo.host_num++;
        free(numaMems);
    }

    if (nodeList != nullptr) {
        free(nodeList);
    }
    return UBSM_OK;
}

uint32_t UbseMemAdapter::GetSocketIdWithNumaNode(int numaNode, uint32_t *socketId)
{
    if (pUbseNodeLocalGet == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("GetSocketIdWithNumaNode, node:  " << numaNode);
    ubs_topo_node_t localNode;
    TP_TRACE_BEGIN(TP_UBSM_GET_LOCAL_NODE_ID);
    auto res = pUbseNodeLocalGet(&localNode);
    TP_TRACE_END(TP_UBSM_GET_LOCAL_NODE_ID, res);
    if (res != 0) {
        DBG_LOGERROR("Get local node failed res is " << res);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("localNode slot_id: " << localNode.slot_id << ", socket_id[0]: " << localNode.socket_id[0]
                                      << " (numa_ids[0][0]: " << localNode.numa_ids[0][0]
                                      << ", numa_ids[0][1]: " << localNode.numa_ids[0][1] << ")"
                                      << ", socket_id[1] " << localNode.socket_id[1]
                                      << " (numa_ids[1][0]: " << localNode.numa_ids[1][0]
                                      << ", numa_ids[1][1]: " << localNode.numa_ids[1][1] << ")"
                                      << ", host_name " << localNode.host_name);
    // 遍历所有socket，查找匹配的NUMA节点
    for (int i = 0; i < UBS_TOPO_SOCKET_NUM; ++i) {
        for (int j = 0; j < UBS_TOPO_NUMA_NUM; ++j) {
            if (localNode.numa_ids[i][j] == numaNode) {
                DBG_LOGINFO("Found socket_id[" << i << "] = " << localNode.socket_id[i] << " for NUMA node "
                                               << numaNode);
                *socketId = localNode.socket_id[i];
                return MXM_OK;
            }
        }
    }

    DBG_LOGERROR("NUMA node " << numaNode << " not found in local node topology.");
    return MXM_ERR_SHM_INVALID_NUMA;
}

int UbseMemAdapter::ShmCreateWithAffinity(const CreateShmParam &param, const ubs_mem_nodes_t &region,
                                          const uint64_t ubseFlags, const uint8_t *usrInfo, uint64_t createSeqNo)
{
    if (pUbseMemShmCreateWithAffinity == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized.");
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("Calling ShmCreateWithAffinity, name=" << param.name);
    TP_TRACE_BEGIN(TP_UBSE_IS_BOUND_NUMA);
    auto numaNode = NumaCpuUtils::IsAppBoundToOneNumaNode(param.appContext.pid);
    TP_TRACE_END(TP_UBSE_IS_BOUND_NUMA, 0);
    if (numaNode == -1) {
        DBG_LOGWARN("parem (name=" << param.name << ", pid=" << param.appContext.pid
                                   << ") is not bound on one numa node.");
        return MXM_ERR_SHM_INVALID_NUMA;
    }

    DBG_LOGINFO("create shm with numa node=" << numaNode << " affinity, name=" << param.name
                                             << ", pid=" << param.appContext.pid);
    uint32_t socketId;
    auto res = GetSocketIdWithNumaNode(numaNode, &socketId);
    if (res == MXM_OK) {
        // 成功找到对应的socket ID
        DBG_LOGINFO("GetSocketIdWithNumaNode found socket ID=" << socketId << ", create shm name=" << param.name);
        TP_TRACE_BEGIN(TP_UBSE_MEM_SHM_CREATE_WITH_AFFINITY);
        auto ret = pUbseMemShmCreateWithAffinity(param.name.c_str(), static_cast<uint64_t>(param.size), socketId,
                                                 const_cast<uint8_t *>(usrInfo), ubseFlags, &region, &param.privider);
        TP_TRACE_END(TP_UBSE_MEM_SHM_CREATE_WITH_AFFINITY, ret);
        if (ret != UBS_SUCCESS && ret != UBS_ERR_TIMED_OUT) { // 失败打warning
            DBG_LOGERROR("pUbseMemShmCreateWithAffinity failed, ret=" << ret << ", name=" << param.name);
            if (ret == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("pUbseMemShmCreateWithAffinity not support.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
        if (ret == UBS_ERR_TIMED_OUT) {
            DelayRemovedKey removedKey{
                param.name, ONE_MINUTE_MS, false, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
                false,      false,         true};
            removedKey.isAttach = false;
            removedKey.createSeqNo = createSeqNo;
            DBG_LOGERROR("Create share memory fail, errCode=" << ret << " name=" << param.name
                                                              << " pid=" << param.appContext.pid
                                                              << ", expires time=" << removedKey.expiresTime);
            UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
            return UBS_ERR_TIMED_OUT;
        }
        DBG_LOGINFO("pUbseMemShmCreateWithAffinity successfully, name=" << param.name);
        return MXM_OK;
    }
    DBG_LOGWARN("Not Found socket id, numa node=" << numaNode << ", ret=" << res);
    return res;
}

uint64_t UbseMemAdapter::GenerateCreateSeqNo(uint32_t slotId)
{
    return (static_cast<uint64_t>(slotId) << 32U) | RecordStore::GetInstance().GetCreateSeqNo();
}

int PrepareUserInfoAndFlags(const ubse_user_info_t &ubsUserInfo, uint8_t *usrInfo, uint64_t *ubseFlags)
{
    int ret = memcpy_s(usrInfo, UBS_MEM_MAX_USR_INFO_LEN, &ubsUserInfo, sizeof(ubsUserInfo));
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed, ret=" << ret);
        return MXM_ERR_MALLOC_FAIL;
    }
    uint64_t flags = 0;
    if (ubsUserInfo.flag & UBSM_FLAG_WR_DELAY_COMP) {
        flags |= UBS_MEM_FLAG_NO_WR_DELAY;
    }
    if (ubsUserInfo.flag & UBSM_FLAG_MEM_ANONYMOUS) {
        flags |= UBS_MEM_FLAG_SHM_ANONYMOUS;
    }
    if (!(ubsUserInfo.flag & UBSM_FLAG_NONCACHE) && !(ubsUserInfo.flag & UBSM_FLAG_ONLY_IMPORT_NONCACHE)) {
        flags |= UBS_MEM_FLAG_CACHEABLE;
    }
    *ubseFlags = flags;
    DBG_LOGINFO("PrepareUserInfoAndFlags ubseFlags=" << *ubseFlags);
    return MXM_OK;
}

int UbseMemAdapter::ShmCreate(const CreateShmParam &param)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Failed to Initialize, name=" << param.name << ", ret=" << ret);
        return ret;
    }
    if (pUbseMemShmCreate == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    ubs_mem_nodes_t region;
    uint32_t count = 0;
    for (int i = 0; i < param.desc.num; i++) {
        DBG_LOGINFO("Region nodeId=" << param.desc.nodeId[i]);
        uint32_t nodeId{1u};
        if (!StrUtil::StrToUint(std::string(param.desc.nodeId[i]), nodeId)) {
            DBG_LOGERROR("Invalid node ID, nodeId[" << i << "]=" << param.desc.nodeId[i]);
            return MXM_ERR_PARAM_INVALID;
        }
        region.slot_ids[i] = nodeId;
        count++;
    }
    region.node_cnt = count;

    ubse_user_info_t ubsUserInfo{};
    uint64_t seqNo = 0;
    ret = FillUserInfo(param.appContext, param.flag, param.mode, ubsUserInfo, seqNo);
    if (ret != HOK) {
        DBG_LOGERROR("FillUserInfo failed, ret=" << ret);
        return ret;
    }
    uint8_t usrInfo[UBS_MEM_MAX_USR_INFO_LEN] = {0};
    uint64_t ubseFlags{0};

    ret = PrepareUserInfoAndFlags(ubsUserInfo, usrInfo, &ubseFlags);
    if (ret != MXM_OK) {
        DBG_LOGERROR("PrepareUserInfoAndFlags failed, name=" << param.name << ", ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("Param name=" << param.name << "size=" << param.size << "User info: uid=" << ubsUserInfo.udsInfo.uid
                              << ", gid=" << ubsUserInfo.udsInfo.gid << ", pid=" << ubsUserInfo.udsInfo.pid << ", flag="
                              << ubsUserInfo.flag << ", mode=" << ubsUserInfo.mode << ", ubseFlags=" << ubseFlags);

    ret = ShmCreateWithAffinity(param, region, ubseFlags, usrInfo, seqNo);
    if (ret == MXM_OK) {
        DBG_LOGINFO("ShmCreateWithAffinity success, name=" << param.name);
        return ret;
    }

    DBG_LOGWARN("Shm create with numa node affinity failed, name=" << param.name << ", ret=" << ret);

    TP_TRACE_BEGIN(TP_UBSE_MEM_SHM_CREATE);
    ret = pUbseMemShmCreate(param.name.c_str(), static_cast<uint64_t>(param.size), usrInfo, ubseFlags, &region,
                            &param.privider);
    TP_TRACE_END(TP_UBSE_MEM_SHM_CREATE, ret);
    if (ret != UBS_SUCCESS && ret != UBS_ERR_TIMED_OUT) {
        DBG_LOGERROR("pUbseMemShmCreate failed, ret=" << ret << ", name=" << param.name);
        if (ret == UBS_ENGINE_ERR_EXISTED) {
            return MXM_ERR_SHM_ALREADY_EXIST;
        }
        if (ret == UBS_ERR_NOT_SUPPORTED) {
            DBG_LOGERROR("pUbseMemShmCreate not support.");
            return MXM_ERR_UBSE_NOT_SUPPORTED;
        }
        return MXM_ERR_UBSE_INNER;
    }
    if (ret == UBS_ERR_TIMED_OUT) {
        DelayRemovedKey removedKey{
            param.name, ONE_MINUTE_MS, false, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
            false,      false,         true};
        removedKey.isAttach = false;
        removedKey.createSeqNo = seqNo;
        DBG_LOGERROR("Create share memory fail, errCode=" << ret << " name=" << param.name
                                                          << " pid=" << param.appContext.pid
                                                          << ", expires time=" << removedKey.expiresTime);
        UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
        return UBS_ERR_TIMED_OUT;
    }
    DBG_LOGINFO("Creating shared memory successfully, name=" << param.name);
    return MXM_OK;
}

int UbseMemAdapter::GetSlotIdFromHostName(const std::string &hostName, uint32_t *slotId)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Failed to Initialize, host name=" << hostName << ", ret=" << ret);
        return ret;
    }
    if (hostName.empty()) {
        DBG_LOGERROR("Host name is empty");
        return MXM_ERR_PARAM_INVALID;
    }
    ubs_topo_node_t *nodeList;
    uint32_t nodeCnt;
    TP_TRACE_BEGIN(TP_UBSM_GET_NODE_LIST);
    ret = pUbseNodeList(&nodeList, &nodeCnt);
    TP_TRACE_END(TP_UBSM_GET_NODE_LIST, ret);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseNodeList failed, ret=" << ret);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("Ubse nodeList get success, nodeCnt=" << nodeCnt << ", hostName=" << hostName);

    if (nodeList == nullptr || nodeCnt == 0 || (nodeCnt > UBS_MEM_MAX_SLOT_NUM)) {
        DBG_LOGERROR("pUbseNodeList return invalid, ret=" << ret << ", nodeCnt=" << nodeCnt);
        return MXM_ERR_UBSE_INNER;
    }

    std::unordered_map<std::string, uint32_t> hostToSlotId;
    for (uint32_t i = 0; i < nodeCnt; ++i) {
        DBG_LOGINFO("Node list info: slot_id[" << i << "]=" << nodeList[i].slot_id
                                               << ", hostname=" << nodeList[i].host_name);
        hostToSlotId[nodeList[i].host_name] = nodeList[i].slot_id;
    }

    auto it = hostToSlotId.find(hostName);
    if (it == hostToSlotId.end()) {
        DBG_LOGERROR("HostName= " << hostName << " not found from ubs engine cluster.");
        free(nodeList);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    *slotId = it->second;
    free(nodeList);
    return MXM_OK;
}

int UbseMemAdapter::PrepareShmCreateWithProviderParams(const CreateShmWithProviderParam &param, uint8_t *usrInfo,
                                                       uint64_t *ubseFlags, uint32_t *slotId, uint64_t &seqNo)
{
    int ret = GetSlotIdFromHostName(param.hostName, slotId);
    if (ret != MXM_OK) {
        DBG_LOGERROR("GetSlotIdFromHostName failed, ret=" << ret << ", hostname=" << param.hostName);
        return ret;
    }
    ubse_user_info_t ubsUserInfo{};
    ret = FillUserInfo(param.appContext, param.flag, param.mode, ubsUserInfo, seqNo);
    if (ret != HOK) {
        DBG_LOGERROR("FillUserInfo failed, ret=" << ret);
        return ret;
    }
    ret = PrepareUserInfoAndFlags(ubsUserInfo, usrInfo, ubseFlags);
    if (ret != MXM_OK) {
        DBG_LOGERROR("PrepareUserInfoAndFlags failed, name=" << param.name << ", ret=" << ret);
        return ret;
    }
    DBG_LOGINFO("PrepareShmCreateWithProviderParams, shm name="
                << param.name << ", uid=" << ubsUserInfo.udsInfo.uid << ", gid=" << ubsUserInfo.udsInfo.gid
                << ", pid=" << ubsUserInfo.udsInfo.pid << ", flag=" << ubsUserInfo.flag << ", mode=" << ubsUserInfo.mode
                << ", ubseFlags=" << *ubseFlags << ", slotId=" << *slotId);
    return MXM_OK;
}

int UbseMemAdapter::ShmCreateWithProvider(const CreateShmWithProviderParam &param)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Failed to Initialize, name=" << param.name << ", ret=" << ret);
        return ret;
    }

    uint8_t usrInfo[UBS_MEM_MAX_USR_INFO_LEN] = {0};
    uint64_t ubseFlags{0};
    uint32_t slotId = UINT32_MAX;
    uint64_t seqNo = 0;
    ret = PrepareShmCreateWithProviderParams(param, usrInfo, &ubseFlags, &slotId, seqNo);
    if (ret != MXM_OK) {
        DBG_LOGERROR("PrepareShmCreateWithProviderParams failed, name=" << param.name << ", ret=" << ret);
        return ret;
    }

    ubs_mem_lender_t provider;
    provider.slot_id = slotId;
    provider.socket_id = param.socketId;
    provider.numa_id = param.numaId;
    provider.port_id = param.portId;
    provider.lender_size = static_cast<uint64_t>(param.size);

    DBG_LOGINFO("ShmCreateWithProvider, shm name="
                << param.name << ", size=" << provider.lender_size << ", hostName=" << param.hostName
                << ", slotId=" << provider.slot_id << ", socketId=" << provider.socket_id
                << ", numaId=" << provider.numa_id << ", protId=" << provider.port_id << ", ubseFlags=" << ubseFlags);

    TP_TRACE_BEGIN(TP_UBSE_MEM_SHM_CREATE_WITH_PROVIDER);
    ret = pUbseMemShmCreateWithLender(param.name.c_str(), usrInfo, ubseFlags, nullptr, &provider);
    TP_TRACE_END(TP_UBSE_MEM_SHM_CREATE_WITH_PROVIDER, ret);
    if (ret != UBS_SUCCESS && ret != UBS_ERR_TIMED_OUT) {
        DBG_LOGERROR("pUbseMemShmCreateWithLender failed, ret=" << ret << ", name=" << param.name);
        if (ret == UBS_ENGINE_ERR_EXISTED) {
            return MXM_ERR_SHM_ALREADY_EXIST;
        }
        if (ret == UBS_ERR_NOT_SUPPORTED) {
            DBG_LOGERROR("pUbseMemShmCreateWithLender not support.");
            return MXM_ERR_UBSE_NOT_SUPPORTED;
        }
        return MXM_ERR_UBSE_INNER;
    }
    if (ret == UBS_ERR_TIMED_OUT) {
        DelayRemovedKey removedKey{
            param.name, ONE_MINUTE_MS, false, {param.appContext.pid, param.appContext.uid, param.appContext.gid},
            false,      false,         true};
        DBG_LOGERROR("Create shm with provider fail, errCode=" << ret << " name=" << param.name
                                                               << " pid=" << param.appContext.pid
                                                               << ", expires time=" << removedKey.expiresTime);
        removedKey.isAttach = false;
        removedKey.createSeqNo = seqNo;
        UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
        return UBS_ERR_TIMED_OUT;
    }
    DBG_LOGINFO("Creating shm with provider successfully, name=" << param.name);
    return MXM_OK;
}

int UbseMemAdapter::ShmDelete(const std::string &name, const ubsm::AppContext &appContext)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, ret=" << ret);
        return ret;
    }
    if (pUbseMemShmDelete == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("Deleting shared memory, name=" << name);
    TP_TRACE_BEGIN(TP_UBSE_DELETE_SHARED_MEMORY);
    ret = pUbseMemShmDelete(name.c_str());
    TP_TRACE_END(TP_UBSE_DELETE_SHARED_MEMORY, ret);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("Delete failed, name=" << name << ", ret=" << ret);
        if (ret == UBS_ENGINE_ERR_SHM_ATTACH_USING) {
            return MXM_ERR_SHM_IN_USING;
        }
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
            return MXM_ERR_SHM_NOT_EXIST;
        }
        if (ret == UBS_ERR_NOT_SUPPORTED) {
            DBG_LOGERROR("pUbseMemShmDelete not support.");
            return MXM_ERR_UBSE_NOT_SUPPORTED;
        }
        return MXM_ERR_UBSE_INNER;
    }
    return MXM_OK;
}

int UbseMemAdapter::ShmGetUserData(const std::string &name, ubse_user_info_t &ubsUserInfo)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, name is " << name << ", ret: " << ret);
        return ret;
    }
    if (pUbseMemShmGet == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("shm get user data of name=" << name);
    ubs_mem_shm_desc_t *desc = nullptr;
    TP_TRACE_BEGIN(TP_UBSE_GET_SHARED_MEMORY);
    ret = pUbseMemShmGet(name.c_str(), &desc);
    TP_TRACE_END(TP_UBSE_GET_SHARED_MEMORY, ret);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseMemShmGet failed, ret=" << ret << ", name=" << name);
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
            return MXM_ERR_SHM_NOT_EXIST;
        }
        return MXM_ERR_UBSE_INNER;
    }
    if (desc == nullptr) {
        DBG_LOGERROR("pUbseMemShmGet desc is nullptr, name=" << name);
        return MXM_ERR_UBSE_INNER;
    }

    DBG_LOGINFO("pUbseMemShmGet name=" << name << ", import descCnt=" << desc->import_desc_cnt);

    ret = memcpy_s(&ubsUserInfo, sizeof(ubsUserInfo), desc->usr_info, sizeof(ubsUserInfo));
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed when restoring userinfo, ret:" << ret);
        FreeShmDesc(desc);
        return MXM_ERR_MALLOC_FAIL;
    }
    FreeShmDesc(desc);
    return MXM_OK;
}

void UbseMemAdapter::FreeShmDesc(ubs_mem_shm_desc_t *shmDes)
{
    if (!shmDes) {
        return;
    }
    free(shmDes);
    shmDes = nullptr;
}
uint64_t GetSeqNoFromUbseUsrInfo(uint8_t userInfo[UBS_MEM_MAX_USR_INFO_LEN])
{
    ubse_user_info_t ubsUserInfo;
    auto ret = memcpy_s(&ubsUserInfo, sizeof(ubsUserInfo), userInfo, UBS_MEM_MAX_USR_INFO_LEN);
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed when restoring userinfo, ret=" << ret);
        return UINT32_MAX;
    }
    return ubsUserInfo.seqNo;
}

int UbseMemAdapter::GetTimeOutShmTaskStatus(const std::string &name, ubs_mem_stage &status, bool isAttach,
                                            uint64_t &seqNo)
{
    ubs_mem_shm_desc_t *desc = nullptr;
    auto ret = pUbseMemShmGet(name.c_str(), &desc);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseMemShmGet failed, ret=" << ret << ", name=" << name.c_str());
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) { // 可以删除，其他错误码需要重试
            return MXM_ERR_SHM_NOT_EXIST;
        }
        return MXM_ERR_UBSE_INNER;
    }
    if (desc == nullptr) {
        DBG_LOGERROR("pUbseMemShmGet desc is nullptr, name=" << name);
        return MXM_ERR_UBSE_INNER;
    }
    if (desc->import_desc_cnt > UBS_TOPO_MAX_NODE_NUM) {
        DBG_LOGERROR("pUbseMemShmGet desc import_desc_cnt is invalid, cnt=" << desc->import_desc_cnt);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("pUbseMemShmGet name=" << name << ", import descCnt=" << desc->import_desc_cnt);
    ubs_topo_node_t localNode{};
    ret = pUbseNodeLocalGet(&localNode);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseNodeLocalGet failed, ret: " << ret << " name: " << name.c_str());
        return MXM_ERR_UBSE_INNER;
    }
    if (!isAttach) { // create
        if (desc->import_desc_cnt != 0) {
            DBG_LOGINFO("Shred memory is in using. Name=" << name << ", import_desc_cnt=" << desc->import_desc_cnt);
            return MXM_ERR_SHM_IN_USING; // 如果In using, 则认为用户已经知道创建成功且正在使用，无需再回滚。
        }
        status = desc->mem_stage;
        seqNo = GetSeqNoFromUbseUsrInfo(desc->usr_info);
        DBG_LOGINFO("Name=" << name << ", mem stage=" << status);
        return MXM_OK;
    }
    // attach
    if (desc->import_desc_cnt == 0) {
        DBG_LOGINFO("Name=" << name << ", import_desc_cnt=" << desc->import_desc_cnt);
        return MXM_ERR_SHM_NOT_EXIST;
    }
    bool exist = false;
    for (int i = 0; i < desc->import_desc_cnt; ++i) {
        if (desc->import_desc[i].import_node.slot_id == localNode.slot_id) {
            status = desc->import_desc[i].mem_stage;
            exist = true;
            DBG_LOGINFO("NodeId=" << localNode.slot_id << ", name=" << name << ", mem stage=" << status);
            break;
        }
    }
    if (!exist) {
        DBG_LOGINFO("Shared memory not attached on current node. name=" << name << ", import_desc_cnt="
                                                                        << desc->import_desc_cnt);
        return MXM_ERR_SHM_NOT_EXIST;
    }

    return MXM_OK;
}

int UbseMemAdapter::GetTimeOutTaskStatus(const std::string &name, bool isLease, bool isNumaLease, ubs_mem_stage &status,
                                         bool isAttach)
{
    DBG_LOGINFO("GetTimeOutTaskStatus start, name=" << name << ", isLease=" << isLease);
    if (!isLease) {
        return MXM_ERR_UNKNOWN; // shm用新的GetTimeOutShmTaskStatus
    } else {
        if (!isNumaLease) {
            ubs_mem_fd_desc_t fdDesc{};
            auto res = pUbseMemFdGet(name.c_str(), &fdDesc);
            if (res != 0) {
                DBG_LOGERROR("pUbseMemFdGet failed, name=" << name << " res=" << res);
                if (res == UBS_ENGINE_ERR_NOT_EXIST) {
                    return MXM_ERR_LEASE_NOT_EXIST;
                }
                return MXM_ERR_UBSE_INNER;
            }
            status = fdDesc.mem_stage;
            DBG_LOGINFO("Name=" << name << ", mem stage=" << status);
        } else {
            ubs_mem_numa_desc_t numaDesc{};
            auto res = pUbseMemNumaGet(name.c_str(), &numaDesc);
            if (res != 0) {
                DBG_LOGERROR("pUbseMemNumaGet failed, name=" << name << " res=" << res);
                if (res == UBS_ENGINE_ERR_NOT_EXIST) {
                    return MXM_ERR_LEASE_NOT_EXIST;
                }
                return MXM_ERR_UBSE_INNER;
            }
            status = numaDesc.mem_stage;
            DBG_LOGINFO("Name=" << name << ", mem stage=" << status);
        }
    }
    return MXM_OK;
}

int UbseMemAdapter::ShmAttach(const std::string &name, const ubse_user_info_t &ubsUserInfo, AttachShmResult &result)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, name is " << name << ", ret: " << ret);
        return ret;
    }
    if (pUbseMemShmAttach == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    ubs_mem_fd_owner_t owner;
    owner.uid = ubsUserInfo.udsInfo.uid;
    owner.gid = ubsUserInfo.udsInfo.gid;
    owner.pid = ubsUserInfo.udsInfo.pid;
    ubs_mem_shm_desc_t *shmDes = nullptr;
    DBG_LOGINFO("pUbseMemShmAttach name=" << name);
    TP_TRACE_BEGIN(TP_UBSE_ATTACH_SHARED_MEMORY);
    ret = pUbseMemShmAttach(name.c_str(), &owner, ubsUserInfo.mode, &shmDes);
    TP_TRACE_END(TP_UBSE_ATTACH_SHARED_MEMORY, ret);
    if (ret == UBS_ERR_TIMED_OUT) {
        AppContext context{static_cast<uint32_t>(ubsUserInfo.udsInfo.pid), ubsUserInfo.udsInfo.uid,
                           ubsUserInfo.udsInfo.gid};
        DelayRemovedKey removedKey{name, ONE_MINUTE_MS, false, context, false, false, true};
        DBG_LOGERROR("Attach share memory fail, errCode=" << ret << " name=" << name << " pid=" << context.pid
                                                          << ", expires time=" << removedKey.expiresTime);
        removedKey.isAttach = true;
        UBSMemMonitor::GetInstance().AddDelayRemoveRecord(removedKey);
        return UBS_ERR_TIMED_OUT;
    }
    if (ret != UBS_SUCCESS && ret != UBS_ENGINE_ERR_EXISTED) {
        DBG_LOGERROR("pUbseMemShmAttach failed, ret: " << ret);
        if (ret == UBS_ERR_NOT_SUPPORTED) {
            DBG_LOGERROR("pUbseMemShmAttach not support.");
            return MXM_ERR_UBSE_NOT_SUPPORTED;
        }
        return MXM_ERR_UBSE_INNER;
    }
    if (shmDes == nullptr) {
        DBG_LOGERROR("pUbseMemShmAttach desc is nullptr.");
        return MXM_ERR_UBSE_INNER;
    }

    DBG_LOGINFO("pUbseMemShmAttach name=" << name << ", import_desc_cnt=" << shmDes->import_desc_cnt);

    if (shmDes->import_desc_cnt != 1) {
        DBG_LOGERROR("pUbseMemShmAttach import_desc_cnt=" << shmDes->import_desc_cnt);
        FreeShmDesc(shmDes);
        return MXM_ERR_UBSE_INNER;
    }

    result.shmSize = shmDes->mem_size;
    result.unitSize = shmDes->unit_size;
    for (uint32_t i = 0; i < shmDes->import_desc[0].memid_cnt; ++i) {
        result.memIds.emplace_back(shmDes->import_desc[0].memids[i]);
        DBG_LOGDEBUG("ubs_mem_fd_desc_t memid_cnt: " << shmDes->import_desc[0].memid_cnt << " index: " << i
                                                     << "memids: " << shmDes->import_desc[0].memids[i]);
    }
    DBG_LOGINFO("shmDes shmSize: " << shmDes->mem_size << " unitSize: " << shmDes->unit_size
                                   << " memidCnt: " << shmDes->import_desc[0].memid_cnt);
    result.flag = ubsUserInfo.flag;
    if ((ubsUserInfo.flag & UBSM_FLAG_ONLY_IMPORT_NONCACHE) != 0 &&
        (shmDes->export_node.slot_id == shmDes->import_desc[0].import_node.slot_id)) {
        result.flag &= ~UBSM_FLAG_ONLY_IMPORT_NONCACHE;
    }
    FreeShmDesc(shmDes);
    return MXM_OK;
}

int UbseMemAdapter::ShmDetach(const std::string &name)
{
#ifdef UBSE_SDK
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, name is " << name << ", ret: " << ret);
        return ret;
    }
    if (pUbseMemShmDetach == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("pUbseMemShmDetach name=" << name);
    TP_TRACE_BEGIN(TP_UBSE_SHM_DATECH_SHARED_MEMORY);
    ret = pUbseMemShmDetach(name.c_str());
    TP_TRACE_END(TP_UBSE_SHM_DATECH_SHARED_MEMORY, ret);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseMemShmDetach failed, ret=" << ret);
        if (ret == UBS_ENGINE_ERR_SHM_NO_ATTACH) {
            return MXM_ERR_UBSE_NOT_ATTACH;
        }
        if (ret == UBS_ERR_NOT_SUPPORTED) {
            DBG_LOGERROR("pUbseMemShmDetach not support.");
            return MXM_ERR_UBSE_NOT_SUPPORTED;
        }
        return MXM_ERR_UBSE_INNER;
    }
#endif
    DBG_LOGINFO("pUbseMemShmDetach success.");
    return MXM_OK;
}

int UbseMemAdapter::ShmListLookup(const std::string &prefix, std::vector<ubsmem_shmem_desc_t> &nameList)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, prefix is " << prefix << ", ret: " << ret);
        return ret;
    }
    if (pUbseMemShmList == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_LIB;
    }
    ubs_mem_shm_desc_t *descs;
    uint32_t desc_cnt;
    DBG_LOGINFO("pUbseMemShmList start, prefix=" << prefix);
    ret = pUbseMemShmList(&descs, &desc_cnt);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseMemShmList failed, ret: " << ret);
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
            return MXM_ERR_SHM_NOT_FOUND;
        }
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("pUbseMemShmList success, shm prefix=" << prefix << ", descCnt=" << desc_cnt);
    for (uint32_t i = 0; i < desc_cnt; i++) {
        DBG_LOGINFO("descs[" << i << "] name=" << descs[i].name << ", size=" << descs[i].mem_size);
        if (strncmp(descs[i].name, prefix.c_str(), prefix.length()) == 0) {
            ubsmem_shmem_desc_t shmDesc;
            ret = strcpy_s(shmDesc.name, MAX_SHM_NAME_LENGTH + 1, descs[i].name);
            if (ret != EOK) {
                DBG_LOGERROR("name copy error, ret=" << ret);
                free(descs);
                return ret;
            }
            shmDesc.size = descs[i].mem_size;
            nameList.emplace_back(shmDesc);
        }
    }
    free(descs);
    return MXM_OK;
}

int UbseMemAdapter::ShmLookup(const std::string &name, ubsmem_shmem_info_t &shmInfo)
{
    if (Initialize() != 0) {
        DBG_LOGERROR("Initialize failed, name is " << name);
        return MXM_ERR_UBSE_LIB;
    }

    if (pUbseNodeLocalGet == nullptr || pUbseMemShmGet == nullptr) {
        DBG_LOGERROR("Ubsm adapter is not initialized");
        return MXM_ERR_UBSE_LIB;
    }
    DBG_LOGINFO("pUbseNodeLocalGet name=" << name);
    ubs_topo_node_t localNode{};
    auto ret = pUbseNodeLocalGet(&localNode);
    if (ret != 0) {
        DBG_LOGERROR("pUbseNodeLocalGet failed, ret=" << ret);
        return MXM_ERR_UBSE_INNER;
    }
    DBG_LOGINFO("localNode slot_id =" << localNode.slot_id << " socket_id 0=" << localNode.socket_id[0]
                                      << " socket_id 1=" << localNode.socket_id[1]
                                      << " host name=" << localNode.host_name);

    ubs_mem_shm_desc_t *desc = nullptr;
    ret = pUbseMemShmGet(name.c_str(), &desc);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("pUbseMemShmGet failed, ret: " << ret << " name: " << name.c_str());
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
            return MXM_ERR_SHM_NOT_FOUND;
        }
        return MXM_ERR_UBSE_INNER;
    }

    if (desc == nullptr) {
        DBG_LOGERROR("pUbseMemShmGet desc is nullptr.");
        return MXM_ERR_UBSE_INNER;
    }

    DBG_LOGINFO("pUbseMemShmGet name=" << name << ", import_desc_cnt=" << desc->import_desc_cnt);

    for (uint32_t i = 0; i < desc->import_desc_cnt; i++) {
        if (desc->import_desc[i].import_node.slot_id == localNode.slot_id) {
            ret = strcpy_s(shmInfo.name, MAX_SHM_NAME_LENGTH + 1, desc->name);
            if (ret != EOK) {
                DBG_LOGERROR("name copy error, ret=" << ret);
                FreeShmDesc(desc);
                return MXM_ERR_MEMORY;
            }

            shmInfo.size = desc->mem_size;
            shmInfo.mem_unit_size = desc->unit_size;
            shmInfo.mem_num = desc->import_desc[i].memid_cnt;
            DBG_LOGINFO("NodeId= " << localNode.slot_id << " name=" << shmInfo.name << ", size=" << shmInfo.size
                                   << ", unit size=" << shmInfo.mem_unit_size << ", mem num=" << shmInfo.mem_num);
            for (uint32_t j = 0; j < desc->import_desc[i].memid_cnt; j++) {
                shmInfo.mem_id_list[j] = desc->import_desc[i].memids[j];
                DBG_LOGDEBUG("  mem id= " << shmInfo.mem_id_list[j]);
            }
            FreeShmDesc(desc);
            return MXM_OK;
        }
    }
    FreeShmDesc(desc);
    DBG_LOGERROR("ShmLookup failed, cannot find shm name=" << name << " in local node=" << localNode.slot_id);
    return MXM_ERR_SHM_NOT_FOUND;
}

int UbseMemAdapter::FdPermissionChange(const std::string &name, const ubsm::AppContext &appContext, mode_t mode)
{
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, name is " << name << ", ret: " << ret);
        return ret;
    }

    DBG_LOGINFO("Name=" << name << ", uid =" << appContext.uid << ", gid=" << appContext.gid << ", mode=" << mode);
    ubs_mem_fd_owner_t owner{appContext.uid, appContext.gid, static_cast<pid_t>(appContext.pid)};
    ret = pUbseMemFdPermission(name.c_str(), &owner, mode);
    if (ret != 0) {
        DBG_LOGERROR("pUbseMemFdPermission failed, name=" << name << " ret=" << ret);
        return MXM_ERR_UBSE_INNER;
    }
    return MXM_OK;
}

int UbseMemAdapter::LeaseFree(const std::string &name, bool isNuma)
{
    DBG_LOGINFO("Start to lease free memory, name=" << name << ", isNuma=" << isNuma);
    auto ret = Initialize();
    if (ret != 0) {
        DBG_LOGERROR("Initialize failed, ret: " << ret);
        return ret;
    }
    if (pUbseMemFdDelete == nullptr || pUbseMemNumaDelete == nullptr) {
        DBG_LOGERROR("Ubsm is not initialized");
        return MXM_ERR_UBSE_INNER;
    }
    if (!isNuma) {
        TP_TRACE_BEGIN(TP_UBSM_DELETE_FD_MEM);
        auto res = pUbseMemFdDelete(name.c_str());
        TP_TRACE_END(TP_UBSM_DELETE_FD_MEM, res);
        if (res != 0) {
            if (res == UBS_ENGINE_ERR_UNIMPORT_SUCCESS) {
                DBG_LOGWARN("pUbseMemFdDelete failed, name=" << name << " ret=" << res);
                return MXM_OK;
            }
            DBG_LOGERROR("pUbseMemFdDelete failed, name " << name << " res " << res);
            if (res == UBS_ENGINE_ERR_NOT_EXIST) {
                return MXM_ERR_LEASE_NOT_EXIST;
            }
            if (res == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("pUbseMemFdDelete not support.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
    } else {
        TP_TRACE_BEGIN(TP_UBSM_DELETE_NUMA_MEM);
        auto res = pUbseMemNumaDelete(name.c_str());
        TP_TRACE_END(TP_UBSM_DELETE_NUMA_MEM, res);
        if (res != 0) {
            if (res == UBS_ENGINE_ERR_UNIMPORT_SUCCESS) {
                DBG_LOGWARN("pUbseMemNumaDelete failed, name=" << name << " ret=" << res);
                return MXM_OK;
            }
            DBG_LOGERROR("pUbseMemNumaDelete failed, name " << name << " res " << res);
            if (res == UBS_ENGINE_ERR_NOT_EXIST) {
                return MXM_ERR_LEASE_NOT_EXIST;
            }
            if (res == UBS_ERR_NOT_SUPPORTED) {
                DBG_LOGERROR("pUbseMemNumaDelete not support.");
                return MXM_ERR_UBSE_NOT_SUPPORTED;
            }
            return MXM_ERR_UBSE_INNER;
        }
    }
    DBG_LOGINFO("Lease free memory success, name=" << name);
    return MXM_OK;
}

void UbseMemAdapter::UseLog(uint32_t level, const char *str)
{
    switch (level + 1) {
        case static_cast<int>(DBG_LOG_DEBUG):
            DBG_LOGDEBUG(str);
            break;
        case static_cast<int>(DBG_LOG_INFO):
            DBG_LOGINFO(str);
            break;
        case static_cast<int>(DBG_LOG_WARN):
            DBG_LOGWARN(str);
            break;
        case static_cast<int>(DBG_LOG_ERROR):
            DBG_LOGERROR(str);
            break;
        default:
            DBG_LOGERROR(str);
    }
}

int UbseMemAdapter::GetLeaseMemoryStage(const std::string &name, bool isNuma, ubs_mem_stage &state)
{
    if (isNuma) {
        if (pUbseMemNumaGet == nullptr) {
            DBG_LOGERROR("pUbseMemNumaGet is nullptr");
            return MXM_ERR_UBSE_LIB;
        }
        ubs_mem_numa_desc_t desc;
        auto ret = pUbseMemNumaGet(name.c_str(), &desc);
        if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
            DBG_LOGINFO("Lease memory not exist. name=" << name);
            return MXM_ERR_LEASE_NOT_EXIST;
        }
        if (ret != 0) {
            DBG_LOGERROR("pUbseMemNumaGet failed. ret=" << ret << " name=" << name);
            return MXM_ERR_UBSE_INNER;
        }
        state = desc.mem_stage;
        return MXM_OK;
    }
    if (pUbseMemFdGet == nullptr) {
        DBG_LOGERROR("pUbseMemFdGet is nullptr");
        return MXM_ERR_UBSE_LIB;
    }
    ubs_mem_fd_desc_t desc;
    auto ret = pUbseMemFdGet(name.c_str(), &desc);
    if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
        DBG_LOGINFO("Lease memory not exit. name=" << name);
        return MXM_ERR_LEASE_NOT_EXIST;
    }
    if (ret != 0) {
        DBG_LOGERROR("pUbseMemFdGet failed. ret=" << ret << " name=" << name);
        return MXM_ERR_UBSE_INNER;
    }
    state = desc.mem_stage;
    return MXM_OK;
}

int UbseMemAdapter::GetShareMemoryStage(const std::string &name, ubs_mem_stage &state)
{
    if (pUbseMemShmGet == nullptr || pUbseNodeLocalGet == nullptr) {
        DBG_LOGERROR("pUbseMemShmGet or pUbseNodeLocalGet is nullptr");
        return MXM_ERR_UBSE_LIB;
    }
    ubs_topo_node_t localNode{};
    auto ret = pUbseNodeLocalGet(&localNode);
    if (ret != 0) {
        DBG_LOGERROR("pUbseNodeLocalGet failed, ret=" << ret);
        return MXM_ERR_UBSE_INNER;
    }
    ubs_mem_shm_desc_t *desc = nullptr;
    ret = pUbseMemShmGet(name.c_str(), &desc);
    if (ret == UBS_ENGINE_ERR_NOT_EXIST) {
        DBG_LOGINFO("Share memory not exist. name=" << name);
        return MXM_ERR_SHM_NOT_EXIST;
    }
    if (ret != 0) {
        DBG_LOGERROR("pUbseMemShmGet failed. ret=" << ret << " name=" << name);
        return MXM_ERR_UBSE_INNER;
    }
    if (desc == nullptr) {
        DBG_LOGERROR("pUbseMemShmGet desc is nullptr.");
        return MXM_ERR_UBSE_INNER;
    }
    if (desc->import_desc_cnt == 0) {
        DBG_LOGINFO("Share memory not attached. The import nodes are empty. name=" << name);
        FreeShmDesc(desc);
        return MXM_ERR_SHM_NOT_EXIST;
    }
    bool localAttached = false;
    for (uint32_t i = 0; i < desc->import_desc_cnt; i++) {
        if (desc->import_desc[i].import_node.slot_id == localNode.slot_id) {
            localAttached = true;
            state = desc->import_desc[i].mem_stage;
            break;
        }
    }
    if (!localAttached) {
        DBG_LOGINFO("Share memory not attached. name=" << name);
        state = UBSE_NOT_EXIST;
    }
    FreeShmDesc(desc);
    return MXM_OK;
}
int UbseMemAdapter::GetLocalNodeId(uint32_t &nid)
{
    if (!initialized_) {
        DBG_LOGERROR("UbseMemExecutor not initialized");
        return MXM_ERR_UBSE_LIB;
    }
    if (pUbseNodeLocalGet == nullptr) {
        DBG_LOGERROR("pUbseNodeLocalGet is nullptr");
        return MXM_ERR_UBSE_LIB;
    }
    ubs_topo_node_t localNode;
    auto ret = pUbseNodeLocalGet(&localNode);
    if (ret != UBS_SUCCESS) {
        DBG_LOGERROR("Get local node failed ret is " << ret);
        return MXM_ERR_UBSE_INNER;
    }
    nid = localNode.slot_id;
    return HOK;
};

int UbseMemAdapter::EnsureGetLocalNodeId(uint32_t &nid)
{
    if (nodeId_ != UINT32_MAX) {
        nid = nodeId_;
        return HOK;
    }
    uint32_t tmpNodeId = UINT32_MAX;
    auto ret = GetLocalNodeId(tmpNodeId);
    if (ret != HOK) {
        DBG_LOGERROR("GetLocalNodeId failed. ret=" << ret);
        return ret;
    }
    nodeId_.store(tmpNodeId);
    nid = tmpNodeId;
    return HOK;
}
} // namespace mxm
} // namespace ock