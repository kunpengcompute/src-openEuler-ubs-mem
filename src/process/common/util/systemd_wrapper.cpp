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
#include <systemd/sd-daemon.h>

#include "log.h"
#include "systemd_wrapper.h"

namespace ock {
namespace common {
namespace systemd {
using namespace ock::common;

bool GetWatchdogTimeout(std::chrono::microseconds &timeout) noexcept
{
    uint64_t keepAliveUs = 0;
    auto res = sd_watchdog_enabled(0, &keepAliveUs);
    if (res < 0) {
        DBG_LOGWARN("Get watch dog status failed( " << res << ")");
        return false;
    }

    if (res == 0) {
        DBG_LOGINFO("watch dog not enabled.");
        return false;
    }

    timeout = std::chrono::microseconds(keepAliveUs);
    return true;
}

int Notify(const std::string &message) noexcept
{
    return sd_notify(0, message.c_str());
}

int NotifyReady() noexcept
{
    return Notify("READY=1");
}

int NotifyStopping() noexcept
{
    return Notify("STOPPING=1");
}

int NotifyWatchdog(const std::string &status) noexcept
{
    std::string message("WATCHDOG=1");
    if (!status.empty()) {
        message.append("\nSTATUS=").append(status);
    }

    return Notify(message);
}

int StoreFd(const std::string &name, int fd) noexcept
{
    auto message = std::string("FDSTORE=1\nFDNAME=").append(name);
    auto ret = sd_pid_notify_with_fds(0, 0, message.c_str(), &fd, 1);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int LoadFd(const std::string &name, int &fd) noexcept
{
    char **restores = nullptr;
    auto n = sd_listen_fds_with_names(1, &restores);
    if (n < 0) {
        DBG_LOGERROR("sd_listen_fds_with_names failed: " << strerror(-n));
        return -1;
    }

    for (int i = 0; i < n; ++i) {
        int cur = SD_LISTEN_FDS_START + i;
        if (name == restores[i]) {
            fd = cur;
            free(restores);
            return 0;
        }
    }

    free(restores);
    DBG_LOGINFO("sd_listen_fds_with_names get fd count: " << n << ", not matches " << name);
    return -1;
}

} // namespace systemd
} // namespace common
} // namespace ock
