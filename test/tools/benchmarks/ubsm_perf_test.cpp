/*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*/

/*
* 提供一种快速测试ubsmd全接口，能测试借用性能的测试工具
* Usage: ./bin/ubsm_perf_test <num_threads> <iterations_per_thread>
* Example: ./bin/ubsm_perf_test 4 10000
* /var/log/ubsm/htrace.dat 性能数据地址
*/

#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include "ubs_mem.h"

static std::atomic_uint32_t g_failedNum{0};
static std::string g_regionName = "g_regionName";
bool ubsmem_lookup_cluster_statistic_test()
{
    ubsmem_cluster_info_t obj{};
    auto res = ubsmem_lookup_cluster_statistic(&obj);
    if (res != 0) {
        printf("ubsmem_lookup_cluster_statistic fail, res is %d\n", res);
        return false;
    }
    for (int i = 0; i < obj.host_num; ++i) {
        for (int j = 0; j < obj.host[i].numa_num; ++j) {
            printf("hostname : %s\t", obj.host[i].host_name);
            printf("socket : %d \t", j);
            printf("mem_total : %llu MB.\t", obj.host[i].numa[j].mem_total / (1ULL * 1024 * 1024));
            printf("mem_free : %llu MB.\t", obj.host[i].numa[j].mem_free / (1ULL * 1024 * 1024));
            printf("mem_borrow : %llu MB.\t", obj.host[i].numa[j].mem_borrow / (1ULL * 1024 * 1024));
            printf("mem_lend : %llu MB.\n", obj.host[i].numa[j].mem_lend / (1ULL * 1024 * 1024));
            printf("---------------------------------------------\n");
        }
    }
    return true;
}

bool ubsmem_create_region_test()
{
    ubsmem_regions_t *regions = new ubsmem_regions_t();
    ubsmem_lookup_regions(regions);
    if (regions->num <= 0) {
        printf("ubsmem_lookup_regions failed \n");
        delete regions;
        return false;
    }
    for (int i = 0; i < regions->region[0].host_num; ++i) {
        regions->region[0].hosts[i].affinity = true;
    }
    auto code = ubsmem_create_region(g_regionName.c_str(), 0, &regions->region[0]);
    if (code != 0) {
        printf("ubsmem_create_region failed \n");
        delete regions;
        return false;
    }
    delete regions;
    return true;
}

int mem_test(size_t threadId, size_t iteration)
{
    if (!ubsmem_lookup_cluster_statistic_test()) {
        g_failedNum.store(g_failedNum.load() + 1);
        return -1;
    }
    std::string name = std::to_string(threadId) + std::to_string(iteration);
    int result = 0;
    void *addr = nullptr;
    result += ubsmem_lease_malloc(g_regionName.c_str(), 1024ULL * 1024 * 1024, DISTANCE_DIRECT_NODE, false, &addr);
    if (addr != nullptr) {
        result += ubsmem_lease_free(addr);
    }
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
    auto createRes =
        ubsmem_shmem_allocate(g_regionName.c_str(), name.c_str(), 1024ULL * 1024 * 1024, mode, UBSM_FLAG_CACHE);
    result += createRes;
    addr = nullptr;
    result += ubsmem_shmem_map(nullptr, 1024ULL * 1024 * 1024, PROT_NONE, MAP_SHARED, name.c_str(), 0, &addr);
    if (addr != nullptr) {
        result += ubsmem_shmem_set_ownership(name.c_str(), addr, 1024ULL * 1024 * 1024, PROT_READ | PROT_WRITE);
        result += ubsmem_shmem_unmap(addr, 1024ULL * 1024 * 1024);
    }
    result += ubsmem_shmem_deallocate(name.c_str());
    if (result != 0) {
        g_failedNum.store(g_failedNum.load() + 1);
    }
    return result;
}

// ========================
// 性能测试函数（单线程）
// ========================
void thread_test(size_t iterations)
{
    for (size_t i = 0; i < iterations; ++i) {
        // 获取当前线程 ID
        auto tid = std::this_thread::get_id();
        // 转换为 size_t（无符号整数）
        std::hash<std::thread::id> hasher;
        size_t tid_hash = hasher(tid);
        mem_test(tid_hash, i);
    }
}

int CheckParam(size_t num_threads, size_t iterations_per_thread)
{
    if (num_threads == 0 || iterations_per_thread == 0) {
        std::cerr << "Error: threads and iterations must be > 0\n";
        return 1;
    }
    ubsmem_options_t options{};
    auto ret = ubsmem_init_attributes(&options);
    if (ret != 0) {
        printf("ubsm init attr error\n");
        return -1;
    }
    ret = ubsmem_initialize(&options);
    if (ret != 0) {
        printf("ubsm sdk lib init error\n");
        return -1;
    }
    std::cout << "(V20250909) Testing function ubs_mem with " << num_threads << " threads, " << iterations_per_thread
              << " iterations per thread.\n";
    return 0;
}

// ========================
// 主函数
// ========================
int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <iterations_per_thread>\n";
        std::cerr << "Example: " << argv[0] << " 4 10000\n";
        return 1;
    }
    size_t num_threads = std::stoul(argv[1]);
    size_t iterations_per_thread = std::stoul(argv[2]);
    if (CheckParam(num_threads, iterations_per_thread) != 0) {
        return 1;
    }
    std::vector<std::thread> threads;
    sleep(2u);
    if (!ubsmem_create_region_test()) {
        return -1;
    }
    printf("Start all threads...\n");
    // 启动所有线程
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_test, iterations_per_thread);
    }
    // 等待所有线程结束
    for (auto &t : threads) {
        t.join();
    }
    ubsmem_destroy_region(g_regionName.c_str());
    ubsmem_finalize();
    if (g_failedNum != 0) {
        std::cout << "mem_test failed." << std::endl;
    } else {
        std::cout << "mem_test successful." << std::endl;
    }
    return 0;
}