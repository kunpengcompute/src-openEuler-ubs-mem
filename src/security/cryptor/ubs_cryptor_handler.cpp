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
#include "ubs_cryptor_handler.h"
#include <dlfcn.h>
#include <securec.h>
#include <sys/ipc.h>
#include <fstream>
#include <sstream>
#include <string>
#include "log.h"
#include "rack_mem_functions.h"
#include "system_adapter.h"

ock::ubsm::UbsCryptorHandler::UbsCryptorHandler() noexcept : initialized{false} {}

ock::ubsm::UbsCryptorHandler::~UbsCryptorHandler() noexcept {}

int ock::ubsm::UbsCryptorHandler::Initialize() noexcept
{
    if (initialized.load()) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(decryptMutex);

    if (!initialized.load()) {
        DBG_LOGINFO("Initialize start LoadDecryptFunction.");
        auto res = LoadDecryptFunction();
        if (res != 0) {
            DBG_LOGERROR("Cryptor initialize failed.");
            return -1;
        }
        initialized.store(true);
    }
    return 0;
}

static bool CanonicalPath(std::string &path)
{
    if (path.empty()) {
        DBG_LOGERROR("Path is empty.");
        return false;
    }

    char *realPath = realpath(path.c_str(), nullptr);
    if (realPath == nullptr) {
        DBG_LOGERROR("Failed to real path.");
        return false;
    }

    path = realPath;
    free(realPath);
    realPath = nullptr;
    return true;
}

static int ReadFile(const std::string &path, std::string &content) noexcept
{
    std::string tmpDir = path;
    if (!CanonicalPath(tmpDir)) {
        DBG_LOGERROR("Path is invalid.");
        return -1;
    }

    std::ifstream in(tmpDir);
    if (!in.is_open()) {
        DBG_LOGERROR("Failed to open the file.");
        return -1;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    content = buffer.str();
    // 去除末尾的换行符
    if (!content.empty() && content.back() == '\n') {
        content.pop_back();
    }
    return 0;
}

void SecureClear(void *str, size_t size)
{
    auto ret = memset_s(str, size, 0, size);
    if (ret != 0) {
        DBG_LOGERROR("memset for Decrypt failed, ret=" << ret);
    }
}

int ock::ubsm::UbsCryptorHandler::Decrypt(int domainId, const std::string &filePath,
                                          std::pair<char *, int> &result) noexcept
{
    if (Initialize() != 0) {
        DBG_LOGERROR("Initialize failed.");
        return -1;
    }
    std::string encryptedText;
    auto ret = ReadFile(filePath, encryptedText);
    if (ret != 0) {
        DBG_LOGERROR("Read private key file error: " << ret);
        return -1;
    }
    size_t oldLength = encryptedText.length();
    size_t decryptLength = 0;
    auto decryptRes = decryptLibHandlePtr(encryptedText.c_str(), oldLength, &decryptLength);
    if (decryptRes == nullptr) {
        DBG_LOGERROR("Decrypt tls key password for file failed: " << decryptRes);
        return -1;
    }
    auto buffer = new (std::nothrow) char[decryptLength + 1];
    if (buffer == nullptr) {
        DBG_LOGERROR("allocate memory for buffer failed.");
        SecureClear(decryptRes, decryptLength);
        mxmd::SafeDeleteArray(decryptRes);
        return -1;
    }

    ret = memcpy_s(buffer, decryptLength, decryptRes, decryptLength);
    if (ret != EOK) {
        DBG_LOGERROR("copy decrypt result to buffer failed.");
        mxmd::SafeDeleteArray(buffer);
        SecureClear(decryptRes, decryptLength);
        mxmd::SafeDeleteArray(decryptRes);
        return -1;
    }
    buffer[decryptLength] = '\0';
    SecureClear(decryptRes, decryptLength);
    mxmd::SafeDeleteArray(decryptRes);
    result = std::make_pair(buffer, decryptLength);
    DBG_LOGINFO("Decrypt success.");
    return 0;
}

void ock::ubsm::UbsCryptorHandler::EraseDecryptData(char *data, int len) noexcept
{
    if (data == nullptr) {
        return;
    }
    auto ret = memset_s(data, len, 0, len);
    if (ret != 0) {
        DBG_LOGERROR("memset for EraseDecryptData failed, ret=" << ret);
    }
    DBG_LOGDEBUG("try to delete buffer in Erase func.");
    mxmd::SafeDeleteArray(data);
    return;
}

int ock::ubsm::UbsCryptorHandler::SetCryptorLogger(CryptorLogHandler logger) noexcept
{
    ubsmem::log::UbsmemLoggerManager::Instance()->SetExternLogCallback(
        reinterpret_cast<void (*)(int, const char *)>(logger));
    return 0;
}

bool is_symlink(const std::string &path)
{
    struct stat sb;
    if (lstat(path.c_str(), &sb) == 0) {
        return S_ISLNK(sb.st_mode);
    }
    return false;
}

bool is_regular_file(const std::string &path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0) {
        return S_ISREG(sb.st_mode);
    }
    return false;
}

// 只允许真实 .so 文件（非软链接）
bool validate_real_so(const std::string &so_path)
{
    // 检查是否是软链接
    if (is_symlink(so_path)) {
        DBG_LOGERROR(so_path << " is a symbolic link, not allowed.");
        return false;
    }

    // 检查是否是普通文件
    if (!is_regular_file(so_path)) {
        DBG_LOGERROR(so_path << " is not valid.");
        return false;
    }
    return true;
}

char *DefaultDecrypt(const char *encrypted_data, size_t encrypted_len, size_t *p_out_len)
{
    if (encrypted_data == nullptr || p_out_len == nullptr) {
        return nullptr;
    }

    if (encrypted_len == 0) {
        return nullptr;
    }
    char *result = new char[encrypted_len];
    auto ret = memcpy_s(result, encrypted_len, encrypted_data, encrypted_len);
    if (ret != 0) {
        DBG_LOGERROR("memcpy_s failed, ret:" << ret);
        delete[] result;
        return nullptr;
    }
    *p_out_len = encrypted_len;
    return result;
}

int ock::ubsm::UbsCryptorHandler::LoadDecryptFunction() noexcept
{
    DBG_LOGINFO("LoadDecryptFunction start.");
    static constexpr auto path = "/usr/local/ubs_mem/lib/libdecrypt.so";
    // 如果没有提供so,也需要支持，不进行解密操作，原样返回
    if (access(path, F_OK) != 0) {
        DBG_LOGERROR("File not found: " << path << ", use default.");
        decryptLibHandlePtr = DefaultDecrypt;
        return 0;
    }

    if (!validate_real_so(path)) {
        return -1;
    }
    auto decryLibHandler = SystemAdapter::DlOpen(path, RTLD_NOW);
    if (decryLibHandler == nullptr) {
        DBG_LOGERROR("Failed to dlopen libdecrypt.so, error info=" << dlerror());
        return -1;
    }

    decryptLibHandlePtr = (DecryptFunc)SystemAdapter::DlSym(decryLibHandler, "DecryptPassword");
    if (decryptLibHandlePtr != nullptr) {
        return 0;
    } else {
        DBG_LOGERROR("Failed to dlsym decryptLibHandlePtr, error info=" << dlerror());
        SystemAdapter::DlClose(decryLibHandler);
        return -1;
    }
}
