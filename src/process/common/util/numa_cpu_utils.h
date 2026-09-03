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
#ifndef NUMA_CPU_UTILS_H
#define NUMA_CPU_UTILS_H

#include <numa.h>
#include <sched.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ock {
namespace common {
class NumaCpuUtils {
public:
    // pid->对应的CPU list
    struct CpuAffinityInfo {
        pid_t pid;
        std::vector<int> cpuList;
        std::string hexMask;

        CpuAffinityInfo() : pid(0) {}
    };

    // CPU->对应的numa节点
    struct CpuNodeMapping {
        int cpuId;    // CPU核心ID
        int numaNode; // 对应的NUMA节点

        CpuNodeMapping() : cpuId(-1), numaNode(-1) {}
    };

    // 获取进程的CPU亲和性列表
    static CpuAffinityInfo SchedGetaffinityGetCpuList(pid_t pid = 0);

    // 获取CPU对应的NUMA节点
    static CpuNodeMapping NumaNodeOfCpuGetMapping(int cpuId);

    // 批量获取多个CPU的NUMA节点映射
    static std::vector<CpuNodeMapping> GetCpusNumaMapping(const std::vector<int> &cpuLists);

    // 获取当前应用绑定的NUMA节点列表
    static std::set<int> GetAppBoundNumaNodes(pid_t pid = 0);

    // 检查应用是否绑定到单个NUMA节点
    static int IsAppBoundToOneNumaNode(pid_t pid = 0);

private:
    // 将cpu_set_t转换为十六进制字符串
    static std::string CpuSetToHexString(cpu_set_t *mask, int num_cpus);
};
} // namespace common
} // namespace ock

#endif // NUMA_CPU_UTILS_H