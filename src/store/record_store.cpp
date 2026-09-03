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
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include <algorithm>

#include <securec.h>
#include "log.h"
#include "record_store.h"

namespace ock {
namespace ubsm {

namespace {
template <class K, class V>
bool EraseLastMultiValue(std::unordered_map<K, std::vector<V>> &map, const K &k, V &v)
{
    auto pos = map.find(k);
    if (pos == map.end()) {
        return false;
    }

    if (pos->second.empty()) {
        map.erase(pos);
        return false;
    }

    v = pos->second[pos->second.size() - 1U];
    pos->second.resize(pos->second.size() - 1U);
    if (pos->second.empty()) {
        map.erase(pos);
    }
    return true;
}

template <class RecordType>
std::shared_ptr<BaseRecordAllocator<RecordType>> CreateAllocator(RecordType *base, uint32_t capacity,
                                                                 const std::function<bool(const RecordType &)> &idleFun,
                                                                 const std::function<void(RecordType &)> &clearFun)
{
    return std::make_shared<BaseRecordAllocator<RecordType>>(base, capacity, idleFun, clearFun);
}
} // namespace

RecordStore::RecordStore() noexcept : shmFd_{-1} {}

RecordStore::~RecordStore() noexcept
{
    Destroy();
}

RecordStore &RecordStore::GetInstance() noexcept
{
    static RecordStore instance;
    return instance;
}

bool RecordStore::CheckAllocators()
{
    regionAllocator_ = CreateAllocator<RegionRecord>(regionRecordBegin_, REGION_MAX_RECORD, RegionIdle, ClearRegion);
    memLeaseAllocator_ =
        CreateAllocator<MemLeaseRecord>(memLeaseRecordBegin_, MEM_LEASE_MAX_RECORD, MemLeaseIdle, ClearMemLease);
    shmImportAllocator_ = CreateAllocator<MemShareImportRecord>(shmImportRecordBegin_, SHM_MAX_ATTACH_RECORD,
                                                                MemShareImportIdle, ClearMemShareImport);
    shmRefAllocator_ = CreateAllocator<MemShareRefRecord>(shmRefRecordBegin_, SHM_MAX_REFERENCE_RECORD, MemShareRefIdle,
                                                          ClearMemShareRef);

    // 检查分配器是否创建成功
    if (!regionAllocator_ || !memLeaseAllocator_ || !shmImportAllocator_ || !shmRefAllocator_) {
        DBG_LOGERROR("Failed to create allocators");
        return false;
    }
    return true;
}

int RecordStore::Initialize(int fd) noexcept
{
    if (fd < 0) {
        DBG_LOGERROR("share mem fd invalid!");
        return -1;
    }

    shmFd_ = fd;
    auto ret = ftruncate(shmFd_, SHARE_MEM_SIZE);
    if (ret != 0) {
        DBG_LOGERROR("ftruncate failed: " << strerror(errno));
        return -1;
    }

    auto addr = mmap(nullptr, SHARE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd_, 0);
    if (addr == MAP_FAILED) {
        DBG_LOGERROR("mmap failed: " << strerror(errno));
        return -1;
    }

    uint64_t magic = 0;
    auto pm = static_cast<uint64_t *>(addr);
    for (auto p = 0U; p < SHM_PAGE_COUNT; p++) {
        magic ^= *(pm + p * SHM_PAGE_SIZE / sizeof(uint64_t));
    }
    DBG_LOGDEBUG("page magic = " << magic);

    mappingAddress_ = addr;
    regionRecordBegin_ = reinterpret_cast<RegionRecord *>(mappingAddress_);
    memLeaseRecordBegin_ = reinterpret_cast<MemLeaseRecord *>(regionRecordBegin_ + REGION_MAX_RECORD);
    shmImportRecordBegin_ = reinterpret_cast<MemShareImportRecord *>(memLeaseRecordBegin_ + MEM_LEASE_MAX_RECORD);
    shmRefRecordBegin_ = reinterpret_cast<MemShareRefRecord *>(shmImportRecordBegin_ + SHM_MAX_ATTACH_RECORD);
    memIdRecordPoolBegin_ = reinterpret_cast<MemIdRecordPool *>(shmRefRecordBegin_ + SHM_MAX_REFERENCE_RECORD);
    createSeqNo_ = reinterpret_cast<uint32_t *>(memIdRecordPoolBegin_ + 1);

    if (!CheckAllocators()) {
        DBG_LOGERROR("CheckAllocators failed.");
        munmap(mappingAddress_, SHARE_MEM_SIZE);
        mappingAddress_ = nullptr;
        return -1;
    }

    ret = poolAllocator_.Initialize(memIdRecordPoolBegin_);
    if (ret != 0) {
        DBG_LOGERROR("pool allocator initialized failed: " << ret);
        munmap(mappingAddress_, SHARE_MEM_SIZE);
        mappingAddress_ = nullptr;
        return -1;
    }

    Restore();
    return 0;
}

void RecordStore::Destroy() noexcept
{
    if (mappingAddress_ == nullptr) {
        return;
    }

    poolAllocator_.Destroy();
    cachedRegionRecords_.clear();
    cachedLeaseRecords_.clear();
    cachedImportShmRecords_.clear();
    cachedRefShmIdxRecords_.clear();
    cachedRefShmPidRecords_.clear();

    munmap(mappingAddress_, SHARE_MEM_SIZE);
    mappingAddress_ = nullptr;
    regionRecordBegin_ = nullptr;
    memLeaseRecordBegin_ = nullptr;
    shmImportRecordBegin_ = nullptr;
    shmRefRecordBegin_ = nullptr;
    memIdRecordPoolBegin_ = nullptr;
    createSeqNo_ = nullptr;
}

int RecordStore::AddRegionRecord(const CreateRegionInput &input) noexcept
{
    DBG_LOGINFO("Add region name(" << input.name << ").");

    if (input.name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input region name(" << input.name << ") too long.");
        return -1;
    }

    if (std::any_of(input.nodeIds.begin(), input.nodeIds.end(),
                    [](uint32_t nid) { return nid >= REGION_NODE_BITMAP_COUNT; })) {
        DBG_LOGERROR("input region name(" << input.name << ") node id invalid.");
        return -1;
    }

    if (input.nodeIds.size() != input.affinity.size()) {
        DBG_LOGERROR("input region param error, node's size not equal affinity's size.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    auto region = regionAllocator_->Allocate();
    if (region == nullptr) {
        DBG_LOGERROR("allocate record for region failed!");
        return -1;
    }

    auto sRet = strcpy_s(region->name, RECORD_NAME_SIZE, input.name.c_str());
    if (sRet != EOK) {
        regionAllocator_->Release(region);
        DBG_LOGERROR("copy region name(" << input.name << ") failed: " << sRet);
        return -1;
    }

    region->size = input.size;
    sRet = memset_s(region->bitmap, sizeof(region->bitmap), 0, sizeof(region->bitmap));
    if (sRet != EOK) {
        regionAllocator_->Release(region);
        DBG_LOGERROR("memset for region nodes bitmap(" << input.name << ") failed: " << sRet);
        return -1;
    }

    for (size_t i = 0; i < input.nodeIds.size(); ++i) {
        auto index = input.nodeIds[i] / U64_BITS_COUNT;
        auto bits = input.nodeIds[i] % U64_BITS_COUNT;
        region->bitmap[index] |= (1UL << bits);
        if (input.affinity[i]) {
            region->bitmapAffinity[index] |= (1UL << bits);
        }
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedRegionRecords_.find(input.name);
    if (pos != cachedRegionRecords_.end()) {
        uniqueLock.unlock();
        regionAllocator_->Release(region);
        DBG_LOGERROR("input region name(" << input.name << ") already exist!");
        return -1;
    }

    cachedRegionRecords_.emplace(input.name, region);
    return 0;
}

int RecordStore::DelRegionRecord(const std::string &name) noexcept
{
    DBG_LOGINFO("Delele regord of region name(" << name << ").");

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedRegionRecords_.find(name);
    if (pos == cachedRegionRecords_.end()) {
        DBG_LOGERROR("input region name(" << name << ") is not exist.");
        return -1;
    }

    auto record = pos->second;
    cachedRegionRecords_.erase(pos);
    uniqueLock.unlock();

    regionAllocator_->Release(record);
    return 0;
}

std::vector<CreateRegionInput> RecordStore::ListRegionRecord() const noexcept
{
    std::vector<CreateRegionInput> result;

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto backup = cachedRegionRecords_;
    uniqueLock.unlock();

    for (auto &it : backup) {
        std::vector<uint32_t> nodeIds;
        std::vector<bool> affinitys;
        for (auto index = 0U; index < REGION_NODE_BITMAP_U64SIZE; index++) {
            auto bitmap = it.second->bitmap[index];
            auto bitmapAffinity = it.second->bitmapAffinity[index];
            auto base = index * U64_BITS_COUNT;
            while (bitmap != 0UL) {
                auto bit = __builtin_ffsl(bitmap) - 1U;
                nodeIds.emplace_back(base + bit);
                bool affinity = (bitmapAffinity >> bit) & 1UL;
                affinitys.emplace_back(affinity);
                bitmap &= (~(1UL << bit));
            }
        }

        DBG_LOGINFO("List region name(" << it.first << ").");
        result.emplace_back(CreateRegionInput(it.first, it.second->size, nodeIds, affinitys));
    }

    return result;
}

int RecordStore::AddMemLeaseRecord(const MemLeaseInfo &info) noexcept
{
    if (info.first.name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input lease memory name(" << info.first.name << ") too long.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    auto record = memLeaseAllocator_->Allocate();
    if (record == nullptr) {
        DBG_LOGERROR("allocate record for MemoryLease failed!");
        return -1;
    }

    uint32_t memIdHeadIndex = 0UL;
    auto ret = poolAllocator_.Allocate(info.second.memIds, memIdHeadIndex);
    if (ret != 0) {
        DBG_LOGERROR("allocate record for mem id pool with size:" << info.second.memIds.size() << " failed!");
        memLeaseAllocator_->Release(record);
        return -1;
    }

    auto sRet = strcpy_s(record->name, RECORD_NAME_SIZE, info.first.name.c_str());
    if (sRet != EOK) {
        memLeaseAllocator_->Release(record);
        poolAllocator_.Release(memIdHeadIndex);
        DBG_LOGERROR("copy memory lease name(" << info.first.name << ") failed: " << sRet);
        return -1;
    }

    record->size = info.first.size;
    record->pid = info.first.appContext.pid;
    record->uid = info.first.appContext.uid;
    record->gid = info.first.appContext.gid;
    record->fdMode = info.first.fdMode ? 1U : 0U;
    record->distance = info.first.distance;
    record->numaId = info.second.numaId;
    record->unitSize = info.second.unitSize;
    record->memIdCount = info.second.memIds.size();
    record->slotId = info.second.slotId;
    record->memIdBuffer = memIdHeadIndex;

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(info.first.name);
    if (pos != cachedLeaseRecords_.end()) {
        uniqueLock.unlock();
        memLeaseAllocator_->Release(record);
        poolAllocator_.Release(memIdHeadIndex);
        DBG_LOGERROR("input mem lease name(" << info.first.name << ") already exist!");
        return -1;
    }

    cachedLeaseRecords_.emplace(info.first.name, WithMemIdsRecord<MemLeaseRecord>{record, info.second.memIds});
    return 0;
}

int RecordStore::DelMemLeaseRecord(const std::string &name) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(name);
    if (pos == cachedLeaseRecords_.end()) {
        uniqueLock.unlock();
        DBG_LOGERROR("input LeaseMem name: " << name << " not exist.");
        return -1;
    }

    auto record = pos->second.record;
    record->name[0] = '\0';
    cachedLeaseRecords_.erase(pos);
    uniqueLock.unlock();

    auto memIdHeadIndex = record->memIdBuffer;
    memLeaseAllocator_->Release(record);
    poolAllocator_.Release(memIdHeadIndex);
    return 0;
}

int RecordStore::AddMemLeaseInput(const LeaseMallocInput &input) noexcept
{
    if (input.name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input lease memory name(" << input.name << ") too long.");
        return -1;
    }
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }
    auto record = memLeaseAllocator_->Allocate();
    if (record == nullptr) {
        DBG_LOGERROR("allocate record for MemoryLease failed!");
        return -1;
    }
    auto sRet = strcpy_s(record->name, RECORD_NAME_SIZE, input.name.c_str());
    if (sRet != EOK) {
        memLeaseAllocator_->Release(record);
        DBG_LOGERROR("copy memory lease name(" << input.name << ") failed: " << sRet);
        return -1;
    }
    record->size = input.size;
    record->pid = input.appContext.pid;
    record->uid = input.appContext.uid;
    record->gid = input.appContext.gid;
    record->fdMode = input.fdMode ? 1U : 0U;
    record->distance = input.distance;
    record->memIdCount = 0;
    record->memIdBuffer = 0;
    record->recordState = input.state;
    record->numaId = -1;

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(input.name);
    if (pos != cachedLeaseRecords_.end()) {
        uniqueLock.unlock();
        memLeaseAllocator_->Release(record);
        DBG_LOGERROR("input mem lease name(" << input.name << ") already exist!");
        return -1;
    }

    cachedLeaseRecords_.emplace(input.name, WithMemIdsRecord<MemLeaseRecord>{record});
    return 0;
}

int RecordStore::AddMemLeaseResult(const std::string &name, const LeaseMallocResult &result) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(name);
    if (pos == cachedLeaseRecords_.end()) {
        uniqueLock.unlock();
        DBG_LOGERROR("input LeaseMem name: " << name << " not exist.");
        return -1;
    }

    uint32_t memIdHeadIndex = 0UL;
    auto ret = poolAllocator_.Allocate(result.memIds, memIdHeadIndex);
    if (ret != 0) {
        DBG_LOGERROR("allocate record for mem id pool with size:" << result.memIds.size() << " failed!");
        return -1;
    }
    auto record = pos->second.record;
    record->recordState = RecordState::FINISH;
    record->memIdCount = result.memIds.size();
    record->memIdBuffer = memIdHeadIndex;
    record->numaId = result.numaId;
    record->unitSize = result.unitSize;
    record->slotId = result.slotId;
    pos->second.memIds.assign(result.memIds.begin(), result.memIds.end());
    return 0;
}

int RecordStore::UpdateMemLeaseRecordState(const std::string &name, RecordState state) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }
    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(name);
    if (pos == cachedLeaseRecords_.end()) {
        DBG_LOGERROR("input LeaseMem name: " << name << " not exist!");
        return -1;
    }
    auto record = pos->second.record;
    record->recordState = state;

    return 0;
}

std::vector<MemLeaseInfo> RecordStore::ListMemLeaseRecord() const noexcept
{
    std::vector<MemLeaseInfo> result;
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return result;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto backup = cachedLeaseRecords_;
    uniqueLock.unlock();

    for (auto &it : backup) {
        MemLeaseInfo info;
        ConvertMemLease(it.second, info);
        result.emplace_back(info);
    }

    return result;
}

int RecordStore::UpdateMemLeaseRecord(const std::string &name, const AppContext &context, uint64_t size) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedLeaseRecords_.find(name);
    if (pos == cachedLeaseRecords_.end()) {
        uniqueLock.unlock();
        DBG_LOGERROR("input LeaseMem name: " << name << " not exist.");
        return -1;
    }
    auto record = pos->second.record;
    record->pid = context.pid;
    record->uid = context.uid;
    record->gid = context.gid;
    record->size = size;

    return 0;
}

int RecordStore::AddShmImportRecord(const ock::ubsm::ShareMemImportInfo &info) noexcept
{
    if (info.name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input share memory import name(" << info.name << ") too long.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    auto record = shmImportAllocator_->Allocate();
    if (record == nullptr) {
        DBG_LOGERROR("allocate record for share memory import failed!");
        return -1;
    }

    uint32_t memIdHeadIndex = 0UL;
    auto ret = poolAllocator_.Allocate(info.memIds, memIdHeadIndex);
    if (ret != 0) {
        DBG_LOGERROR("allocate record for mem id pool with size:" << info.memIds.size() << " failed!");
        shmImportAllocator_->Release(record);
        return -1;
    }

    auto sRet = strcpy_s(record->name, RECORD_NAME_SIZE, info.name.c_str());
    if (sRet != EOK) {
        shmImportAllocator_->Release(record);
        DBG_LOGERROR("copy share memory import name(" << info.name << ") failed: " << sRet);
        return -1;
    }
    record->shmSize = info.size;
    record->pid = info.appContext.pid;
    record->uid = info.appContext.uid;
    record->gid = info.appContext.gid;
    record->unitSize = info.unitSize;
    record->memIdCount = info.memIds.size();
    record->memIdBuffer = memIdHeadIndex;
    record->flag = info.flag;
    record->oflag = info.oflag;

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedImportShmRecords_.find(info.name);
    if (pos != cachedImportShmRecords_.end()) {
        uniqueLock.unlock();
        shmImportAllocator_->Release(record);
        poolAllocator_.Release(memIdHeadIndex);
        DBG_LOGERROR("input share memory import name(" << info.name << ") already exist!");
        return -1;
    }

    cachedImportShmRecords_.emplace(info.name, WithMemIdsRecord<MemShareImportRecord>{record, info.memIds});
    return 0;
}

int RecordStore::DelShmImportRecord(const std::string &name) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedImportShmRecords_.find(name);
    if (pos == cachedImportShmRecords_.end()) {
        DBG_LOGERROR("input share memory import name: " << name << " not exist.");
        return -1;
    }

    auto record = pos->second.record;
    auto index = static_cast<uint32_t>(record - shmImportRecordBegin_);
    auto refPos = cachedRefShmIdxRecords_.find(index);
    if (refPos != cachedRefShmIdxRecords_.end()) {
        DBG_LOGERROR("input share memory import name: " << name << " referenced by applications.");
        return -1;
    }

    cachedImportShmRecords_.erase(pos);
    uniqueLock.unlock();

    auto memIdHeadIndex = record->memIdBuffer;
    shmImportAllocator_->Release(record);
    poolAllocator_.Release(memIdHeadIndex);
    return 0;
}

int RecordStore::AddShmImportInput(const ShareMemImportInput &input) noexcept
{
    if (input.name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input share memory import name(" << input.name << ") too long.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }
    auto record = shmImportAllocator_->Allocate();
    if (record == nullptr) {
        DBG_LOGERROR("allocate record for share memory import failed!");
        return -1;
    }
    auto sRet = strcpy_s(record->name, RECORD_NAME_SIZE, input.name.c_str());
    if (sRet != EOK) {
        shmImportAllocator_->Release(record);
        DBG_LOGERROR("copy share memory import name(" << input.name << ") failed: " << sRet);
        return -1;
    }

    record->shmSize = input.size;
    record->pid = input.pid;
    record->memIdCount = 0;
    record->memIdBuffer = 0;
    record->recordState = RecordState::PRE_ADD;

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedImportShmRecords_.find(input.name);
    if (pos != cachedImportShmRecords_.end()) {
        uniqueLock.unlock();
        shmImportAllocator_->Release(record);
        DBG_LOGERROR("input share memory import name(" << input.name << ") already exist!");
        return -1;
    }

    cachedImportShmRecords_.emplace(input.name, WithMemIdsRecord<MemShareImportRecord>{record});
    return 0;
}

int RecordStore::AddShmImportResult(const std::string &name, const ShareMemImportResult &result) noexcept
{
    if (name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input share memory import name(" << name << ") too long.");
        return -1;
    }
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }
    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedImportShmRecords_.find(name);
    if (pos == cachedImportShmRecords_.end()) {
        DBG_LOGERROR("input share memory import name: " << name << " not exist.");
        return -1;
    }
    auto record = pos->second.record;
    uint32_t memIdHeadIndex = 0UL;
    auto ret = poolAllocator_.Allocate(result.memIds, memIdHeadIndex);
    if (ret != 0) {
        DBG_LOGERROR("allocate record for mem id pool with size:" << result.memIds.size() << " failed!");
        return -1;
    }
    record->uid = result.ownerUserId;
    record->gid = result.ownerGroupId;
    record->memIdCount = result.memIds.size();
    record->memIdBuffer = memIdHeadIndex;
    record->recordState = RecordState::FINISH;
    record->unitSize = result.unitSize;
    record->oflag = result.openFlag;
    record->flag = result.createFlags;
    pos->second.memIds.assign(result.memIds.begin(), result.memIds.end());
    return 0;
}

int RecordStore::UpdateShmImportRecordState(const std::string &name, RecordState state) noexcept
{
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }
    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedImportShmRecords_.find(name);
    if (pos == cachedImportShmRecords_.end()) {
        DBG_LOGERROR("input share memory import name: " << name << " not exist!");
        return -1;
    }
    auto record = pos->second.record;
    record->recordState = state;
    return 0;
}

std::vector<ShareMemImportInfo> RecordStore::ListShmImportRecord() const noexcept
{
    std::vector<ShareMemImportInfo> result;
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return result;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto backup = cachedImportShmRecords_;
    uniqueLock.unlock();

    for (auto &it : backup) {
        ShareMemImportInfo info;
        info.name = it.first;
        info.size = it.second.record->shmSize;
        info.appContext.Fill(it.second.record->pid, it.second.record->uid, it.second.record->gid);
        info.unitSize = it.second.record->unitSize;
        info.memIds = it.second.memIds;
        info.flag = it.second.record->flag;
        info.oflag = it.second.record->oflag;
        info.state = it.second.record->recordState;
        result.emplace_back(info);
    }

    return result;
}

int RecordStore::AddShmRefRecord(pid_t pid, const std::string &name) noexcept
{
    if (pid == 0 || name.empty() || name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input pid: " << pid << ", name: " << name << " invalid.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    auto shm = shmRefAllocator_->Allocate();
    if (shm == nullptr) {
        DBG_LOGERROR("allocate share memory reference record failed!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto importPos = cachedImportShmRecords_.find(name);
    if (importPos == cachedImportShmRecords_.end()) {
        uniqueLock.unlock();
        shmRefAllocator_->Release(shm);
        DBG_LOGERROR("input share name(" << name << ") not imported.");
        return -1;
    }

    auto importRecord = importPos->second.record;
    auto index = static_cast<uint32_t>(importRecord - shmImportRecordBegin_);
    cachedRefShmIdxRecords_[index][pid].emplace_back(shm);
    cachedRefShmPidRecords_[pid][index].emplace_back(shm);
    shm->pid = pid;
    shm->shmImportIndex = index;

    return 0;
}

int RecordStore::DelShmRefRecord(pid_t pid, const std::string &name) noexcept
{
    if (pid == 0 || name.empty() || name.size() >= RECORD_NAME_SIZE) {
        DBG_LOGERROR("input pid: " << pid << ", name: " << name << " invalid.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto importPos = cachedImportShmRecords_.find(name);
    if (importPos == cachedImportShmRecords_.end()) {
        uniqueLock.unlock();
        DBG_LOGERROR("input share name(" << name << ") not imported.");
        return -1;
    }

    MemShareRefRecord *record = nullptr;
    auto importRecord = importPos->second.record;
    auto index = static_cast<uint32_t>(importRecord - shmImportRecordBegin_);
    auto indexPos = cachedRefShmIdxRecords_.find(index);
    if (indexPos != cachedRefShmIdxRecords_.end()) {
        EraseLastMultiValue(indexPos->second, pid, record);
        if (indexPos->second.empty()) {
            cachedRefShmIdxRecords_.erase(indexPos);
        }
    }

    auto pidPos = cachedRefShmPidRecords_.find(pid);
    if (pidPos != cachedRefShmPidRecords_.end()) {
        EraseLastMultiValue(pidPos->second, index, record);
        if (pidPos->second.empty()) {
            cachedRefShmPidRecords_.erase(pidPos);
        }
    }
    uniqueLock.unlock();

    if (record == nullptr) {
        DBG_LOGERROR("input share name(" << name << "), pid(" << pid << ") not exist.");
        return -1;
    }

    shmRefAllocator_->Release(record);
    return 0;
}

int RecordStore::DelShmRefRecordsByPid(pid_t pid) noexcept
{
    if (pid == 0) {
        DBG_LOGERROR("input pid: " << pid << " invalid.");
        return -1;
    }

    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return -1;
    }

    std::vector<MemShareRefRecord *> removedRecords;
    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    auto pos = cachedRefShmPidRecords_.find(pid);
    if (pos == cachedRefShmPidRecords_.end()) {
        return 0;
    }

    for (auto it = pos->second.begin(); it != pos->second.end(); ++it) {
        cachedRefShmIdxRecords_[it->first].erase(pid);
        if (cachedRefShmIdxRecords_[it->first].empty()) {
            cachedRefShmIdxRecords_.erase(it->first);
        }
        removedRecords.insert(removedRecords.end(), it->second.begin(), it->second.end());
    }
    cachedRefShmPidRecords_.erase(pos);
    uniqueLock.unlock();

    for (auto record : removedRecords) {
        shmRefAllocator_->Release(record);
    }

    return 0;
}

std::unordered_map<std::string, std::unordered_map<pid_t, uint32_t>> RecordStore::ListShmRefRecordNameF() const noexcept
{
    std::unordered_map<std::string, std::unordered_map<pid_t, uint32_t>> result;
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return result;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &it : cachedRefShmIdxRecords_) {
        auto importShm = shmImportRecordBegin_ + it.first;
        for (auto &ref : it.second) {
            result[importShm->name][ref.first] += ref.second.size();
        }
    }
    uniqueLock.unlock();

    return result;
}

std::unordered_map<pid_t, std::unordered_map<std::string, uint32_t>> RecordStore::ListShmRefRecordPidF() const noexcept
{
    std::unordered_map<pid_t, std::unordered_map<std::string, uint32_t>> result;
    if (mappingAddress_ == nullptr) {
        DBG_LOGERROR("not initialized!");
        return result;
    }

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &it : cachedRefShmPidRecords_) {
        for (auto &ref : it.second) {
            auto importShm = shmImportRecordBegin_ + ref.first;
            result[it.first][importShm->name] += ref.second.size();
        }
    }
    uniqueLock.unlock();

    return result;
}

bool RecordStore::RegionIdle(const RegionRecord &record) noexcept
{
    auto idle = (record.name[0] == '\0');
    return idle;
}

void RecordStore::ClearRegion(RegionRecord &record) noexcept
{
    record.name[0] = '\0';
}

bool RecordStore::MemLeaseIdle(const MemLeaseRecord &record) noexcept
{
    return record.name[0] == '\0';
}

void RecordStore::ClearMemLease(MemLeaseRecord &record) noexcept
{
    record.name[0] = '\0';
}

bool RecordStore::MemShareImportIdle(const MemShareImportRecord &record) noexcept
{
    return record.name[0] == '\0';
}

void RecordStore::ClearMemShareImport(MemShareImportRecord &record) noexcept
{
    record.name[0] = '\0';
}

bool RecordStore::MemShareRefIdle(const MemShareRefRecord &record) noexcept
{
    return record.pid == 0;
}

void RecordStore::ClearMemShareRef(MemShareRefRecord &record) noexcept
{
    record.pid = 0;
}

void RecordStore::Restore() noexcept
{
    RestoreRegion();
    RestoreMemLease();
    RestoreShmImport();
    RestoreShmReference();
}

void RecordStore::RestoreRegion() noexcept
{
    auto regions = regionAllocator_->Restore();
    DBG_LOGDEBUG("restore region record count: " << regions.size());

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &region : regions) {
        cachedRegionRecords_.emplace(region->name, region);
    }
}

void RecordStore::RestoreMemLease() noexcept
{
    auto leases = memLeaseAllocator_->Restore();
    DBG_LOGDEBUG("restore lease memory record count: " << leases.size());

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &lease : leases) {
        WithMemIdsRecord<MemLeaseRecord> record{lease};
        if (lease->memIdCount != 0 && poolAllocator_.FillAllocated(lease->memIdBuffer, record.memIds) != 0) {
            continue;
        }
        cachedLeaseRecords_.emplace(lease->name, record);
    }
}

void RecordStore::RestoreShmImport() noexcept
{
    auto shares = shmImportAllocator_->Restore();
    DBG_LOGDEBUG("restore share memory import record count: " << shares.size());

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &share : shares) {
        WithMemIdsRecord<MemShareImportRecord> record{share};
        if (share->memIdCount != 0 && poolAllocator_.FillAllocated(share->memIdBuffer, record.memIds) != 0) {
            continue;
        }
        cachedImportShmRecords_.emplace(share->name, record);
    }
}

void RecordStore::RestoreShmReference() noexcept
{
    auto shares = shmRefAllocator_->Restore();
    DBG_LOGDEBUG("restore share memory reference record count: " << shares.size());

    std::unique_lock<std::mutex> uniqueLock{cachedRecordMutex_};
    for (auto &share : shares) {
        cachedRefShmPidRecords_[share->pid][share->shmImportIndex].emplace_back(share);
        cachedRefShmIdxRecords_[share->shmImportIndex][share->pid].emplace_back(share);
    }
}

void RecordStore::ConvertMemLease(const WithMemIdsRecord<MemLeaseRecord> &record, MemLeaseInfo &info) noexcept
{
    info.first.fdMode = (record.record->fdMode == 1);
    info.first.name = record.record->name;
    info.first.size = record.record->size;
    info.first.distance = record.record->distance;
    info.first.appContext.pid = record.record->pid;
    info.first.appContext.uid = record.record->uid;
    info.first.appContext.gid = record.record->gid;
    info.first.state = record.record->recordState;
    info.second.memIds = record.memIds;
    info.second.numaId = record.record->numaId;
    info.second.unitSize = record.record->unitSize;
    info.second.slotId = record.record->slotId;
}

} // namespace ubsm
} // namespace ock