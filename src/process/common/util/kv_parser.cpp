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
#include "kv_parser.h"
#include "utils/ubsm_file_util.h"

namespace ock {
namespace common {
KVParser::KVParser() : mLock() {}

KVParser::~KVParser()
{
    {
        GUARD(&mLock, mLock);
        for (auto val : mItems) {
            KvPair *p = val;
            SAFE_DELETE(p);
        }

        mItems.clear();
        mItemsIndex.clear();
    }
}

HRESULT KVParser::FromFile(const std::string &filePath)
{
    char path[PATH_MAX + 1] = {0};
    if (filePath.size() > PATH_MAX || realpath(filePath.c_str(), path) == nullptr) {
        return HFAIL;
    }
    /* open file to read */
    std::ifstream inConfFile(path);
    if (!ock::utils::FileUtil::CheckFileSize(inConfFile, MAX_CONF_FILE_SIZE)) {
        inConfFile.close();
        return HFAIL;
    }
    std::string strLine;
    HRESULT hr = HOK;
    while (getline(inConfFile, strLine)) {
        strLine.erase(strLine.find_last_not_of("\r") + 1);
        OckTrimString(strLine);
        if (strLine.empty() || strLine.at(0) == '#') {
            continue;
        }
        if (strLine.size() > MAX_LENGTH_COMMON_CONFIG) {
            std::cerr << "Configuration file <" << path << "> strline is too long." << std::endl;
            hr = HFAIL;
            break;
        }

        std::string::size_type equalDivPos = strLine.find("=");
        if (equalDivPos == strLine.npos) {
            continue;
        }

        std::string strKey = strLine.substr(0, equalDivPos);
        std::string strValue = strLine.substr(equalDivPos + 1, strLine.size() - 1);
        OckTrimString(strKey);
        OckTrimString(strValue);

        if (strKey.empty() || strValue.empty()) {
            hr = HFAIL;
            std::cerr << "Configuration item has empty key or value." << std::endl;
            break;
        }
        if (SetItem(strKey, strValue) != 0) {
            hr = HFAIL;
            break;
        }
    }

    inConfFile.close();
    inConfFile.clear();

    return hr;
}

HRESULT KVParser::GetItem(const std::string &key, std::string &outValue)
{
    GUARD(&mLock, mLock);
    auto iter = mItemsIndex.find(key);
    if (iter != mItemsIndex.end()) {
        auto itemPtr = mItems.at(iter->second);
        if (itemPtr == nullptr) {
            return HFAIL;
        }
        outValue = itemPtr->value;
        return HOK;
    }
    return HFAIL;
}

HRESULT KVParser::SetItem(const std::string &key, const std::string &value)
{
    GUARD(&mLock, mLock);
    auto iter = mItemsIndex.find(key);
    if (iter != mItemsIndex.end()) {
        std::cerr << "Key <" << key << "> in configuration file is repeated." << std::endl;
        return HFAIL;
    }
    auto *kv = new (std::nothrow) KvPair();
    if (kv == nullptr) {
        std::cerr << "Parse lines in configuration file failed, maybe out of memory." << std::endl;
        return HFAIL;
    }
    kv->name = key;
    kv->value = value;
    mItems.push_back(kv);
    mItemsIndex.insert(std::make_pair(key, mItems.size() - 1));
    if (mGotKeys.find(key) == mGotKeys.end()) {
        mGotKeys.insert(std::make_pair(key, true));
    }
    return HOK;
}

uint32_t KVParser::Size()
{
    GUARD(&mLock, mLock);
    return mItems.size();
}

void KVParser::GetI(uint32_t index, std::string &outKey, std::string &outValue)
{
    GUARD(&mLock, mLock);
    if (index >= mItems.size()) {
        return;
    }
    auto itemPtr = mItems.at(index);
    if (itemPtr == nullptr) {
        return;
    }
    outKey = itemPtr->name;
    outValue = itemPtr->value;
}

void KVParser::Dump()
{
    GUARD(&mLock, mLock);
    for (auto val : mItems) {
        KvPair *p = val;
        if (p == nullptr) {
            continue;
        }
        printf("%s = %s\n", p->name.c_str(), p->value.c_str());
    }
}

bool KVParser::CheckSet(const std::vector<std::string> &keys)
{
    bool check = true;
    for (auto &item : keys) {
        if (mGotKeys.find(item) == mGotKeys.end()) {
            std::cerr << "Config item <" << item << "> is not set!" << std::endl;
            check = false;
        }
    }
    mGotKeys = std::unordered_map<std::string, bool>();
    return check;
}
} // namespace common
} // namespace ock
