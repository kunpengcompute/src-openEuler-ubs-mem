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

#ifndef RACK_MEMORYFABRIC_FUNCTIONS_H
#define RACK_MEMORYFABRIC_FUNCTIONS_H

#include <regex.h>
#include <securec.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <list>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include "log.h"

#include "ubs_mem_def.h"
#include "rack_mem_constants.h"
#include "rack_mem_err.h"

using namespace ock::dagger;
using namespace ock::common;

namespace ock::mxmd {

class ThreadExecutorServiceHolder {
public:
    static ThreadExecutorServiceHolder &GetInstance()
    {
        static ThreadExecutorServiceHolder gInstance;
        return gInstance;
    }

    void StopAllThreadExecutorService()
    {
        std::unique_lock<std::recursive_mutex> uniqueLock(mLock);
        if (mOneThreadExecutorService != nullptr) {
            mOneThreadExecutorService->Stop();
            mOneThreadExecutorService = nullptr;
        }
        if (mMoreThreadExecutorService != nullptr) {
            mMoreThreadExecutorService->Stop();
            mMoreThreadExecutorService = nullptr;
        }
    }

    [[nodiscard]] ExecutorServicePtr GetOneThreadExecutorService()
    {
        std::unique_lock<std::recursive_mutex> uniqueLock(mLock);
        if (mOneThreadExecutorService == nullptr) {
            mOneThreadExecutorService = ExecutorService::Create(1);
        }
        return mOneThreadExecutorService;
    }

    [[nodiscard]] ExecutorServicePtr GetMoreThreadExecutorService()
    {
        std::unique_lock<std::recursive_mutex> uniqueLock(mLock);
        if (mMoreThreadExecutorService == nullptr) {
            mMoreThreadExecutorService = ExecutorService::Create(8u);
        }
        return mMoreThreadExecutorService;
    }

private:
    ThreadExecutorServiceHolder() = default;
    std::recursive_mutex mLock{};
    ExecutorServicePtr mOneThreadExecutorService{nullptr};
    ExecutorServicePtr mMoreThreadExecutorService{nullptr};
};

inline ExecutorServicePtr GetOneThreadExecutorService()
{
    auto ptr = ThreadExecutorServiceHolder::GetInstance().GetOneThreadExecutorService();
    if (ptr == nullptr) {
        DBG_LOGERROR("Failed to create executor service");
        return nullptr;
    }
    ptr->SetThreadName("OneThreadExecutorService");
    if (!ptr->Start()) {
        DBG_LOGERROR("Failed to start executor service");
        return nullptr;
    }
    return ptr;
}

inline ExecutorServicePtr GetMoreThreadExecutorService()
{
    auto ptr = ThreadExecutorServiceHolder::GetInstance().GetMoreThreadExecutorService();
    if (ptr == nullptr) {
        DBG_LOGERROR("Failed to create executor service");
        return nullptr;
    }
    ptr->SetThreadName("MoreThreadExecutorService");
    if (!ptr->Start()) {
        DBG_LOGERROR("Failed to start executor service");
        return nullptr;
    }
    return ptr;
}

class MemStrUtil {
public:
    static std::vector<std::string> SplitTrim(const std::string &src, const std::string &sep);
    static inline bool StrToULL(const std::string &src, uint64_t &value, int base = 10L);
};

inline bool MemStrUtil::StrToULL(const std::string &src, uint64_t &value, int base)
{
    char *remain = nullptr;
    errno = 0;
    value = std::strtoull(src.c_str(), &remain, base);
    if (base == 16U && value == 0 && src == "0x0") {
        return true;
    }
    if ((value == 0 && src != "0") || remain == nullptr || strlen(remain) > 0 || errno == ERANGE) {
        return false;
    }
    return true;
}

inline std::vector<std::string> MemStrUtil::SplitTrim(const std::string &src, const std::string &sep)
{
    std::vector<std::string> tmp{};
    std::vector<std::string> tmp2{};
    StrUtil::Split(src, sep, tmp);
    for (const auto &str : tmp) {
        if (!str.empty()) {
            tmp2.push_back(str);
        }
    }
    return tmp2;
}

template <typename T>
void SafeFree(T &ptr)
{
    if (ptr) {
        free(ptr);
        ptr = nullptr;
    }
}

template <typename T>
void SafeDelete(T *&ptr)
{
    if (ptr) {
        delete ptr;
        ptr = nullptr;
    }
}

template <typename T>
void SafeDeleteArray(T *&ptr)
{
    if (ptr) {
        delete[] ptr;
        ptr = nullptr;
    }
}

template <typename T>
void SafeDeleteArray(T *&ptr, size_t ptrLen)
{
    if (ptr && ptrLen != 0) {
        delete[] ptr;
        ptr = nullptr;
    }
}

template <typename T>
bool SafeAdd(T a, T b, T &result)
{
    const auto ret = __builtin_add_overflow(a, b, &result);
    if (ret) {
        DBG_LOGERROR("Add would overflow.");
    }
    return ret;
}

template <typename T>
bool SafeSub(T a, T b, T &result)
{
    const auto ret = __builtin_sub_overflow(a, b, &result);
    if (ret) {
        DBG_LOGERROR("Sub would overflow.");
    }
    return ret;
}

template <typename T>
bool SafeMul(T a, T b, T &result)
{
    const auto ret = __builtin_mul_overflow(a, b, &result);
    if (ret) {
        DBG_LOGERROR("Multiplication would overflow.");
    }
    return ret;
}

// 模板函数，将 std::array<XXX, 12> 转换为 C 语言的数组 XXX[12]
template <typename T, std::size_t N>
void ConvertToArray(const std::array<T, N> &stdArray, T cArray[N])
{
    for (std::size_t i = 0; i < N; ++i) {
        cArray[i] = stdArray[i];
    }
}

template <typename T, std::size_t N>
void ConvertToArray(const std::array<std::array<T, N>, N> &stdArray, T cArray[N][N])
{
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            cArray[i][j] = stdArray[i][j];
        }
    }
}

inline uint64_t MinMemId(const std::vector<uint64_t> &memIdList)
{
    if (memIdList.empty()) {
        return INVALID_MEM_ID;
    }
    const auto minIter = std::min_element(memIdList.begin(), memIdList.end());
    return *minIter;
}

inline std::string MemToStr(const std::vector<uint64_t> &list)
{
    std::string memIdStr;
    memIdStr += "[";
    for (const auto &memId : list) {
        memIdStr += std::to_string(memId) + ",";
    }
    if (memIdStr.size() > 1) {
        memIdStr.resize(memIdStr.size() - 1);
    }
    memIdStr += "]";
    return memIdStr;
}

// 错误码映射表
static const std::unordered_map<uint32_t, int> errorMapping = {
    // common error
    {MXM_ERR_MEMLIB, UBSM_ERR_MEMLIB},
    {MXM_ERR_NULLPTR, UBSM_ERR_PARAM_INVALID},
    {MXM_ERR_MALLOC_FAIL, UBSM_ERR_MALLOC_FAIL},
    {MXM_ERR_MEMORY, UBSM_ERR_MEMORY},
    {MXM_ERR_PARAM_INVALID, UBSM_ERR_PARAM_INVALID},
    {MXM_ERR_MMAP_VA_FAILED, UBSM_ERR_PARAM_INVALID},
    {MXM_ERR_CHECK_RESOURCE, UBSM_CHECK_RESOURCE_ERROR},
    {MXM_ERR_DAEMON_NO_FEATURE_ENABLED, UBSM_ERR_UNIMPL},
    {MXM_ERR_NAME_BUSY, UBSM_ERR_BUSY},
    // net
    {MXM_ERR_IPC_INIT_CALL, UBSM_ERR_NET},
    {MXM_ERR_IPC_SYNC_CALL, UBSM_ERR_NET},
    {MXM_ERR_IPC_HCOM_INNER_SYNC_CALL, UBSM_ERR_NET},
    {MXM_ERR_IPC_CRC_CHECK_ERROR, UBSM_ERR_NET},
    {MXM_ERR_IPC_SERIALIZE_DESERIALIZE_ERROR, UBSM_ERR_NET},
    {MXM_ERR_REGISTER_OP_CODE, UBSM_ERR_NET},
    {MXM_ERR_REGISTER_HANDLER_NULL, UBSM_ERR_NET},
    {MXM_ERR_RPC_QUERY_ERROR, UBSM_ERR_NET},
    // cc lock
    {MXM_ERR_LOCK_NOT_READY, UBSM_ERR_LOCK_NOT_SUPPORTED},
    {MXM_ERR_LOCK_ALREADY_LOCKED, UBSM_ERR_LOCK_ALREADY_LOCKED},
    {MXM_ERR_LOCK_NOT_FOUND, UBSM_ERR_NOT_FOUND},
    {MXM_ERR_DLOCK_LIB, UBSM_ERR_DLOCK},
    {MXM_ERR_DLOCK_INNER, UBSM_ERR_DLOCK},
    // recorde store
    {MXM_ERR_RECORD_DELETE, UBSM_ERR_RECORD},
    {MXM_ERR_RECORD_ADD, UBSM_ERR_RECORD},
    {MXM_ERR_RECORD_CHANGE_STATE, UBSM_ERR_RECORD},
    // region
    {MXM_ERR_REGION_NOT_FOUND, UBSM_ERR_NOT_FOUND},
    {MXM_ERR_REGION_PARAM_INVALID, UBSM_ERR_PARAM_INVALID},
    {MXM_ERR_REGION_EXIST, UBSM_ERR_ALREADY_EXIST},
    {MXM_ERR_REGION_NOT_EXIST, UBSM_ERR_NOT_FOUND},
    {MXM_ERR_REGION_NO_NEEDED, UBSM_ERR_NO_NEEDED},
    // shm obj
    {MXM_ERR_SHM_NOT_FOUND, UBSM_ERR_NOT_FOUND},
    {MXM_ERR_SHM_ALREADY_EXIST, UBSM_ERR_ALREADY_EXIST},
    {MXM_ERR_SHM_PERMISSION_DENIED, UBSM_ERR_NOPERM},
    {MXM_ERR_SHM_IN_USING, UBSM_ERR_IN_USING},
    {MXM_ERR_SHM_NOT_EXIST, UBSM_ERR_NOT_FOUND},
    // lease obj
    {MXM_ERR_LEASE_NOT_FOUND, UBSM_ERR_NOT_FOUND},
    {MXM_ERR_LEASE_NOT_EXIST, UBSM_ERR_NOT_FOUND},
    // ubse
    {MXM_ERR_UBSE_INNER, UBSM_ERR_UBSE},
    {MXM_ERR_UBSE_LIB, UBSM_ERR_UBSE},
    {MXM_ERR_UBSE_NOT_ATTACH, UBSM_ERR_UBSE},
    {MXM_ERR_UBSE_NOT_SUPPORTED, UBSM_ERR_NOT_SUPPORTED},
    // obmm
    {MXM_ERR_OBMM_INNER, UBSM_ERR_OBMM},
    {MXM_ERR_SHM_OBMM_OPEN, UBSM_ERR_OBMM},
};

// 错误码转换函数
inline int GetErrCode(uint32_t internalError) noexcept
{
    if (internalError == MXM_OK) {
        return UBSM_OK;
    }
    auto it = errorMapping.find(internalError);
    if (it != errorMapping.end()) {
        return it->second;
    }
    DBG_LOGERROR("Failed to find error=" << internalError << ", return error=" << UBSM_ERR_BUFF);
    return UBSM_ERR_BUFF;
}

inline std::string ConvertBoolToString(bool value) noexcept
{
    return value ? "true" : "false";
}

#define ERROR_STRING_ENTRY(entry) {(entry), (#entry)},
// 错误码字符串映射表
static const std::unordered_map<uint32_t, std::string> errorStrMapping = {
    ERROR_STRING_ENTRY(MXM_OK) ERROR_STRING_ENTRY(MXM_ERR_MEMLIB) ERROR_STRING_ENTRY(MXM_ERR_NULLPTR)
        ERROR_STRING_ENTRY(MXM_ERR_MALLOC_FAIL) ERROR_STRING_ENTRY(MXM_ERR_MEMORY) ERROR_STRING_ENTRY(
            MXM_ERR_PARAM_INVALID) ERROR_STRING_ENTRY(MXM_ERR_MMAP_VA_FAILED) ERROR_STRING_ENTRY(MXM_ERR_CHECK_RESOURCE)
            ERROR_STRING_ENTRY(MXM_ERR_DAEMON_NO_FEATURE_ENABLED) ERROR_STRING_ENTRY(MXM_ERR_NAME_BUSY)
                ERROR_STRING_ENTRY(MXM_ERR_IPC_INIT_CALL) ERROR_STRING_ENTRY(MXM_ERR_IPC_SYNC_CALL) ERROR_STRING_ENTRY(
                    MXM_ERR_IPC_HCOM_INNER_SYNC_CALL) ERROR_STRING_ENTRY(MXM_ERR_IPC_CRC_CHECK_ERROR)
                    ERROR_STRING_ENTRY(MXM_ERR_IPC_SERIALIZE_DESERIALIZE_ERROR) ERROR_STRING_ENTRY(
                        MXM_ERR_REGISTER_OP_CODE) ERROR_STRING_ENTRY(MXM_ERR_REGISTER_HANDLER_NULL)
                        ERROR_STRING_ENTRY(MXM_ERR_RPC_QUERY_ERROR) ERROR_STRING_ENTRY(MXM_ERR_LOCK_NOT_READY)
                            ERROR_STRING_ENTRY(MXM_ERR_LOCK_ALREADY_LOCKED) ERROR_STRING_ENTRY(MXM_ERR_LOCK_NOT_FOUND)
                                ERROR_STRING_ENTRY(MXM_ERR_DLOCK_LIB) ERROR_STRING_ENTRY(
                                    MXM_ERR_DLOCK_INNER) ERROR_STRING_ENTRY(MXM_ERR_RECORD_DELETE)
                                    ERROR_STRING_ENTRY(MXM_ERR_RECORD_ADD) ERROR_STRING_ENTRY(
                                        MXM_ERR_RECORD_CHANGE_STATE) ERROR_STRING_ENTRY(MXM_ERR_REGION_NOT_FOUND)
                                        ERROR_STRING_ENTRY(MXM_ERR_REGION_PARAM_INVALID) ERROR_STRING_ENTRY(
                                            MXM_ERR_REGION_EXIST) ERROR_STRING_ENTRY(MXM_ERR_REGION_NOT_EXIST)
                                            ERROR_STRING_ENTRY(MXM_ERR_REGION_NO_NEEDED) ERROR_STRING_ENTRY(
                                                MXM_ERR_SHM_NOT_FOUND) ERROR_STRING_ENTRY(MXM_ERR_SHM_ALREADY_EXIST)
                                                ERROR_STRING_ENTRY(MXM_ERR_SHM_PERMISSION_DENIED) ERROR_STRING_ENTRY(
                                                    MXM_ERR_SHM_IN_USING) ERROR_STRING_ENTRY(MXM_ERR_SHM_NOT_EXIST)
                                                    ERROR_STRING_ENTRY(MXM_ERR_LEASE_NOT_FOUND)
                                                        ERROR_STRING_ENTRY(MXM_ERR_LEASE_NOT_EXIST)
                                                            ERROR_STRING_ENTRY(MXM_ERR_UBSE_INNER)
                                                                ERROR_STRING_ENTRY(MXM_ERR_UBSE_LIB)
                                                                    ERROR_STRING_ENTRY(MXM_ERR_UBSE_NOT_ATTACH)
                                                                        ERROR_STRING_ENTRY(MXM_ERR_OBMM_INNER)
                                                                            ERROR_STRING_ENTRY(MXM_ERR_SHM_OBMM_OPEN)};

inline std::string ConvertErrorToString(int value) noexcept
{
    auto it = errorStrMapping.find(value);
    if (it != errorStrMapping.end()) {
        return it->second;
    }
    return "UNKNOWN_ERROR(" + std::to_string(value) + ")";
}

constexpr size_t HUGE_PAGES_2M = 2 * 1024 * 1024;
constexpr size_t HUGE_PAGES_512M = 512 * 1024 * 1024;

static uint64_t GetHugeTlbPmdSize()
{
    if (getpagesize() == NO_64 * NO_1024) {
        return HUGE_PAGES_512M;
    }
    return HUGE_PAGES_2M;
}

} // namespace ock::mxmd

#endif