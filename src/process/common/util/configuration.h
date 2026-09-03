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
#ifndef OCK_COMMON_COMMON_UTIL_CONF_H
#define OCK_COMMON_COMMON_UTIL_CONF_H

#include <map>
#include <vector>

#include "conf_constants.h"
#include "configuration_converter.h"
#include "configuration_validator.h"
#include "defines.h"
#include "lock.h"
#include "ref.h"

namespace ock {
namespace common {
constexpr uint32_t CONF_MUST = 1;

enum class ConfValueType {
    VINT = 0,
    VFLOAT = 1,
    VSTRING = 2,
    VBOOL = 3,
    VLONG = 4,
};

const std::unordered_map<std::string, std::string> ConfigUnitValues = {
    {ConfConstant::MXMD_LOCK_EXPIRE_TIME.first, "seconds"}};

class Configuration;
using ConfigurationPtr = Ref<Configuration>;

class Configuration : public Referable {
public:
    Configuration();
    ~Configuration() override;

    // forbid copy operation
    Configuration(const Configuration &) = delete;
    Configuration &operator=(const Configuration &) = delete;

    // forbid move operation
    Configuration(const Configuration &&) = delete;
    Configuration &operator=(const Configuration &&) = delete;

    static ConfigurationPtr FromFile(const std::string &filePath);
    static ConfigurationPtr GetInstance(const std::string &filePath);
    static ConfigurationPtr GetInstance();
    static void DestroyInstance();

    int32_t GetInt(const std::string &key, int32_t defaultValue = 0);
    float GetFloat(const std::string &key, float defaultValue = 0.0);
    std::string GetString(const std::string &key, const std::string &defaultValue = "");
    bool GetBool(const std::string &key, bool defaultValue = false);
    long GetLong(const std::string &key, long defaultValue = 0);
    std::string GetConvertedValue(const std::string &key);

    void Set(const std::string &key, int32_t value);
    void Set(const std::string &key, float value);
    void Set(const std::string &key, const std::string &value);
    void Set(const std::string &key, bool value);
    void Set(const std::string &key, long value);

    bool SetWithTypeAutoConvert(const std::string &key, const std::string &value);

    void DumpConfig();
    void AddIntConf(const std::pair<std::string, int> &pair, const ValidatorPtr &validator = nullptr,
                    uint32_t flag = CONF_MUST);
    void AddStrConf(const std::pair<std::string, std::string> &pair, const ValidatorPtr &validator = nullptr,
                    uint32_t flag = CONF_MUST);
    void AddConverter(const std::string &key, const ConverterPtr &converter);
    void AddPathConf(const std::pair<std::string, std::string> &pair, const ValidatorPtr &validator = nullptr,
                     uint32_t flag = CONF_MUST);
    std::vector<std::string> Validate(bool isAuth = false, bool isTLS = false, bool isAuthor = false,
                                      bool isZKSecure = false);
    std::vector<std::string> ValidateDaemonConf();
    std::vector<std::string> ValidateFilePath(std::vector<std::pair<std::string, bool>> &filePaths);

    inline std::string GetConfigPath()
    {
        return mConfigPath;
    }

    inline void SetConfigPath(std::string filePath)
    {
        mConfigPath = filePath;
    }

    bool Initialized()
    {
        return mInitialized;
    }

private:
    void SetValidator(const std::string &key, const ValidatorPtr &validator, uint32_t flag);

    void ValidateOneValueMap(std::vector<std::string> &errors,
                             const std::map<std::string, ValidatorPtr> &valueValidator);

    template <class T>
    void AddValidateError(const ValidatorPtr &validator, std::vector<std::string> &errors, const T &iter)
    {
        if (validator == nullptr) {
            errors.push_back("Failed to validate <" + iter->first + ">, validator is NULL");
            return;
        }
        if (!(validator->Validate(iter->second))) {
            errors.push_back(validator->ErrorMessage());
        }
    }
    void ValidateOneType(const std::string &key, const ValidatorPtr &validator, std::vector<std::string> &errors,
                         ConfValueType &vType);

    void ValidateItem(const std::string &itemKey, std::vector<std::string> &errors);

    void LoadConfigurations();

    void LoadDefault()
    {
        using namespace ConfConstant;
        AddPathConf(MXMD_SERVER_LOG_PATH, VPathAccess::Create(MXMD_SERVER_LOG_PATH.first, R_OK | W_OK | X_OK));
        AddStrConf(MXMD_SERVER_LOG_LEVEL,
                   VStrEnum::Create(MXMD_SERVER_LOG_LEVEL.first, "DEBUG||INFO||WARN||ERROR||CRITICAL"));
        AddConverter(MXMD_SERVER_LOG_LEVEL.first, ConLogLevel::Create());
        AddIntConf(MXMD_SERVER_LOG_ROTATION_FILE_SIZE,
                   VIntRange::Create(MXMD_SERVER_LOG_ROTATION_FILE_SIZE.first, MIN_LOG_FILE_SIZE, MAX_LOG_FILE_SIZE));

        AddIntConf(UBSMD_MAX_HCOM_CONNECT_NUM,
                   VIntRange::Create(UBSMD_MAX_HCOM_CONNECT_NUM.first, MIN_HCOM_CONNECT_NUM, MAX_HCOM_CONNECT_NUM));

        AddIntConf(MXMD_SERVER_LOG_ROTATION_FILE_COUNT, VIntRange::Create(MXMD_SERVER_LOG_ROTATION_FILE_COUNT.first,
                                                                          MIN_LOG_FILE_COUNT, MAX_LOG_FILE_COUNT));
        AddStrConf(MXMD_SERVER_AUDIT_ENABLE, VStrEnum::Create(MXMD_SERVER_AUDIT_ENABLE.first, "on||off"));
        AddStrConf(MXMD_SEVER_PERFORMANCE_STATISTICS_ENABLE,
                   VStrEnum::Create(MXMD_SEVER_PERFORMANCE_STATISTICS_ENABLE.first, "on||off"));
        AddPathConf(MXMD_SERVER_AUDIT_LOG_PATH,
                    VPathAccess::Create(MXMD_SERVER_AUDIT_LOG_PATH.first, R_OK | W_OK | X_OK));
        AddIntConf(MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_SIZE,
                   VIntRange::Create(MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_SIZE.first, MIN_AUDIT_LOG_FILE_SIZE,
                                     MAX_AUDIT_LOG_FILE_SIZE));
        AddIntConf(MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_COUNT,
                   VIntRange::Create(MXMD_SERVER_AUDIT_LOG_ROTATION_FILE_COUNT.first, MIN_AUDIT_LOG_FILE_COUNT,
                                     MAX_AUDIT_LOG_FILE_COUNT));
        AddIntConf(MXMD_DISCOVERY_ELECTION_TIMEOUT,
                   VIntRange::Create(MXMD_DISCOVERY_ELECTION_TIMEOUT.first, MIN_DISCOVERY_ELECTION_TIMEOUT,
                                     MAX_DISCOVERY_ELECTION_TIMEOUT));
        AddIntConf(MXMD_DISCOVERY_MIN_NODES,
                   VIntRange::Create(MXMD_DISCOVERY_MIN_NODES.first, MIN_DISCOVERY_MIN_NODES, MAX_DISCOVERY_MIN_NODES));
        AddStrConf(MXMD_SERVER_RPC_LOCAL_IPSEG, VIpAndPort::Create(MXMD_SERVER_RPC_LOCAL_IPSEG.first), 0);
        AddStrConf(MXMD_SERVER_RPC_REMOTE_IPSEG, VIpAndPort::Create(MXMD_SERVER_RPC_REMOTE_IPSEG.first), 0);
        AddStrConf(MXMD_IS_LOCK_ENABLE, VStrEnum::Create(MXMD_IS_LOCK_ENABLE.first, "on||off"));
        AddStrConf(MXMD_IS_LOCK_UB_TOKEN_ENABLE, VStrEnum::Create(MXMD_IS_LOCK_UB_TOKEN_ENABLE.first, "on||off"));
        AddStrConf(MXMD_IS_LOCK_TLS_ENABLE, VStrEnum::Create(MXMD_IS_LOCK_TLS_ENABLE.first, "on||off"));
        AddStrConf(UBSMD_LOCK_TLS_CA_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_LOCK_TLS_CRL_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_LOCK_TLS_CERT_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_LOCK_TLS_KEY_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_LOCK_TLS_KEYPASS_PATH, VNoCheck::Create(), 0);
        AddStrConf(MXMD_LOCK_DEV_NAME, VNoCheck::Create(), 0);
        AddStrConf(MXMD_LOCK_DEV_EID, VNoCheck::Create(), 0);
        AddIntConf(MXMD_LOCK_EXPIRE_TIME,
                   VIntRange::Create(MXMD_LOCK_EXPIRE_TIME.first, MIN_LOCK_EXPIRE_TIME, MAX_LOCK_EXPIRE_TIME));
        AddStrConf(UBSMD_SERVER_TLS_ENABLE, VStrEnum::Create(UBSMD_SERVER_TLS_ENABLE.first, "on||off"));
        AddStrConf(UBSMD_SERVER_TLS_CIPHERSUITS,
                   VStrEnum::Create(UBSMD_SERVER_TLS_CIPHERSUITS.first,
                                    "aes_gcm_128||aes_gcm_256||aes_ccm_128||chacha20_poly1305"));
        AddStrConf(UBSMD_SERVER_TLS_CA_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_SERVER_TLS_CRL_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_SERVER_TLS_CERT_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_SERVER_TLS_KEY_PATH, VNoCheck::Create(), 0);
        AddStrConf(UBSMD_SERVER_TLS_KEYPASS_PATH, VNoCheck::Create(), 0);
        AddStrConf(MXMD_SERVER_LEASE_CACHE_ENABLE, VStrEnum::Create(MXMD_SERVER_LEASE_CACHE_ENABLE.first, "on||off"));
    }

private:
    static ConfigurationPtr gInstance;
    std::string mConfigPath;

    std::map<std::string, int32_t> mIntItems;
    std::map<std::string, float> mFloatItems;
    std::map<std::string, std::string> mStrItems;
    std::map<std::string, bool> mBoolItems;
    std::map<std::string, long> mLongItems;
    std::map<std::string, std::string> mAllItems;

    std::map<std::string, ConfValueType> mValueTypes;
    std::map<std::string, ValidatorPtr> mValueValidator;
    std::map<std::string, ConverterPtr> mValueConverter;

    std::vector<std::pair<std::string, std::string>> mServiceList;
    std::vector<std::string> mMustKeys;
    std::vector<std::string> mLoadDefaultErrors;

    std::vector<std::string> mPathConfs;
    std::vector<std::string> mExceptPrintConfs{ConfConstant::MXMD_DAEMON_BINPATH.first};
    std::vector<std::string> mInvalidSetConfs{ConfConstant::MXMD_DAEMON_BINPATH.first};

    bool mInitialized = false;
    Lock mLock;
};

} // namespace common
} // namespace ock

#endif
