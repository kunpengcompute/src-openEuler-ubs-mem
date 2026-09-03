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
#include <fcntl.h>
#include <unistd.h>
#include "ock_daemon.h"
#include "syslog.h"

using namespace ock::daemon;

bool CheckIsRunning()
{
    std::string filePath = "/tmp/matrix_mem_daemon";
    std::string fileName = filePath + ".lock";
    int fd = open(fileName.c_str(), O_WRONLY | O_CREAT, 0600);
    if (fd < 0) {
        std::cerr << "Open file " << fileName.c_str() << " failed, error message is " << strerror(errno) << "."
                  << std::endl;
        return true;
    }
    flock lock{};
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    auto ret = fcntl(fd, F_SETLK, &lock);
    if (ret < 0) {
        std::cerr << "Fail to start ubsmd, process lock file is locked." << std::endl;
        close(fd);
        return true;
    }
    close(fd);
    return false;
}

int main(int argc, char *argv[])
{
    auto start = std::chrono::steady_clock::now();
    auto uid = getuid();
    auto gid = getgid();
#if defined(_DEBUG) || defined(DEBUG)
    syslog(LOG_INFO, "(D)The current uid is %d, gid is %d.", uid, gid);
#else
    syslog(LOG_INFO, "(R)The current uid is %d, gid is %d.", uid, gid);
#endif
    if (argc != ARGS_NUM || argv == nullptr) {
        std::cerr << "Error, invalid parameters." << std::endl;
        return ERROR_EXIT_CODE;
    }
    if (CheckIsRunning()) {
        return ERROR_EXIT_CODE;
    }
    ock::daemon::OCKDaemonPtr daemon = new (std::nothrow) ock::daemon::OckDaemon();
    if (daemon == nullptr) {
        std::cerr << "Failed to new ock daemon, maybe out of memory" << std::endl;
        return ERROR_EXIT_CODE;
    }
    if (daemon->CheckParam(argv[BIN_PATH_POSITION]) != 0) {
        daemon.Set(nullptr);
        return ERROR_EXIT_CODE;
    }
    if (daemon->Initialize() != 0) {
        daemon.Set(nullptr);
        return ERROR_EXIT_CODE;
    }
    if (daemon->Start(start) != 0) {
        DBG_LOGERROR("Failed to start OckDaemon");
        daemon.Set(nullptr);
        ock::daemon::OckDaemon::PrintStartTime(start, "START_FAILED");
        return ERROR_EXIT_CODE;
    }
    if (daemon->Uninitialize() != 0) {
        daemon.Set(nullptr);
        DBG_LOGERROR("Failed to uninitialize ock daemon");
        return ERROR_EXIT_CODE;
    }
    daemon.Set(nullptr);
    ock::daemon::OckDaemon::PrintStartTime(start, "STOP");
    return CLEAN_EXIT_CODE;
}