/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.

 * ubs-mem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *      http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include "ubsmem_logger_filesink.h"

#include <sys/stat.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "ubsmem_logger_constants.h"

namespace ubsmem::log {

static bool CreateDir(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        std::error_code ec;
        fs::create_directories(path, ec);
        return !static_cast<bool>(ec);
    }
    return true;
}

UbsmemLoggerFilesink::UbsmemLoggerFilesink(std::string basePath, uint32_t maxFileSize, uint32_t maxFileCount)
    : basePath_(std::move(basePath)),
      maxFileSize_(maxFileSize),
      maxFileCount_(maxFileCount)
{
}

UbsmemLoggerFilesink::~UbsmemLoggerFilesink()
{
    for (auto &it : fileMap_) {
        if (it.second.logFile.is_open()) {
            it.second.logFile.close();
        }
    }
}

bool UbsmemLoggerFilesink::Initialize() const
{
    if (maxFileSize_ == 0 || maxFileCount_ == 0) {
        return false;
    }

    std::error_code ec;
    fs::create_directories(fs::path(basePath_), ec);
    if (ec) {
        std::cerr << "Failed to create directory: " << basePath_ << std::endl;
        return false;
    }
    return true;
}

bool UbsmemLoggerFilesink::InitializeAuditSink(const std::string &basePath, uint32_t maxFileSize, uint32_t maxFileCount)
{
    if (!CreateDir(basePath)) {
        std::cerr << "Failed to create audit directory: " << basePath << std::endl;
        return false;
    }
    auditBasePath_ = basePath;
    auditMaxFileSize_ = maxFileSize;
    auditMaxFileCount_ = maxFileCount;
    return true;
}

bool UbsmemLoggerFilesink::Write(const UbsmemLoggerEntry &loggerEntry)
{
    const char *fileName = loggerEntry.IsAudit() ? "ubsmd.audit" : "ubsmd";
    if (!fileMap_[fileName].isInitialized) {
        std::string fileBasePath = loggerEntry.IsAudit() ? auditBasePath_ : basePath_;
        fileMap_[fileName].filePath = fileBasePath + "/" + fileName + ".log";
        fileMap_[fileName].isInitialized = true;
        fileMap_[fileName].maxFileCount = loggerEntry.IsAudit() ? auditMaxFileCount_ : maxFileCount_;
    }

    bool needOpen = !fileMap_[fileName].logFile.is_open() || IsFileStatusChanged(fileName) ||
                    !fileMap_[fileName].logFile.good();
    if (needOpen) {
        fileMap_[fileName].logFile.close();
        if (!OpenFile(fileName)) {
            std::cerr << "Open file failed" << std::endl;
            return false;
        }
    }

    loggerEntry.OutPutLog(fileMap_[fileName].logFile);

    uint32_t fileSizeLimit = maxFileSize_;
    if (loggerEntry.IsAudit() && auditMaxFileSize_ > 0) {
        fileSizeLimit = auditMaxFileSize_;
    }
    uintmax_t currentFileSize = 0;
    try {
        currentFileSize = fs::file_size(fileMap_[fileName].filePath);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    if (currentFileSize > fileSizeLimit) {
        if (!RollFile(fileName)) {
            std::cerr << "Failed rolling file" << std::endl;
            return false;
        }
    }
    return true;
}

bool UbsmemLoggerFilesink::IsFileStatusChanged(const std::string &fileName)
{
    struct stat fileStat {
    };
    if (stat(fileMap_[fileName].filePath.c_str(), &fileStat) != 0) {
        return true; // 文件不存在或无法访问
    }
    bool isChanged = (fileMap_[fileName].inode != fileStat.st_ino);
    return isChanged;
}

bool UbsmemLoggerFilesink::RollFile(const std::string &fileName)
{
    ManageFileRotation(fileName);
    time_t currentTime = std::time(nullptr);
    std::string dirForTar = (fileName == "ubsmd.audit") ? auditBasePath_ : basePath_;
    std::string compressedFilename =
        GenerateCompressedFilename(dirForTar, fileName, fileMap_[fileName].fileIndex, currentTime);
    if (!CompressFile(fileName, fileMap_[fileName].filePath, compressedFilename, dirForTar)) {
        std::cerr << "Failed compressing file" << std::endl;
        return false;
    }
    return true;
}

std::string UbsmemLoggerFilesink::GenerateCompressedFilename(const std::string &baseDir, const std::string &fileName,
                                                             uint32_t index, time_t timeStamp)
{
    std::ostringstream oss;

    // 获取当前时间信息
    struct tm timeinfo {
    };
    localtime_r(&timeStamp, &timeinfo);
    struct tm *ptimeinfo = &timeinfo;

    // 生成文件名
    oss << baseDir << "/" << fileName << "_" << std::setw(4) << std::setfill('0') << // 年份格式占4位
        (ptimeinfo->tm_year + 1900) << std::setw(2) << std::setfill('0') << // 月份格式占2位 起始年份1900
        (ptimeinfo->tm_mon + 1) << std::setw(2) << std::setfill('0') << ptimeinfo->tm_mday << // 日期格式占2位
        "_" << std::setw(2) << std::setfill('0') << ptimeinfo->tm_hour <<                     // 小时格式占2位
        std::setw(2) << std::setfill('0') << ptimeinfo->tm_min <<                             // 分钟格式占2位
        std::setw(2) << std::setfill('0') << ptimeinfo->tm_sec <<                             // 秒格式占2位
        "_" << std::setw(3) << std::setfill('0') << index <<                                  // 序号格式占3位
        ".tar.gz";

    return oss.str();
}

bool UbsmemLoggerFilesink::OpenFile(const std::string &fileName)
{
    fs::path filePath = fileMap_[fileName].filePath;
    try {
        fileMap_[fileName].logFile.open(filePath, std::ios::out | std::ios::app);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    if (fileMap_[fileName].logFile.is_open()) {
        struct stat fileStat {
        };
        if (stat(fileMap_[fileName].filePath.c_str(), &fileStat) == 0) {
            fileMap_[fileName].inode = fileStat.st_ino;
        }
        try {
            // 设置权限为640
            fs::permissions(filePath, fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
        return true;
    } else {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
        return false;
    }
}

std::string ShellEscape(const std::string &str)
{
    if (str.empty()) {
        return "''";
    }
    std::string result;
    result += '\'';
    for (char c : str) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += '\'';
    return result;
}

bool UbsmemLoggerFilesink::CompressFile(const std::string &fileName, const std::string &sourceFilename,
                                        const std::string &destFilename, const std::string &baseDir)
{
    std::string command =
        "tar -czf " + ShellEscape(destFilename) + " -C " + ShellEscape(baseDir) + " " + ShellEscape(fileName + ".log");
    std::string result;
    int ret = system(command.c_str());
    if (ret != 0) {
        std::cerr << "Failed to compress file using system command, " << result << std::endl;
        return false;
    }
    try {
        fs::permissions(destFilename, fs::perms::owner_read | fs::perms::group_read); // 设置权限为440
        fs::remove(sourceFilename);
        fileMap_[fileName].logFile.close();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return true;
}

uint32_t UbsmemLoggerFilesink::RenameCompressedFile(std::vector<std::string> &compressedFiles)
{
    uint32_t currentFilecount = compressedFiles.size();
    for (uint32_t i = 0; i < currentFilecount; i++) {
        std::smatch match;
        if (std::regex_search(compressedFiles[i], match, std::regex(R"(_(\d{3})\.tar\.gz)"))) {
            uint32_t newIndex = i + 1;
            std::ostringstream oss;
            oss << std::setw(3) << std::setfill('0') << newIndex; // 匹配压缩包名3位序号

            std::string newFilename = compressedFiles[i];
            newFilename.replace(match.position(1), match.length(1), oss.str());

            try {
                fs::rename(compressedFiles[i], newFilename);
                compressedFiles[i] = std::move(newFilename);
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
    return currentFilecount;
}

void UbsmemLoggerFilesink::ManageFileRotation(const std::string &fileName)
{
    std::vector<std::string> compressedFiles;
    fs::path dir = fileMap_[fileName].filePath.parent_path();

    std::string patternBase = (fileName == "ubsmd.audit") ? auditBasePath_ : basePath_;
    std::regex filenamePattern(patternBase + "/" + fileName + filenameSuffixPattern_);

    try {
        for (const auto &entry : fs::directory_iterator(dir)) {
            if (fs::is_regular_file(entry) && std::regex_match(entry.path().string(), filenamePattern)) {
                compressedFiles.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Failed to list directory for rotation: " << e.what() << std::endl;
        return;
    }

    // 按照文件名中的序号进行排序
    std::sort(compressedFiles.begin(), compressedFiles.end());

    fileMap_[fileName].fileIndex = RenameCompressedFile(compressedFiles) + 1;

    if (compressedFiles.size() < fileMap_[fileName].maxFileCount) {
        return;
    }
    uint32_t currentFilecount = compressedFiles.size();
    for (uint32_t i = 0; i < currentFilecount - fileMap_[fileName].maxFileCount + 1; i++) {
        std::string oldestFile = compressedFiles.front();
        if (!fs::remove(oldestFile)) {
            std::cerr << "Failed to remove old log file: " << oldestFile << std::endl;
        }
        compressedFiles.erase(compressedFiles.begin());
    }

    // 重新命名剩余的文件
    RenameCompressedFile(compressedFiles);

    // 将新文件的索引设置为最大
    fileMap_[fileName].fileIndex = fileMap_[fileName].maxFileCount;
}
} // namespace ubsmem::log