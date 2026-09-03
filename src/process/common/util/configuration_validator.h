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
#ifndef OCK_COMMON_CONFIGURATION_VALIDATOR_H
#define OCK_COMMON_CONFIGURATION_VALIDATOR_H

#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <iostream>
#include <string>

#include "conf_constants.h"
#include "functions.h"
#include "ref.h"
#include "rpc_config.h"

namespace ock {
namespace common {

constexpr uint32_t SEG = 4;
constexpr uint32_t MAX_IP_SEG_NUM = 255;
constexpr uint32_t MIN_PORT = 7201;
constexpr uint32_t MAX_PORT = 7800;

class Validator : public Referable {
public:
    ~Validator() override = default;

    virtual bool Initialize() = 0;
    virtual bool Validate(const std::string &)
    {
        return true;
    }

    virtual bool Validate(int)
    {
        return true;
    }

    virtual bool Validate(float)
    {
        return true;
    }

    virtual bool Validate(long)
    {
        return true;
    }

    const std::string &ErrorMessage()
    {
        return mErrMsg;
    }

protected:
    explicit Validator(const std::string &name) : mName(name) {}

protected:
    std::string mName;
    std::string mErrMsg;
};
using ValidatorPtr = Ref<Validator>;

class VNoCheck : public Validator {
public:
    static ValidatorPtr Create(const std::string &name = "")
    {
        return ValidatorPtr(new (std::nothrow) VNoCheck(name));
    }

    explicit VNoCheck(const std::string &name) : Validator(name) {}

    ~VNoCheck() override = default;

    bool Initialize() override
    {
        return true;
    }
};

class VStrEnum : public Validator {
public:
    static ValidatorPtr Create(const std::string &name, const std::string &enumStr)
    {
        return ValidatorPtr(new (std::nothrow) VStrEnum(name, enumStr));
    }

    VStrEnum(const std::string &name, const std::string &enumStr) : Validator(name), mEnumString(enumStr) {}

    ~VStrEnum() override = default;

    bool Initialize() override
    {
        // enum string should like this
        // rc||tcp||xx
        std::set<std::string> validEnumSet;
        SplitStr(mEnumString, "||", validEnumSet);
        if (validEnumSet.empty()) {
            mErrMsg = "Failed to initialize validator for <" + mName + ">, because enum string is not correct";
            return false;
        }
        return true;
    }

    bool Validate(const std::string &value) override
    {
        // if value has ||
        // for example rc||tcp
        if (value.find("||") != std::string::npos) {
            mErrMsg = "Invalid value for <" + mName + ">, it should be one of " + mEnumString;
            return false;
        }

        // create ||rc||tcp||xx||
        // find ||rc||
        std::string tmp = "||" + mEnumString + "||";
        if (tmp.find("||" + value + "||") == std::string::npos) {
            mErrMsg = "Invalid value for <" + mName + ">, it should be one of " + mEnumString;
            return false;
        }
        return true;
    }

private:
    std::string mEnumString;
};

class VStrInSet : public Validator {
public:
    static ValidatorPtr Create(const std::string &name, const std::string &enumStr)
    {
        return ValidatorPtr(new (std::nothrow) VStrInSet(name, enumStr));
    }

    VStrInSet(const std::string &name, const std::string &enumStr) : Validator(name), mEnumString(enumStr)
    {
        Initialize();
    }

    ~VStrInSet() override = default;

    bool Initialize() override
    {
        SplitStr(mEnumString, "||", validEnumSet);
        if (validEnumSet.empty()) {
            mErrMsg = "Failed to initialize validator for <" + mName + ">, because enum string is not correct";
            return false;
        }
        return true;
    }

    bool Validate(const std::string &value) override
    {
        std::vector<std::string> validValues;
        SplitStr(value, "|", validValues);
        for (auto item : validValues) {
            if (validEnumSet.find(item) == validEnumSet.end()) {
                mErrMsg = "Invalid value for <" + mName + ">, it should be one of " + mEnumString;
                return false;
            }
        }
        return true;
    }

private:
    std::string mEnumString;
    std::set<std::string> validEnumSet;
};

class VStrNotNull : public Validator {
public:
    static ValidatorPtr Create(const std::string &name)
    {
        return ValidatorPtr(new (std::nothrow) VStrNotNull(name));
    }

    explicit VStrNotNull(const std::string &name) : Validator(name){};

    ~VStrNotNull() override = default;

    bool Initialize() override
    {
        return true;
    }

    bool Validate(const std::string &value) override
    {
        if (value.empty()) {
            mErrMsg = "Invalid value for <" + mName + ">, it should not be empty";
            return false;
        }
        return true;
    }
};

class VIntRange : public Validator {
public:
    static ValidatorPtr Create(const std::string &name, const int &start, const int &end)
    {
        return ValidatorPtr(new (std::nothrow) VIntRange(name, start, end));
    }
    VIntRange(const std::string &name, const int &start, const int &end) : Validator(name), mStart(start), mEnd(end){};

    ~VIntRange() override = default;

    bool Initialize() override
    {
        if (mStart >= mEnd) {
            mErrMsg = "Failed to initialize validator for <" + mName + ">, because end should be bigger than start";
            return false;
        }
        return true;
    }

    bool Validate(int value) override
    {
        if (value < mStart || value > mEnd) {
            if (mEnd == INT32_MAX) {
                mErrMsg = "Invalid value for <" + mName + ">, it should be >= " + std::to_string(mStart);
            } else {
                mErrMsg = "Invalid value for <" + mName + ">, it should be between " + std::to_string(mStart) + "-" +
                          std::to_string(mEnd);
            }
            return false;
        }

        return true;
    }

private:
    int mStart;
    int mEnd;
};

class VPathAccess : public Validator {
public:
    static ValidatorPtr Create(const std::string &name, int flag)
    {
        return ValidatorPtr(new (std::nothrow) VPathAccess(name, flag));
    }

    VPathAccess(const std::string &name, int flag) : Validator(name)
    {
        mFlag = flag;
    }

    ~VPathAccess() override = default;

    bool Initialize() override
    {
        return true;
    }

    bool Validate(const std::string &path) override
    {
        if (path.empty()) {
            mErrMsg = "Invalid value for " + mName + ", path is empty.";
            return false;
        }
        std::string normalizedPath;
        char realPath[PATH_MAX + 1] = {0};
        char *ret = realpath(path.c_str(), realPath);
        if (ret == nullptr) {
            mErrMsg = "Invalid value (" + path + ") for <" + mName + ">. Error: " + strerror(errno);
            return false;
        }
        normalizedPath = realPath;
        std::string fullPath = normalizedPath;
        if (fullPath.back() != '/') {
            fullPath += "/";
        }
        size_t posLeft = 0;
        size_t posRight = 0;
        std::string existDeepestDir = "";
        while (posLeft < fullPath.size()) {
            posRight = fullPath.find("/", posLeft);
            if (posRight == std::string::npos) {
                break;
            }
            if (posRight > posLeft && posLeft > 0) {
                // 检查更深一层路径是否存在
                std::string tmp = existDeepestDir + "/" + fullPath.substr(posLeft, posRight - posLeft);
                if (access(tmp.c_str(), F_OK) != 0) {
                    break;
                }
                existDeepestDir = std::string(tmp); // 记录当前存在的路径
            }
            posLeft = posRight + 1;
        }
        return PathCheck(existDeepestDir, fullPath.substr(posRight), path);
    }

private:
    bool PathCheck(const std::string &existDeepestDir, const std::string &rest, const std::string &path)
    {
        // 检查路径是否合法
        char realBinPath[PATH_MAX + 1] = {0x00};
        if (existDeepestDir.length() > PATH_MAX) {
            return false;
        }
        char *tmp = realpath(existDeepestDir.c_str(), realBinPath);
        if (tmp == nullptr) {
            mErrMsg = "Invalid value (" + path + ") for <" + mName + ">. Permission denied.";
            return false;
        }
        // 检查路径是否有权限访问
        if (access(realBinPath, mFlag) != 0) {
            mErrMsg = "Invalid value (" + path + ") for <" + mName + ">. Permission denied.";
            return false;
        }
        // 检查待创建路径是否存在禁止字符
        if (!rest.empty()) {
            for (auto &forbidden : forbiddenWords) {
                auto pos = rest.find(forbidden + "/");
                if (pos != std::string::npos) {
                    mErrMsg = "Invalid value (" + path + ") for <" + mName +
                              ">, "
                              "whose part need to be automatically created should not contain '" +
                              forbidden + "'.";
                    return false;
                }
            }
        }
        return true;
    }

    int mFlag = 0;
    const std::vector<std::string> forbiddenWords{".."};
};

class VIpAndPort : public Validator {
public:
    static ValidatorPtr Create(const std::string &name)
    {
        return ValidatorPtr(new (std::nothrow) VIpAndPort(name));
    }

    explicit VIpAndPort(const std::string &name) : Validator(name) {}

    ~VIpAndPort() override = default;

    bool Initialize() override
    {
        return true;
    }

    bool Validate(const std::string &value) override
    {
        if (mName == ConfConstant::MXMD_SERVER_RPC_LOCAL_IPSEG.first) {
            std::pair<std::string, uint16_t> ipAndPort;
            if (!ValidateIpAndPort(value, ipAndPort)) {
                return false;
            }
            ock::rpc::NetRpcConfig::GetInstance().SetLocalNode(ipAndPort);
            return true;
        }

        if ((mName == ConfConstant::MXMD_SERVER_RPC_REMOTE_IPSEG.first)) {
            std::stringstream ss(value);
            std::string segment;
            std::vector<std::pair<std::string, uint16_t>> ipList;
            while (std::getline(ss, segment, ',')) {
                std::pair<std::string, uint16_t> ipAndPort;
                if (!ValidateIpAndPort(segment, ipAndPort)) {
                    return false;
                }
                ipList.push_back(ipAndPort);
            }
            ock::rpc::NetRpcConfig::GetInstance().SetRemoteNodes(ipList);
            return true;
        }
        return false;
    }

private:
    bool ValidateIpAndPort(std::string ipSeg, std::pair<std::string, uint16_t> &ipAndPort)
    {
        std::vector<std::string> validSet;
        SplitStr(ipSeg, ":", validSet);
        if (validSet.empty() || validSet.size() != 2) {
            mErrMsg = "Failed to initialize validator for <" + mName + ">, because enum string is not correct";
            return false;
        }

        int port;
        try {
            port = std::stoi(validSet[1]);
        } catch (...) {
            mErrMsg = "Invalid port for <" + mName + ">";
            return false;
        }
        if (port < MIN_PORT || port > MAX_PORT) {
            mErrMsg = "Invalid port for <" + mName + ">, port should be in [7201, 7800]";
            return false;
        }

        if (!ValidateIp(validSet[0])) {
            mErrMsg = "Invalid Ip for <" + mName;
        }
        ipAndPort.first = validSet[0];
        ipAndPort.second = static_cast<uint16_t>(port);
        return true;
    }

    bool ValidateIp(const std::string &ip)
    {
        std::istringstream ss(ip);
        std::string token;
        int count = 0;

        while (std::getline(ss, token, '.')) {
            if (++count > SEG) {
                return false;
            }
            if (token.empty() || token.size() > SEG - 1) {
                return false;
            }
            for (char c : token) {
                if (!std::isdigit(c)) {
                    return false;
                }
            }
            int num = std::stoi(token);
            if (num < 0 || num > MAX_IP_SEG_NUM) {
                return false;
            }
            if (token.front() == '0' && token.size() > 1) {
                return false;
            }
        }
        return count == SEG;
    }
};

class VFileAccess : public Validator {
public:
    static ValidatorPtr Create(const std::string &name, bool flag)
    {
        return ValidatorPtr(new (std::nothrow) VFileAccess(name, flag));
    }

    VFileAccess(const std::string &name, bool flag) : Validator(name)
    {
        mFlag = flag;
    }

    ~VFileAccess() override = default;

    bool Initialize() override
    {
        return true;
    }

    bool Validate(const std::string &filePath) override
    {
        if (filePath.empty()) {
            if (!mFlag) {
                return true;
            }
            mErrMsg = "Invalid value for " + mName + ", path is empty.";
            return false;
        }

        return PathCheck(filePath);
    }

private:
    bool PathCheck(const std::string &filePath)
    {
        if (filePath[0] != '/') {
            mErrMsg = "The file path(" + filePath + ") is not absolute path.";
            return false;
        }
        char realPath[PATH_MAX] = {0};
        if (realpath(filePath.c_str(), realPath) == nullptr) {
            mErrMsg = "Failed to resolve file path(" + filePath + "), errno(" + std::to_string(errno) + ").";
            return false;
        }
        if (strcmp(filePath.c_str(), realPath) != 0) {
            mErrMsg = "The file path(" + filePath + ") is not canonical path.";
            return false;
        }
        struct stat st;
        if (lstat(filePath.c_str(), &st) != 0) {
            mErrMsg = "The file path(" + filePath + ") is not accessable, errno(" + std::to_string(errno) + ").";
            return false;
        }
        if (S_ISLNK(st.st_mode)) {
            mErrMsg = "The file path(" + filePath + ") is a link.";
            return false;
        }
        if (!S_ISREG(st.st_mode)) {
            mErrMsg = "The file path(" + filePath + ") is not regular file.";
            return false;
        }
        if (access(filePath.c_str(), R_OK) != 0) {
            mErrMsg = "The file path(" + filePath + ") is not read accessable.";
            return false;
        }
        return true;
    }

    bool mFlag = false;
};

} // namespace common
} // namespace ock

#endif
