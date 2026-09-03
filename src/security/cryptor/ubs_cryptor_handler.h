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

#ifndef UBS_HSECRYPTORHELPER_H
#define UBS_HSECRYPTORHELPER_H

#include <mutex>
#include <string>
#include "log.h"

#include "ubs_cryptor_def.h"

namespace ock::ubsm {
using DecryptFunc = char *(*)(const char *encrypted_data, size_t encrypted_len, size_t *p_out_len);

class UbsCryptorHandler {
public:
    UbsCryptorHandler() noexcept;
    ~UbsCryptorHandler() noexcept;
    static UbsCryptorHandler &GetInstance()
    {
        static UbsCryptorHandler instance;
        return instance;
    }

public:
    int Initialize() noexcept;
    int Decrypt(int domainId, const std::string &filePath, std::pair<char *, int> &result) noexcept;
    void EraseDecryptData(char *data, int len) noexcept;
    static int SetCryptorLogger(CryptorLogHandler logger) noexcept;
    int LoadDecryptFunction() noexcept;

private:
    std::atomic<bool> initialized;
    std::mutex decryptMutex;
    DecryptFunc decryptLibHandlePtr = nullptr;
};
} // namespace ock::ubsm
#endif