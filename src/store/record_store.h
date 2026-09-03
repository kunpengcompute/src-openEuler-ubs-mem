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
#ifndef SIMPLE_SAMPLES_RECORD_STORE_H
#define SIMPLE_SAMPLES_RECORD_STORE_H

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base_record_allocator.h"
#include "record_id_pool_allocator.h"
#include "record_store_def.h"
#include "ubs_common_types.h"

namespace ock {
namespace ubsm {

/**
 * @brief 包含一个指向共享内存的记录指针和一个vector用于表示一组memId
 * @tparam RecordType 共享内存上保存的持化化的结构类型
 */
template <class RecordType>
struct WithMemIdsRecord {
    RecordType *record{nullptr};
    std::vector<uint64_t> memIds;

    WithMemIdsRecord() noexcept {}
    explicit WithMemIdsRecord(RecordType *rd) noexcept : record{rd} {}
    WithMemIdsRecord(RecordType *rd, std::vector<uint64_t> ms) noexcept : record{rd}, memIds{std::move(ms)} {}
};

/**
 * @brief 内存借用信息的类型定义
 */
using MemLeaseInfo = std::pair<LeaseMallocInput, LeaseMallocResult>;

/**
 * @brief 本类中包含持久化到共享内存中的处理和一个方便查询的本地内存中的缓存信息，此类保证缓存中的信息与持久化中的一致。
 */
class RecordStore {
public:
    static RecordStore &GetInstance() noexcept;

    /**
     * @brief 初始化过程中打开共享内存，读取其中持久化的记录缓存下来
     * @return 0表示成功，非0表示失败
     */
    int Initialize(int fd) noexcept;

    /**
     * @brief 清除初始化过程中建立的信息
     */
    void Destroy() noexcept;

public:
    /**
     * @brief 向缓存及共享内存持久化中同时增加一个region信息
     * @param input region信息
     * @return 0表示成功，非0表示失败
     */
    int AddRegionRecord(const CreateRegionInput &input) noexcept;

    /**
     * @brief 通过名称从缓存及共享内存持久化中同时删除一条region记录
     * @param name region名称，每个region名称节点内唯一
     * @return 0表示成功，非0表示失败
     */
    int DelRegionRecord(const std::string &name) noexcept;

    /**
     * @brief 列举缓存中的全部region信息
     * @return 返回region信息vector，不保证顺序
     */
    std::vector<CreateRegionInput> ListRegionRecord() const noexcept;

    /**
     * @brief 向缓存及共享内存持久化中同时增加一个内存借用信息
     * @param info 内存借用信息
     * @return 0表示成功，非0表示失败
     */
    int AddMemLeaseRecord(const MemLeaseInfo &info) noexcept;

    /**
     * @brief 通过名称从缓存及共享内存持久化中同时删除一条内存借用信息记录
     * @param name 内存借用信息名称，每个内存借用信息名称全局唯一
     * @return 0表示借用内存存入缓存中，1表示从缓存以及持久化中删除成功，非0表示失败
     */
    int DelMemLeaseRecord(const std::string &name) noexcept;

    int AddMemLeaseInput(const LeaseMallocInput &input) noexcept;

    int AddMemLeaseResult(const std::string &name, const LeaseMallocResult &info) noexcept;

    int UpdateMemLeaseRecordState(const std::string &name, RecordState state) noexcept;

    /**
     * @brief 列举缓存中的全部内存借用信息
     * @return 返回内存借用信息vector，不保证顺序
     */
    std::vector<MemLeaseInfo> ListMemLeaseRecord() const noexcept;

    int UpdateMemLeaseRecord(const std::string &name, const AppContext &context, uint64_t size) noexcept;

    /**
     * @brief 向缓存及共享内存持久化中同时增加一个导入的内存共享信息，在使用共享内存节点调用，同一个共享内存对象在同一个节点只能导入
     *        一次，如果本节点多次使用同一个共享内存对象，一次导入后，多次引用，参见 AddShmRefRecord
     * @param info 导入的共享内存信息
     * @return 0表示成功，非0表示失败
     */
    int AddShmImportRecord(const ShareMemImportInfo &info) noexcept;

    /**
     * @brief 通过名称从缓存及共享内存持久化中同时删除一条已导入的内存共享信息，删除导入共享内存记录前题是引用数为0，否则失败
     * @param name 导入的共享内存名称
     * @return 0表示成功，非0表示失败
     */
    int DelShmImportRecord(const std::string &name) noexcept;

    int AddShmImportInput(const ShareMemImportInput &input) noexcept;
    int AddShmImportResult(const std::string &name, const ShareMemImportResult &result) noexcept;
    int UpdateShmImportRecordState(const std::string &name, RecordState state) noexcept;

    /**
     * @brief 列举缓存中的全部已导入的内存共享信息
     * @return 返回已创建的内存共享信息vector，不保证顺序
     */
    std::vector<ShareMemImportInfo> ListShmImportRecord() const noexcept;

    /**
     * @brief 向缓存及共享内存持久化中同时增加一个内存共享引用信息，传入的名称必须是已导入的，参见 AddShmImportRecord
     * @param pid 使用此引用的应用进程pid
     * @param name 引用的共享内存名称
     * @return 0表示成功，非0表示失败
     */
    uint32_t GetCreateSeqNo() const noexcept
    {
        return __sync_fetch_and_add(createSeqNo_, 1U);
    }
    int AddShmRefRecord(pid_t pid, const std::string &name) noexcept;

    /**
     * @brief 通过应用进程pid和共享内存名称从缓存及共享内存持久化中同时删除一条被引用的内存共享信息
     * @param pid 应用进程pid
     * @param name 共享内存名称
     * @return 0表示成功，非0表示失败
     */
    int DelShmRefRecord(pid_t pid, const std::string &name) noexcept;

    /**
     * 通过应用进程pid和从缓存及共享内存持久化中同时删除全部此进程引用的内存共享信息，如果此进程未引用，也返回成功。
     * @param pid 应用进程pid
     * @return 0表示成功，非0表示失败
     */
    int DelShmRefRecordsByPid(pid_t pid) noexcept;

    /**
     * @brief 列举缓存中的全部被引用的共享内存信息，以共享内存名称优先，应用进程pid次之的顺序返回，得到的信息是对于每个共享内存，
     *        被各个pid引用的次数。
     * @return 返回全部被的内存共享信息vector，不保证顺序
     */
    std::unordered_map<std::string, std::unordered_map<pid_t, uint32_t>> ListShmRefRecordNameF() const noexcept;

    /**
     * @brief 列举缓存中的全部被引用的共享内存信息，以应用进程pid优先，共享内存名称次之的顺序返回，得到的信息是对于每个应用进程，
     *        引用各个共享内存的次数。
     * @return 返回全部被的内存共享信息vector，不保证顺序
     */
    std::unordered_map<pid_t, std::unordered_map<std::string, uint32_t>> ListShmRefRecordPidF() const noexcept;

private:
    RecordStore() noexcept;
    ~RecordStore() noexcept;
    static bool RegionIdle(const RegionRecord &record) noexcept;
    static void ClearRegion(RegionRecord &record) noexcept;
    static bool MemLeaseIdle(const MemLeaseRecord &record) noexcept;
    static void ClearMemLease(MemLeaseRecord &record) noexcept;
    static bool MemShareImportIdle(const MemShareImportRecord &record) noexcept;
    static void ClearMemShareImport(MemShareImportRecord &record) noexcept;
    static bool MemShareRefIdle(const MemShareRefRecord &record) noexcept;
    static void ClearMemShareRef(MemShareRefRecord &record) noexcept;
    void Restore() noexcept;
    void RestoreRegion() noexcept;
    void RestoreMemLease() noexcept;
    void RestoreShmImport() noexcept;
    void RestoreShmReference() noexcept;
    static void ConvertMemLease(const WithMemIdsRecord<MemLeaseRecord> &record, MemLeaseInfo &info) noexcept;
    bool CheckAllocators();

private:
    int shmFd_;
    void *mappingAddress_{nullptr};
    RegionRecord *regionRecordBegin_{nullptr};
    MemLeaseRecord *memLeaseRecordBegin_{nullptr};
    MemShareImportRecord *shmImportRecordBegin_{nullptr};
    MemShareRefRecord *shmRefRecordBegin_{nullptr};
    MemIdRecordPool *memIdRecordPoolBegin_{nullptr};
    uint32_t *createSeqNo_{nullptr};

    mutable std::mutex cachedRecordMutex_;
    std::unordered_map<std::string, RegionRecord *> cachedRegionRecords_;
    std::unordered_map<std::string, WithMemIdsRecord<MemLeaseRecord>> cachedLeaseRecords_;
    std::unordered_map<std::string, WithMemIdsRecord<MemShareImportRecord>> cachedImportShmRecords_;
    std::unordered_map<uint32_t, std::unordered_map<pid_t, std::vector<MemShareRefRecord *>>> cachedRefShmIdxRecords_;
    std::unordered_map<pid_t, std::unordered_map<uint32_t, std::vector<MemShareRefRecord *>>> cachedRefShmPidRecords_;

    std::shared_ptr<BaseRecordAllocator<RegionRecord>> regionAllocator_;
    std::shared_ptr<BaseRecordAllocator<MemLeaseRecord>> memLeaseAllocator_;
    std::shared_ptr<BaseRecordAllocator<MemShareImportRecord>> shmImportAllocator_;
    std::shared_ptr<BaseRecordAllocator<MemShareRefRecord>> shmRefAllocator_;

    RecordIdPoolAllocator poolAllocator_;
};
} // namespace ubsm
} // namespace ock

#endif // SIMPLE_SAMPLES_RECORD_STORE_H
