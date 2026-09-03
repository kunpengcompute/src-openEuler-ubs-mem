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

#include "ShmMetaDataMgr.h"
#include "ubs_mem_def.h"
#include "log.h"
#include "rack_mem_err.h"
#include "system_adapter.h"

namespace ock::mxmd {
using namespace ock::common;
using namespace ock::ubsm;
uint32_t ShmMetaDataMgr::Init()
{
    return UBSM_OK;
}

uint32_t ShmMetaDataMgr::Destroy()
{
    return UBSM_OK;
}

void *ShmMetaDataMgr::GetAddr(const std::string &name)
{
    const auto key = StrToSegment(name);
    Locker<Lock> spinLocker(&mNameLock[key]);
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        return nullptr;
    }
    return find->second.addr;
}

uint32_t GetSegmentsAndFirstUsage(ShmAppMetaData &shmAppMetaData, void *checkAddr, uint64_t checkLen,
                                  uint64_t &usedFirst, std::vector<int> &indices)
{
    if (checkAddr == nullptr || checkLen == 0) {
        DBG_LOGERROR("Invalid param, checkLen=" << checkLen);
        return MXM_ERR_PARAM_INVALID;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(shmAppMetaData.addr);
    uintptr_t queryStart = reinterpret_cast<uintptr_t>(checkAddr);
    uintptr_t queryEnd = queryStart + checkLen;
    // 确保 queryStart >= base，避免偏移量过大
    if (queryStart < base) {
        DBG_LOGERROR("queryStart < base, invalid param, start " << queryStart << ", base " << base);
        return MXM_ERR_PARAM_INVALID;
    }
    if (((queryStart - base) % FOUR_KB) != 0) {
        DBG_LOGERROR("queryStart not align 4kb " << queryStart << ", end " << queryEnd);
        return MXM_ERR_PARAM_INVALID;
    }
    // 检查 queryStart + checkLen 是否溢出
    if (std::numeric_limits<uintptr_t>::max() - queryStart < checkLen) {
        DBG_LOGERROR("queryStart + checkLen overflow");
        return MXM_ERR_PARAM_INVALID;
    }

    if (queryEnd < queryStart) {
        DBG_LOGERROR("queryEnd < queryStart, invalid param, start " << queryStart << ", end " << queryEnd);
        return MXM_ERR_PARAM_INVALID;
    }

    if (queryEnd > base + shmAppMetaData.mapSize) {
        DBG_LOGERROR("queryEnd < base + mapSize, invalid param, start " << queryStart << ", end " << queryEnd
                                                                        << ", base " << base << ", mapSize "
                                                                        << shmAppMetaData.mapSize);
        return MXM_ERR_PARAM_INVALID;
    }

    if (shmAppMetaData.unitSize == 0) {
        DBG_LOGERROR("shmAppMetaData.unitSize is zero.");
        return MXM_ERR_PARAM_INVALID;
    }

    // 起始段索引
    int64_t startIdx = static_cast<int64_t>((queryStart - base) / shmAppMetaData.unitSize);
    // 结束段索引 (query_end 是开区间)
    int64_t endIdx = static_cast<int64_t>((queryEnd - 1 - base) / shmAppMetaData.unitSize);

    indices.reserve(endIdx - startIdx + 1); // 预分配空间，提高效率
    for (int i = startIdx; i <= endIdx; ++i) {
        indices.push_back(i);
    }

    usedFirst =
        shmAppMetaData.unitSize + reinterpret_cast<uintptr_t>(shmAppMetaData.addrs[startIdx].first) - queryStart;
    usedFirst = (startIdx == endIdx) ? checkLen : usedFirst;
    return UBSM_OK;
}

int32_t ShmMetaDataMgr::CheckAddr(std::string name, void *start, size_t length, ShmAppMetaData &shmAppMetaData,
                                  uint64_t &usedFirst, std::vector<int> &indices)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto findMeta = mAppShmInfoMap[key].find(name);
    if (findMeta == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("Name meta data not exist, name=" << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    shmAppMetaData = findMeta->second;

    auto res = GetSegmentsAndFirstUsage(shmAppMetaData, start, length, usedFirst, indices);
    if (res == 0) {
        mNameLock[key].UnLock();
        return UBSM_OK;
    }
    mNameLock[key].UnLock();
    DBG_LOGERROR("CheckAddr failed, name=" << name);
    return MXM_ERR_PARAM_INVALID;
}

int32_t ShmMetaDataMgr::CheckNameExistAndGet(void *start, size_t mapSize, ShmAppMetaData &shmAppMetaData)
{
    if (start == nullptr) {
        DBG_LOGERROR("Param start is nullptr.");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto pkey = PtrToSegment(start);
    mPtrLock[pkey].DoLock();
    const auto find = mAddr2NameMap[pkey].find(start);
    if (find == mAddr2NameMap[pkey].end()) {
        mPtrLock[pkey].UnLock();
        DBG_LOGERROR("Name not exist");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    std::string name = find->second;
    mPtrLock[pkey].UnLock();

    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto findMeta = mAppShmInfoMap[key].find(name);
    if (findMeta == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("Name meta data not exist, name=" << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    if (findMeta->second.mapSize != mapSize) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("Map size not match, name=" << name << " lenth=" << mapSize
                                                 << " found mapSize=" << findMeta->second.mapSize);
        return MXM_ERR_PARAM_INVALID;
    }
    shmAppMetaData = findMeta->second;
    mNameLock[key].UnLock();
    return UBSM_OK;
}

int32_t ShmMetaDataMgr::GetShmMetaFromName(const std::string &name, ShmAppMetaData &shmAppMetaData)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto findMeta = mAppShmInfoMap[key].find(name);
    if (findMeta == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("Name meta data not exist, name=" << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    shmAppMetaData = findMeta->second;
    mNameLock[key].UnLock();
    return UBSM_OK;
}

int32_t ShmMetaDataMgr::SetMetaHasUnmapped(const std::string &name, bool val)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("SetMetaHasUnmapped error. Name not exist " << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    find->second.SetHasUnmapped(val);
    mNameLock[key].UnLock();
    return UBSM_OK;
}

int32_t ShmMetaDataMgr::SetMetaIsLockAddress(const std::string &name, bool val)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("SetMetaIsLockAddress error. Name not exist " << name);
        return MXM_ERR_SHM_NOT_FOUND;
    }
    find->second.SetisLockAddress(val);
    mNameLock[key].UnLock();
    return UBSM_OK;
}

bool ShmMetaDataMgr::GetMetaHasUnmapped(const std::string &name)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("GetMetaHasUnmapped error. Name not exist " << name);
        return true;
    }
    auto hasUnmap = find->second.GetHasUnmapped();
    mNameLock[key].UnLock();
    return hasUnmap;
}

bool ShmMetaDataMgr::GetMetaIsLockAddress(const std::string &name)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGWARN("GetMetaIsLockAddress not find name " << name);
        return false;
    }
    auto isLock = find->second.GetisLockAddress();
    mNameLock[key].UnLock();
    return isLock;
}

uint32_t ShmMetaDataMgr::GetAllUsedMemoryNames(std::vector<std::string> &names)
{
    names.clear();
    for (size_t index = 0; index < SEGMENT_COUNT; ++index) {
        mNameLock[index].DoLock();
        for (const auto &[fst, snd] : this->mAppShmInfoMap[index]) {
            names.push_back(fst);
        }
        mNameLock[index].UnLock();
    }
    return MXM_OK;
}

bool ShmMetaDataMgr::HasOverlapWithKnownVmas(uintptr_t start, uintptr_t end)
{
    ReadLocker<ReadWriteLock> locker(&mVmasLock);

    if (mMappedMemoryAddress.empty()) {
        return false;
    }

    const auto firstIterGreaterThanStart = mMappedMemoryAddress.upper_bound(start); // 二分查找 O(log(N))
    if (firstIterGreaterThanStart != mMappedMemoryAddress.begin()) {
        auto prevIter = std::prev(firstIterGreaterThanStart);
        uintptr_t prevVmaStart = prevIter->first;
        uintptr_t prevVmaEnd = prevIter->second;

        if (start < prevVmaEnd && end > prevVmaStart) {
            return true;
        }
    }

    if (firstIterGreaterThanStart != mMappedMemoryAddress.end()) {
        uintptr_t VmaStart = firstIterGreaterThanStart->first;
        uintptr_t VmaEnd = firstIterGreaterThanStart->second;

        if (start < VmaEnd && VmaStart < end) {
            return true;
        }
    }

    return false;
}

int32_t ShmMetaDataMgr::AddMappedMemoryRange(void *start, size_t length)
{
    auto VmaStart = reinterpret_cast<uintptr_t>(start);
    if (length == 0) {
        DBG_LOGERROR("length=0 is invalid.");
        return MXM_ERR_PARAM_INVALID;
    }
    if (UNLIKELY(length > std::numeric_limits<uintptr_t>::max() - VmaStart)) {
        DBG_LOGERROR("Address is too long, length=" << length);
        return MXM_ERR_PARAM_INVALID; // 溢出
    }
    WriteLocker<ReadWriteLock> locker(&mVmasLock);
    if (mMappedMemoryAddress.find(VmaStart) != mMappedMemoryAddress.end()) {
        DBG_LOGWARN("Address already mapped.");
        mMappedMemoryAddress[VmaStart] = VmaStart + length;
    } else {
        mMappedMemoryAddress.emplace(VmaStart, VmaStart + length);
    }
    return MXM_OK;
}

int32_t ShmMetaDataMgr::RemoveMappedMemoryRange(void *start)
{
    auto VmaStart = reinterpret_cast<uintptr_t>(start);
    WriteLocker<ReadWriteLock> locker(&mVmasLock);
    if (mMappedMemoryAddress.find(VmaStart) == mMappedMemoryAddress.end()) {
        DBG_LOGERROR("Address not found.");
        return MXM_ERR_SHM_NOT_FOUND;
    }
    mMappedMemoryAddress.erase(VmaStart);
    return MXM_OK;
}

uint32_t ShmMetaDataMgr::RemoveMetaData(void *start, const std::string &name)
{
    if (start == nullptr) {
        DBG_LOGERROR("Param start is nullptr.");
        return MXM_ERR_NULLPTR;
    }
    if (name.empty()) {
        DBG_LOGERROR("Param error");
        return MXM_ERR_PARAM_INVALID;
    }
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mNameLock[key].UnLock();
        DBG_LOGERROR("Name not exist " << name);
        return MXM_ERR_MEMLIB;
    }
    auto shmMapMetaData = find->second;

    mAppShmInfoMap[key].erase(name);
    mNameLock[key].UnLock();

    const auto pkey = PtrToSegment(start);
    mPtrLock[pkey].DoLock();
    mAddr2NameMap[pkey].erase(start);
    mPtrLock[pkey].UnLock();
    DBG_LOGINFO("WhenUnMap end name " << name);
    auto ret = RemoveMappedMemoryRange(start);
    if (ret != MXM_OK) {
        DBG_LOGERROR("RemoveMappedMemoryRange failed: " << ret);
        return ret;
    }
    return UBSM_OK;
}

uint32_t ShmMetaDataMgr::AddMetaData(const std::string &name, void *mAddr, ShmAppMetaData &shmAppMetaData)
{
    if (mAddr == nullptr) {
        DBG_LOGERROR("Param mAddr is nullptr.");
        return MXM_ERR_NULLPTR;
    }
    auto ret = AddMappedMemoryRange(mAddr, shmAppMetaData.mapSize);
    if (ret != MXM_OK) {
        DBG_LOGERROR("AddMappedMemoryRange failed: " << ret);
        return ret;
    }
    DBG_LOGDEBUG("Adding meta data, name=" << name);
    const auto key = StrToSegment(name);
    const auto pkey = PtrToSegment(mAddr);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        mAppShmInfoMap[key].emplace(name, shmAppMetaData);
    } else {
        DBG_LOGWARN("AddMetaDataAfterMap name exist! name:" << name);
        mAppShmInfoMap[key][name] = shmAppMetaData;
    }
    mNameLock[key].UnLock();

    mPtrLock[pkey].DoLock();
    mAddr2NameMap[pkey][mAddr] = name;
    mPtrLock[pkey].UnLock();
    DBG_LOGDEBUG("Adding meta data successfully, name=" << name << ", data=" << shmAppMetaData);
    return UBSM_OK;
}

int32_t ShmMetaDataMgr::UpdateMetaData(const std::string &name, const ShmAppMetaData &shmAppMetaData)
{
    const auto key = StrToSegment(name);
    mNameLock[key].DoLock();
    const auto find = mAppShmInfoMap[key].find(name);
    if (find == mAppShmInfoMap[key].end()) {
        DBG_LOGERROR("[UpdateMetaData SHM META] failed, name=" << name);
        mNameLock[key].UnLock();
        return MXM_ERR_SHM_NOT_FOUND;
    } else {
        mAppShmInfoMap[key][name] = shmAppMetaData;
    }
    mNameLock[key].UnLock();
    DBG_LOGDEBUG("[UpdateMetaData SHM META] " << shmAppMetaData);
    return UBSM_OK;
}

} // namespace ock::mxmd
  // ock