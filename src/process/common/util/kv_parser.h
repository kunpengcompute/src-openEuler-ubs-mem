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
#ifndef OCK_COMMON_COMMON_UTIL_KVFILE_H
#define OCK_COMMON_COMMON_UTIL_KVFILE_H

#include <fstream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "functions.h"
#include "lock.h"

namespace ock {
namespace common {
using KvPair = struct KvPairs {
    std::string name;
    std::string value;
};

class KVParser {
public:
    KVParser();
    ~KVParser();

    KVParser(const KVParser &) = delete;
    KVParser &operator=(const KVParser &) = delete;
    KVParser(const KVParser &&) = delete;
    KVParser &operator=(const KVParser &&) = delete;

    HRESULT FromFile(const std::string &filePath);

    HRESULT GetItem(const std::string &key, std::string &outValue);
    HRESULT SetItem(const std::string &key, const std::string &value);

    uint32_t Size();
    void GetI(uint32_t index, std::string &outKey, std::string &outValue);

    void Dump();
    bool CheckSet(const std::vector<std::string> &keys);

private:
    std::map<std::string, uint32_t> mItemsIndex;
    std::vector<KvPair *> mItems;
    std::unordered_map<std::string, bool> mGotKeys;
    Lock mLock;
};
} // namespace common
} // namespace ock

#endif