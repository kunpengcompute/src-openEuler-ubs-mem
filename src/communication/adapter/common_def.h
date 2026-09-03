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
#ifndef COMMON_DEF_H
#define COMMON_DEF_H
#include <functional>
#include <iostream>
#include <unordered_map>
#include <utility>

template <typename T>
void SafeFree(T &ptr)
{
    if (ptr) {
        free(ptr);
        ptr = nullptr;
    }
}

template <typename T>
void SafeDelete(T &ptr)
{
    if (ptr) {
        delete ptr;
        ptr = nullptr;
    }
}

template <typename T>
void SafeDeleteArray(T *&ptr, size_t ptrLen = 1)
{
    if (ptr && ptrLen != 0) {
        delete[] ptr;
        ptr = nullptr;
    }
}

template <typename T1, typename T2>
struct PairHash {
    size_t operator()(const std::pair<T1, T2> &p) const noexcept
    {
        static_assert(sizeof(T1) + sizeof(T2) <= sizeof(uint64_t),
                      "Types are too large to combine into a single hash value");
        uint64_t combined = (static_cast<uint64_t>(p.first) << (sizeof(T2) * 8)) | p.second;
        return std::hash<uint64_t>{}(combined);
    }
};

template <typename Key1, typename Key2, typename Value>
using PairMap = std::unordered_map<std::pair<Key1, Key2>, Value, PairHash<Key1, Key2>>;

#endif // COMMON_DEF_H
