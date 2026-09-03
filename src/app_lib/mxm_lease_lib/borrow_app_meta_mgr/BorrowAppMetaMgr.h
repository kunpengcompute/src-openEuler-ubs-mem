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
#ifndef MEMORYFABRIC_BORROW_APP_META_MGR_H
#define MEMORYFABRIC_BORROW_APP_META_MGR_H

#include <rack_mem_functions.h>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "rack_mem_err.h"
#include "rack_mem_lib_common.h"
#include "rack_mem_libobmm.h"
#include "system_adapter.h"

namespace ock::mxmd {
class AppBorrowMetaDesc {
public:
    AppBorrowMetaDesc() = default;

    explicit AppBorrowMetaDesc(const uint64_t fileSize, const std::string region = "default")
        : fileSize{fileSize},
          regionName_(region)
    {
    }

    void UpdateBorrowMetaDesc(const std::vector<uint64_t> &memIdIn, const uint64_t fileSizeIn, bool isNumaIn,
                              const std::string &nameIn)
    {
        this->memIds = memIdIn;
        this->actualMapFileSize = fileSizeIn;
        this->isNuma = isNumaIn;
        this->name = nameIn;
    }

    [[nodiscard]] int GetIsNuma() const
    {
        return isNuma;
    }

    [[nodiscard]] mem_id GetMinMemId() const
    {
        return MinMemId(this->memIds);
    }

    [[nodiscard]] uint64_t GetFileSize() const
    {
        return fileSize;
    }

    [[nodiscard]] uint64_t GetActualMapFileSize() const
    {
        return actualMapFileSize;
    }

    void SetFileSize(const uint64_t fileSizeIn)
    {
        this->fileSize = fileSizeIn;
    }
    size_t GetUniSize() const
    {
        return unitSize;
    }
    [[nodiscard]] std::string GetName() const
    {
        return name;
    }

    [[nodiscard]] std::string GetRegionName() const
    {
        return regionName_;
    }

    [[nodiscard]] std::vector<mem_id> GetMemIds() const
    {
        return memIds;
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
    void AddFd(const int fd)
    {
        fds.push_back(fd);
    }
    void AddAddr(void *addr)
    {
        addrs.push_back(addr);
    }
    void CloseAllFd()
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
    void SetName(const std::string &nameIn)
    {
        this->name = nameIn;
    }
    [[nodiscard]] int FirstFd() const
    {
        if (fds.empty()) {
            return -1;
        }
        return fds[0];
    }

private:
    std::vector<int> fds{}; // numa模式下，该值不再使用
    std::vector<void *> addrs{};
    std::vector<mem_id> memIds{}; // numa模式下，该memId非obmm import返回的id，仅作为资源释放标识符
    uint64_t fileSize{INVALID_MEM_SIZE};
    uint64_t actualMapFileSize{INVALID_MEM_SIZE};
    std::string regionName_{};
    std::string name{}; // numa模式下，该值不再使用，name由agent端生成
    bool isNuma{false};
    size_t unitSize{};
    bool hasUnmapped_{false};
    bool isLockAddress_{false};
};

class BorrowAppMetaMgr {
public:
    uint32_t AddMeta(const void *addr, const AppBorrowMetaDesc &desc);

    uint32_t RemoveMeta(const void *addr);

    uint32_t GetMeta(const void *addr, AppBorrowMetaDesc &desc);

    uint32_t UpdateMeta(const void *addr, AppBorrowMetaDesc &desc);

    inline size_t PtrToSegment(void *ptr) const
    {
        return ptrHash(ptr) % SEGMENT_COUNT;
    }

    inline size_t PtrToSegment(const void *ptr) const
    {
        return ptrHash(const_cast<void *>(ptr)) % SEGMENT_COUNT;
    }

    int32_t SetMetaHasUnmapped(const void *addr, bool val);
    int32_t SetMetaIsLockAddress(const void *addr, bool val);

    bool GetMetaHasUnmapped(const void *addr);
    bool GetMetaIsLockAddress(const void *addr);

    uint32_t GetAllUsedMemoryNames(std::vector<std::string> &names);
    static BorrowAppMetaMgr &GetInstance()
    {
        static BorrowAppMetaMgr instance;
        return instance;
    }

private:
    BorrowAppMetaMgr() = default;

    static constexpr int SEGMENT_COUNT{4};
    std::hash<void *> ptrHash{};
    SpinLock mapLock[SEGMENT_COUNT]{};
    std::unordered_map<const void *, AppBorrowMetaDesc> appBorrowInfoMap[SEGMENT_COUNT]{};
};
} // namespace ock::mxmd
// ock

#endif // MEMORYFABRIC_BORROW_APP_META_MGR_H