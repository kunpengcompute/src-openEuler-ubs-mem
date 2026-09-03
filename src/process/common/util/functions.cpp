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

#include "functions.h"

#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <string>
#include "dirent.h"
#include "log_adapter.h"

namespace ock {
namespace common {
void OckTrimString(std::string &str)
{
    if (str.empty()) {
        return;
    }

    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
}

std::string OckGetHostname()
{
    struct utsname buf;
    if (uname(&buf) != 0) {
        *buf.nodename = '\0';
    }

    return buf.nodename;
}

void SplitStr(const std::string &str, const std::string &separator, std::set<std::string> &result)
{
    if (separator.empty()) {
        if (str.empty()) {
            return;
        }
        result.insert(str);
        return;
    }
    std::string::size_type pos1 = 0;
    std::string::size_type pos2 = str.find(separator);

    std::string tmpStr;
    while (pos2 != std::string::npos) {
        tmpStr = str.substr(pos1, pos2 - pos1);
        // trim the string, i.e. remove space at the begin and end position
        OckTrimString(tmpStr);
        result.insert(tmpStr);

        pos1 = pos2 + separator.size();
        pos2 = str.find(separator, pos1);
    }

    if (pos1 != str.length()) {
        tmpStr = str.substr(pos1);
        // trim the string, i.e. remove space at the begin and end position
        OckTrimString(tmpStr);
        result.insert(tmpStr);
    }
}

void SplitStr(const std::string &str, const std::string &separator, std::vector<std::string> &result)
{
    if (separator.empty()) {
        if (str.empty()) {
            return;
        }
        result.emplace_back(str);
        return;
    }
    std::string::size_type pos1 = 0;
    std::string::size_type pos2 = str.find(separator);

    std::string tmpStr;
    while (pos2 != std::string::npos) {
        tmpStr = str.substr(pos1, pos2 - pos1);
        OckTrimString(tmpStr);
        result.emplace_back(tmpStr);
        pos1 = pos2 + separator.size();
        pos2 = str.find(separator, pos1);
    }

    if (pos1 != str.length()) {
        tmpStr = str.substr(pos1);
        OckTrimString(tmpStr);
        result.emplace_back(tmpStr);
    }
}

void SplitStr(const std::string &str, const std::string &separator, std::list<std::string> &result)
{
    if (separator.empty()) {
        if (str.empty()) {
            return;
        }
        result.push_back(str);
        return;
    }
    std::string::size_type pos1 = 0;
    std::string::size_type pos2 = str.find(separator);

    std::string tmpStr;
    while (pos2 != std::string::npos) {
        tmpStr = str.substr(pos1, pos2 - pos1);
        result.push_back(tmpStr);
        pos1 = pos2 + separator.size();
        pos2 = str.find(separator, pos1);
    }

    if (pos1 != str.length()) {
        tmpStr = str.substr(pos1);
        result.push_back(tmpStr);
    }
}

bool OckFileDirExists(const std::string &path)
{
    if (access(path.c_str(), 0) == -1) {
        return false;
    }
    return true;
}

bool OckFolderDirDelete(const std::string &oldPath)
{
    std::string path = oldPath + "_deleted";
    int result = ::rename(oldPath.c_str(), path.c_str());
    if (result != 0) {
        DBG_LOGERROR("Failed to rename " << oldPath << " to " << path << " with errno " << errno);
        return false;
    }

    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        DBG_LOGERROR("Can't open directory " << path);
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct stat statBuf;
        std::string absPath = path + "/" + entry->d_name;
        if (!stat(absPath.c_str(), &statBuf) && S_ISDIR(statBuf.st_mode)) {
            if (!OckFolderDirDelete(absPath)) {
                closedir(dir);
                return false;
            }
        }
        remove(absPath.c_str());
    }

    closedir(dir);

    remove(path.c_str());
    return true;
}

HRESULT CreateArgs(int &newArgc, char **&newArgv, const std::vector<std::string> &values)
{
    if (newArgv != nullptr) {
        return HFAIL;
    }
    newArgc = static_cast<int>(values.size());
    newArgv = new (std::nothrow) char *[newArgc];
    if (newArgv == nullptr) {
        return HFAIL;
    }
    errno_t ret = EOK;
    for (int j = 0; j < newArgc; j++) {
        auto &value = values.at(j);
        newArgv[j] = new (std::nothrow) char[value.size() + 1];
        if (newArgv[j] == nullptr) {
            DeleteStrArray(j, newArgv);
            return HFAIL;
        }
        ret = strcpy_s(newArgv[j], value.size() + 1, value.c_str());
        if (ret != EOK) {
            DeleteStrArray(j + 1, newArgv);
            return HFAIL;
        }
    }
    return HOK;
}

void DeleteStrArray(int count, char **&array)
{
    if (array != nullptr) {
        for (int i = 0; i < count; i++) {
            if (array[i] != nullptr) {
                delete[] array[i];
            }
        }
        delete[] array;
        array = nullptr;
    }
}

} // namespace common
} // namespace ock