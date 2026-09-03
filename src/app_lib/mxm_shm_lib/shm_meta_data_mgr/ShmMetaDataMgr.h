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

#ifndef MEMORYFABRIC_SHM_META_DATA_MGR_H
#define MEMORYFABRIC_SHM_META_DATA_MGR_H

#include <cstring>
#include <ostream>
#include <unordered_map>
#include <utility>
#include "mx_shm.h"
#include "rack_mem_err.h"
#include "rack_mem_lib_common.h"
#include "system_adapter.h"

using namespace ock::dagger;

namespace ock::mxmd {
using namespace ock::ubsm;
struct ShmAppMetaData {
    std::string name{};
    int fd{INVALID_MEM_FD};
    mem_id memId{INVALID_MEM_ID};
    void *addr{nullptr};
    size_t mapSize{0}; // unmap的时候需要检查和map的大小
    size_t unitSize{0};
    std::vector<mem_id> memIds{};
    std::vector<int> fds{};
    std::vector<std::pair<void *, size_t>> addrs{};
    ShmOwnStatus ownStatuss; // 共享内存读写权限
    uint64_t flag{};
    uint64_t oflag{};
    int isLocked{0};
    ShmAppMetaData() = default;

    ShmAppMetaData(std::string name, const std::vector<mem_id> &memIdIn, void *const addr, const size_t mapSize,
                   ShmOwnStatus status, uint64_t flag)
        : name{std::move(name)},
          addr{addr},
          mapSize{mapSize},
          memIds{memIdIn},
          ownStatuss{status},
          flag{flag}
    {
    }

    [[nodiscard]] bool GetHasUnmapped() const
    {
        return hasUnmapped_;
    }

    [[nodiscard]] bool GetisLockAddress() const
    {
        return isLockAddress_;
    }
    void SetisLockAddress(bool val)
    {
        isLockAddress_ = val;
    }
    void SetHasUnmapped(bool val)
    {
        hasUnmapped_ = val;
    }
    void SetUnitSize(size_t unitSizeIn)
    {
        unitSize = unitSizeIn;
    }
    void SetOflag(uint64_t oflagPar)
    {
        oflag = oflagPar;
    }
    void AddFds(const int fdIn)
    {
        fds.push_back(fdIn);
    };
    void AddAddr(void *addrIn, size_t length)
    {
        addrs.push_back({addrIn, length});
    };

    void UnmmapAllAddr()
    {
        for (auto &item : addrs) {
            if (SystemAdapter::MemoryUnMap(item.first, item.second) == -1) {
                DBG_LOGERROR("Munmap failed. length is " << item.second);
            }
        }
    }
    inline void CloseAllFd()
    {
        for (auto &item : fds) {
            if (item == -1) {
                continue;
            }
            if (ubsm::SystemAdapter::Close(item) == -1) {
                DBG_LOGWARN("Close fd " << item << " failed, error=" << strerror(errno));
            } else {
                item = -1;
            };
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const ShmAppMetaData &obj)
    {
        return os << "name: " << obj.name << " fd: " << obj.fd << " memId: " << obj.memId
                  << " unitSize:" << obj.unitSize;
    }

private:
    bool hasUnmapped_{false};
    bool isLockAddress_{false};
};

class ShmMetaDataMgr {
public:
    uint32_t Init();
    uint32_t Destroy();
    void *GetAddr(const std::string &name);
    uint32_t AddMetaData(const std::string &name, void *mAddr, ShmAppMetaData &shmAppMetaData);
    int32_t UpdateMetaData(const std::string &name, const ShmAppMetaData &shmAppMetaData);
    // 检查start已经被map过，传入的mapsize必须是map时传入的大小，unmap size必须等于map的大小
    int32_t CheckNameExistAndGet(void *start, size_t mapSize, ShmAppMetaData &shmAppMetaData);
    int32_t CheckAddr(std::string name, void *start, size_t length, ShmAppMetaData &shmAppMetaData, uint64_t &usedFirst,
                      std::vector<int> &indices);
    int32_t GetShmMetaFromName(const std::string &name, ShmAppMetaData &shmAppMetaData);
    uint32_t RemoveMetaData(void *start, const std::string &name);
    inline size_t StrToSegment(const std::string &name) const
    {
        return strHash(name) % SEGMENT_COUNT;
    }

    inline size_t PtrToSegment(void *ptr) const
    {
        return ptrHash(ptr) % SEGMENT_COUNT;
    }
    inline size_t PtrToSegment(const void *ptr) const
    {
        return ptrHash(const_cast<void *>(ptr)) % SEGMENT_COUNT;
    }

    int32_t SetMetaHasUnmapped(const std::string &name, bool val);
    int32_t SetMetaIsLockAddress(const std::string &name, bool val);
    bool GetMetaHasUnmapped(const std::string &name);
    bool GetMetaIsLockAddress(const std::string &name);

    uint32_t GetAllUsedMemoryNames(std::vector<std::string> &names);

    bool HasOverlapWithKnownVmas(uintptr_t start, uintptr_t end);

    int32_t AddMappedMemoryRange(void *start, size_t length);

    int32_t RemoveMappedMemoryRange(void *start);

    static ShmMetaDataMgr &GetInstance()
    {
        static ShmMetaDataMgr instance;
        return instance;
    }
    ShmMetaDataMgr(const ShmMetaDataMgr &other) = delete;
    ShmMetaDataMgr(ShmMetaDataMgr &&other) = delete;
    ShmMetaDataMgr &operator=(const ShmMetaDataMgr &other) = delete;
    ShmMetaDataMgr &operator=(ShmMetaDataMgr &&other) noexcept = delete;

private:
    static constexpr int SEGMENT_COUNT{4};
    std::hash<std::string> strHash{};
    std::hash<void *> ptrHash{};
    Lock mNameLock[SEGMENT_COUNT]{};
    Lock mPtrLock[SEGMENT_COUNT]{};
    ReadWriteLock mVmasLock{};
    // 不支持name多次map
    std::unordered_map<std::string, ShmAppMetaData> mAppShmInfoMap[SEGMENT_COUNT]{};
    std::unordered_map<void *, std::string> mAddr2NameMap[SEGMENT_COUNT]{};
    std::map<uintptr_t, uintptr_t> mMappedMemoryAddress{}; // [start, end) 按地址start升序
    ShmMetaDataMgr() = default;
};
} // namespace ock::mxmd
// ock

#endif // MEMORYFABRIC_SHM_META_DATA_MGR_H