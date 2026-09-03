/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */
#ifndef DAEMON_TEST_COMMON_H
#define DAEMON_TEST_COMMON_H

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>
#include "logger/ubsmem_logger_manager.h"
#include "util/common_headers.h"
#include "util/kv_parser.h"

using namespace ubsmem::log;

namespace UT::Daemon {
const size_t MAX_SIZE = 255;

class DaemonTestCommon {
public:
    static std::string CWD()
    {
        static std::string curDir("");
        if (curDir.empty()) {
            char buf[MAX_SIZE];
            getcwd(buf, MAX_SIZE);
            curDir = std::string(buf);
        }
        return curDir;
    }

    static void CreateConf(const std::string &conf)
    {
        mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
        int ret = 0;
        ret = mkdir((CWD() + "/config").c_str(), mode);
        int fd = open((CWD() + "/config/ubsmd.conf").c_str(), O_RDWR | O_CREAT | O_TRUNC, mode);
        std::string config = conf;
        ret = write(fd, config.data(), config.size());
        close(fd);
        ret = mkdir((CWD() + "/log").c_str(), mode);
    }

    static void DeleteConf()
    {
        ock::common::OckFolderDirDelete(CWD() + "/config");
        ock::common::OckFolderDirDelete(CWD() + "/log");
    }

    static void ClearUlog()
    {
        ubsmem::log::UbsmemLoggerManager::Destroy();
    }

    static void RmLogFile()
    {
        unlink((CWD() + "/log/ockd.log").c_str());
        unlink((CWD() + "/log/ockd.audit.log").c_str());
    }

    static int RecoverUlog()
    {
        return 0;
    }
};
} // namespace UT::Daemon

#endif