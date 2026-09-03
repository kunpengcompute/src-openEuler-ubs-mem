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

#ifndef UBS_COMMON_CONFIG_H
#define UBS_COMMON_CONFIG_H

#include <unistd.h>
#include <string>
#include "hcom.h"

namespace ock::ubsm {

const std::string CIPHER_STR_AES_GCM_128 = "aes_gcm_128";
const std::string CIPHER_STR_AES_GCM_256 = "aes_gcm_256";
const std::string CIPHER_STR_AES_CCM_128 = "aes_ccm_128";
const std::string CIPHER_STR_CHACHA20_POLY1305 = "chacha20_poly1305";

class UbsCommonConfig {
public:
    static UbsCommonConfig &GetInstance()
    {
        static UbsCommonConfig instance;
        return instance;
    }

    void SetLeaseCacheSwitch(bool b)
    {
        enableLeaseCache_ = b;
    }

    bool GetLeaseCachedSwitch() const
    {
        return enableLeaseCache_;
    }

    void SetTlsSwitch(bool b)
    {
        enableTls_ = b;
    }

    bool GetTlsSwitch() const
    {
        return enableTls_;
    }

    void SetCipherSuit(const std::string &cipher)
    {
        if (cipher == CIPHER_STR_AES_GCM_128) {
            cipherSuit_ = ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128;
        } else if (cipher == CIPHER_STR_AES_GCM_256) {
            cipherSuit_ = ock::hcom::UBSHcomNetCipherSuite::AES_GCM_256;
        } else if (cipher == CIPHER_STR_AES_CCM_128) {
            cipherSuit_ = ock::hcom::UBSHcomNetCipherSuite::AES_CCM_128;
        } else if (cipher == CIPHER_STR_CHACHA20_POLY1305) {
            cipherSuit_ = ock::hcom::UBSHcomNetCipherSuite::CHACHA20_POLY1305;
        } else {
            cipherSuit_ = ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128;
        }
        return;
    }

    ock::hcom::UBSHcomNetCipherSuite GetCipherSuit() const
    {
        return cipherSuit_;
    }

    void SetCaPath(const std::string &ca)
    {
        caPath_ = ca;
    }

    void SetCrlPath(const std::string &crl)
    {
        crlPath_ = crl;
    }

    void SetCertPath(const std::string &cert)
    {
        certPath_ = cert;
    }

    void SetKeyPath(const std::string &key)
    {
        keyPath_ = key;
    }

    void SetLockCaPath(const std::string &ca)
    {
        lockCaPath_ = ca;
    }

    void SetLockCrlPath(const std::string &crl)
    {
        lockCrlPath_ = crl;
    }

    void SetLockCertPath(const std::string &cert)
    {
        lockCertPath_ = cert;
    }

    void SetLockKeyPath(const std::string &key)
    {
        lockKeyPath_ = key;
    }

    void SetLockKeypassPath(const std::string &keypass)
    {
        lockKeypassPath_ = keypass;
    }

    void SetLockKsfMasterPath(const std::string &ksfMaster)
    {
        lockKsfMasterPath_ = ksfMaster;
    }

    void SetLockKsfStandbyPath(const std::string &ksfStandby)
    {
        lockKsfStandbyPath_ = ksfStandby;
    }

    std::string GetLockCaPath() const
    {
        return lockCaPath_;
    }

    std::string GetLockCrlPath() const
    {
        return lockCrlPath_;
    }

    std::string GetLockCertPath() const
    {
        return lockCertPath_;
    }

    std::string GetLockKeyPath() const
    {
        return lockKeyPath_;
    }

    std::string GetLockKeypassPath() const
    {
        return lockKeypassPath_;
    }

    std::string GetLockKsfMasterPath() const
    {
        return lockKsfMasterPath_;
    }

    std::string GetLockKsfStandbyPath() const
    {
        return lockKsfStandbyPath_;
    }

    void SetKeypassPath(const std::string &keypass)
    {
        keypassPath_ = keypass;
    }

    void SetKsfMasterPath(const std::string &ksfMaster)
    {
        ksfMasterPath_ = ksfMaster;
    }

    void SetKsfStandbyPath(const std::string &ksfStandby)
    {
        ksfStandbyPath_ = ksfStandby;
    }

    std::string GetCaPath() const
    {
        return caPath_;
    }

    std::string GetCrlPath() const
    {
        return crlPath_;
    }

    std::string GetCertPath() const
    {
        return certPath_;
    }

    std::string GetKeyPath() const
    {
        return keyPath_;
    }

    std::string GetKeypassPath() const
    {
        return keypassPath_;
    }

    std::string GetKsfMasterPath() const
    {
        return ksfMasterPath_;
    }

    std::string GetKsfStandbyPath() const
    {
        return ksfStandbyPath_;
    }

private:
    UbsCommonConfig() = default;
    ~UbsCommonConfig() = default;

    bool enableLeaseCache_{true};
    bool enableTls_{false};
    ock::hcom::UBSHcomNetCipherSuite cipherSuit_{ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128};
    std::string caPath_;
    std::string crlPath_;
    std::string certPath_;
    std::string keyPath_;
    std::string keypassPath_;
    std::string ksfMasterPath_;
    std::string ksfStandbyPath_;
    std::string lockCaPath_;
    std::string lockCrlPath_;
    std::string lockCertPath_;
    std::string lockKeyPath_;
    std::string lockKeypassPath_;
    std::string lockKsfMasterPath_;
    std::string lockKsfStandbyPath_;
};

} // namespace ock::ubsm

#endif