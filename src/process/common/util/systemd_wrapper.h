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
#ifndef OCK_COMMON_UTIL_SYSTEMD_WRAPPER_H
#define OCK_COMMON_UTIL_SYSTEMD_WRAPPER_H

#include <string>

namespace ock {
namespace common {
namespace systemd {
/**
 * @brief 获取systemd配置的超时时长，超过此时长不发送心跳消息，进程就会被重启
 *
 * @param timeout [out] 超时时长的值
 *
 * @return 成功时返回 true，失败时返回 false。
 * @note 如果进程不是通过systemd启动，或systemd配置方式不是notify，则返回失败。
 */
bool GetWatchdogTimeout(std::chrono::microseconds &timeout) noexcept;

/**
 * @brief 向systemd传递信息
 *
 * @param message 要传递的信息内容
 *
 * @return 成功时返回0，失败时返回非0
 */
int Notify(const std::string &message) noexcept;

/**
 * @brief 发送信息内容为"READY=1"，通知进程启动完成
 *
 * @return 成功时返回0，失败时返回非0
 */
int NotifyReady() noexcept;

/**
 * @brief 发送信息内容为"STOPPING=1"，通知进程正在停止
 *
 * @return 成功时返回0，失败时返回非0
 */
int NotifyStopping() noexcept;

/**
 * @brief 发送信息内容为"WATCHDOG=1"，心跳消息
 * @param status 携带的状态信息，默认不带
 * @return 成功时返回0，失败时返回非0
 */
int NotifyWatchdog(const std::string &status = "") noexcept;

/**
 * @brief 向systemd保存一个文件描述符
 * @param name 用于通过以名称找回描述符
 * @param fd 要保存的文件描述符
 * @return 成功时返回0，失败时返回非0
 */
int StoreFd(const std::string &name, int fd) noexcept;

/**
 * @brief 从systemd找回保存的文件描述符
 * @param name 用于通过以名称找回描述符
 * @param fd 得到的文件描述符
 * @return 成功时返回0，失败时返回非0
 */
int LoadFd(const std::string &name, int &fd) noexcept;
} // namespace systemd
} // namespace common
} // namespace ock

#endif // UTIL_SYSTEMD_WRAPPER_H
