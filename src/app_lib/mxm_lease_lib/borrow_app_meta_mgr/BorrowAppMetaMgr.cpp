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
#include "BorrowAppMetaMgr.h"
#include "ubs_mem_def.h"
#include "log.h"

namespace ock::mxmd {
using namespace ock::common;
uint32_t BorrowAppMetaMgr::AddMeta(const void *addr, const AppBorrowMetaDesc &desc)
{
    if (addr == nullptr) {
        DBG_LOGERROR("Param is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        appBorrowInfoMap[key].emplace(addr, desc);
    } else {
        appBorrowInfoMap[key][addr] = desc;
        DBG_LOGERROR("addr exist !");
    }
    mapLock[key].UnLock();
    return UBSM_OK;
}

int32_t BorrowAppMetaMgr::SetMetaHasUnmapped(const void *addr, bool val)
{
    if (addr == nullptr) {
        DBG_LOGERROR("SetMetaHasUnmapped error. addr is null");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("SetMetaHasUnmapped error. Not exist meta");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    find->second.SetHasUnmapped(val);
    mapLock[key].UnLock();
    return UBSM_OK;
}

int32_t BorrowAppMetaMgr::SetMetaIsLockAddress(const void *addr, bool val)
{
    if (addr == nullptr) {
        DBG_LOGERROR("SetMetaIsLockAddress error. addr is null");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("SetMetaIsLockAddress error. Not exist meta");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    find->second.SetisLockAddress(val);
    mapLock[key].UnLock();
    return UBSM_OK;
}

bool BorrowAppMetaMgr::GetMetaHasUnmapped(const void *addr)
{
    if (addr == nullptr) {
        DBG_LOGERROR("GetMetaHasUnmapped error. addr is null");
        return false;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("GetMetaHasUnmapped error. Not exist meta");
        return false;
    }
    auto hasUnmap = find->second.GetHasUnmapped();
    mapLock[key].UnLock();
    return hasUnmap;
}
bool BorrowAppMetaMgr::GetMetaIsLockAddress(const void *addr)
{
    if (addr == nullptr) {
        DBG_LOGERROR("GetMetaIsLockAddress error. addr is null");
        return false;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGINFO("GetMetaIsLockAddress Not exist meta");
        return false;
    }
    auto isLock = find->second.GetisLockAddress();
    mapLock[key].UnLock();
    return isLock;
}

uint32_t BorrowAppMetaMgr::GetAllUsedMemoryNames(std::vector<std::string> &names)
{
    names.clear();
    for (size_t index = 0; index < SEGMENT_COUNT; ++index) {
        mapLock[index].DoLock();
        for (const auto &[fst, snd] : appBorrowInfoMap[index]) {
            names.push_back(snd.GetName());
        }
        mapLock[index].UnLock();
    }
    return MXM_OK;
}

uint32_t BorrowAppMetaMgr::RemoveMeta(const void *addr)
{
    if (addr == nullptr) {
        DBG_LOGERROR("Param is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("Not exist meta");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    appBorrowInfoMap[key].erase(addr);
    mapLock[key].UnLock();
    return UBSM_OK;
}

uint32_t BorrowAppMetaMgr::GetMeta(const void *addr, AppBorrowMetaDesc &desc)
{
    if (addr == nullptr) {
        DBG_LOGERROR("Param is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    const auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("The addr is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    desc = find->second;
    mapLock[key].UnLock();
    if (desc.GetName().empty()) {
        DBG_LOGERROR("borrow name is empty.");
        return MXM_ERR_PARAM_INVALID;
    }
    return UBSM_OK;
}

uint32_t BorrowAppMetaMgr::UpdateMeta(const void *addr, AppBorrowMetaDesc &desc)
{
    if (addr == nullptr) {
        DBG_LOGERROR("Param is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = PtrToSegment(addr);
    mapLock[key].DoLock();
    auto find = appBorrowInfoMap[key].find(addr);
    if (find == appBorrowInfoMap[key].end()) {
        mapLock[key].UnLock();
        DBG_LOGERROR("The addr is not exist.");
        return MXM_ERR_LEASE_NOT_FOUND;
    }
    find->second = desc;
    mapLock[key].UnLock();
    DBG_LOGDEBUG("Update meta of borrow memory, name=" << desc.GetName());
    return UBSM_OK;
}

} // namespace ock::mxmd
  // ock