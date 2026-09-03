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
#include "numa_cpu_utils.h"

#include <numa.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "log.h"

namespace ock {
namespace common {
const int BITS_PER_BYTE = 8;
const int HEX_WIDTH = 2; // 十六进制输出的宽度
NumaCpuUtils::CpuAffinityInfo NumaCpuUtils::SchedGetaffinityGetCpuList(pid_t pid)
{
    CpuAffinityInfo info;
    info.pid = pid;
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    if (sched_getaffinity(pid, sizeof(cpu_set_t), &cpu_mask) == -1) {
        DBG_LOGERROR("sched_getaffinity failed for PID " << pid);
        return info;
    }
    static int total_cpus = sysconf(_SC_NPROCESSORS_CONF);
    if (total_cpus <= 0) {
        total_cpus = CPU_SETSIZE;
    }
    total_cpus = std::min(total_cpus, CPU_SETSIZE);
    for (int cpu = 0; cpu < total_cpus; ++cpu) {
        if (CPU_ISSET(cpu, &cpu_mask)) {
            info.cpuList.push_back(cpu);
        }
    }
    if (info.cpuList.size() == total_cpus) {
        info.cpuList.clear();
        DBG_LOGWARN("pid=" << pid << "is not bound core");
        return info;
    }
    info.hexMask = CpuSetToHexString(&cpu_mask, total_cpus);
    DBG_LOGINFO("SchedGetaffinityGetCpuList success for PID " << pid);
    return info;
}

NumaCpuUtils::CpuNodeMapping NumaCpuUtils::NumaNodeOfCpuGetMapping(int cpuId)
{
    CpuNodeMapping mapping;
    mapping.cpuId = cpuId;
    // 获取最大CPU数量
    static int maxCpuId = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (cpuId < 0 || cpuId > maxCpuId) {
        mapping.numaNode = -1;
        return mapping;
    }
    // 调用 numa_node_of_cpu 获取节点映射
    mapping.numaNode = numa_node_of_cpu(cpuId);
    return mapping;
}

std::vector<NumaCpuUtils::CpuNodeMapping> NumaCpuUtils::GetCpusNumaMapping(const std::vector<int> &cpuLists)
{
    std::vector<CpuNodeMapping> mappings;

    for (int cpuId : cpuLists) {
        mappings.push_back(NumaNodeOfCpuGetMapping(cpuId));
    }
    DBG_LOGINFO("GetCpusNumaMapping end, numa mapping size: " << mappings.size());
    return mappings;
}

std::string NumaCpuUtils::CpuSetToHexString(cpu_set_t *mask, int num_cpus)
{
    std::stringstream ss;
    ss << "0x";

    int bytes_needed = (num_cpus + 7) / 8;
    bool found_non_zero = false;

    for (int byte = bytes_needed - 1; byte >= 0; --byte) {
        unsigned char value = 0;
        for (int bit = 0; bit < BITS_PER_BYTE; ++bit) {
            int cpu = byte * BITS_PER_BYTE + bit;
            if (cpu < num_cpus && CPU_ISSET(cpu, mask)) {
                value |= (1 << bit);
            }
        }
        if (value != 0 || found_non_zero || byte == 0) {
            ss << std::hex << std::setw(HEX_WIDTH) << std::setfill('0') << static_cast<int>(value);
            found_non_zero = true;
        }
    }

    return ss.str();
}

std::set<int> NumaCpuUtils::GetAppBoundNumaNodes(pid_t pid)
{
    std::set<int> boundNodes;
    try {
        CpuAffinityInfo affinityInfo = SchedGetaffinityGetCpuList(pid);
        if (affinityInfo.cpuList.empty()) {
            DBG_LOGINFO("No CPUs found in affinity info for PID: %d", pid);
            return boundNodes;
        }
        auto numaMappings = GetCpusNumaMapping(affinityInfo.cpuList);
        if (numaMappings.empty()) {
            DBG_LOGINFO("No NUMA mappings found for CPUs");
            return boundNodes;
        }
        for (const auto &mapping : numaMappings) {
            if (mapping.numaNode != -1) {
                DBG_LOGDEBUG("valid NUMA nodes found in mappings, node: " << mapping.numaNode);
                boundNodes.insert(mapping.numaNode);
            }
        }
        if (boundNodes.empty()) {
            DBG_LOGINFO("No valid NUMA nodes found in mappings");
        }
    } catch (const std::exception &e) {
        DBG_LOGINFO("Failed to get app bound NUMA nodes for PID %d: %s", pid, e.what());
    }
    DBG_LOGINFO("GetAppBoundNumaNodes success, bound numaNode num: " << boundNodes.size());
    return boundNodes;
}

int NumaCpuUtils::IsAppBoundToOneNumaNode(pid_t pid)
{
    try {
        std::set<int> boundNodes = GetAppBoundNumaNodes(pid);
        if (boundNodes.size() == 1) {
            auto node = *boundNodes.begin();
            DBG_LOGINFO("App  PID: " << pid << " is bound to NUMA node: " << node);
            return node;
        }
        DBG_LOGINFO("App  PID: " << pid << " is not bound to one NUMA node.");
        return -1;
    } catch (const std::exception &e) {
        DBG_LOGERROR("Failed to check app NUMA binding, PID: " << pid);
        return -1;
    }
}
} // namespace common
} // namespace ock