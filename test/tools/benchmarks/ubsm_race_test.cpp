/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 */
#include "ubs_mem.h"

#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

constexpr auto SHARE_MEMORY_NAME = "ubsm_race_test";
constexpr auto SYS_MEMORY_NAME = "sys_race_test";

int g_iterations = 0;
size_t g_shareMemoryLength = 0;
size_t g_shareMemoryCreateFlags = 0;
int g_logLevel = 0;

std::atomic<bool> g_shareMemoryThreadRunning(true);

constexpr char MAGIC = 0xac;

int UbsMemShareMemoryMapper(const std::string &name)
{
    void *result = nullptr;
    auto ret =
        ubsmem_shmem_map(nullptr, g_shareMemoryLength, PROT_READ | PROT_WRITE, MAP_SHARED, name.c_str(), 0, &result);
    if (ret != 0) {
        std::cerr << "Failed to map " << name << ", ret=" << ret << std::endl;
        return ret;
    }

    ret = memset_s(result, g_shareMemoryLength, MAGIC, g_shareMemoryLength);
    if (ret != 0) {
        std::cerr << "Failed to write share memory" << name << ", ret=" << ret << std::endl;
        return ret;
    }

    volatile auto arr = static_cast<char *>(result);
    for (int i = 0; i < g_shareMemoryLength; i++) {
        if (arr[i] != MAGIC) {
            std::cerr << "Failed to read share memory " << name << std::endl;
            std::cerr << "index: " << i << std::endl;
            std::cerr << "expect: 0x" << std::hex << MAGIC << std::dec << std::endl;
            std::cerr << "actual: 0x" << std::hex << arr[i] << std::dec << std::endl;
            return 1;
        }
    }

    ret = ubsmem_shmem_unmap(result, g_shareMemoryLength);
    if (ret != 0) {
        std::cerr << "Failed to unmap " << name << ", ret=" << ret << std::endl;
        return ret;
    }

    return 0;
}

int UbsMemShareMemoryMapFrequently(const std::string &name)
{
    for (int i = 0; i < g_iterations; ++i) {
        auto ret = UbsMemShareMemoryMapper(name);
        if (ret != 0) {
            std::cerr << "Failed to use " << name << ", index=" << i << ", ret=" << ret << std::endl;
            return ret;
        }
    }
    std::cout << "Share memory stress test successfully. name=" << name << std::endl;
    g_shareMemoryThreadRunning.store(false);
    return 0;
}

int SystemMemoryMapper(const std::string &name)
{
    int fd = memfd_create(name.c_str(), MFD_CLOEXEC);
    if (fd < 0) {
        std::cerr << "memfd_create failed. errno=" << errno << std::endl;
        return -1;
    }
    auto ret = ftruncate(fd, static_cast<off_t>(g_shareMemoryLength));
    if (ret != 0) {
        std::cerr << "ftruncate failed. errno=" << errno << std::endl;
        return ret;
    }
    void *address = mmap(nullptr, g_shareMemoryLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == MAP_FAILED) {
        std::cerr << "mmap failed. errno=" << errno << std::endl;
        return errno;
    }

    ret = memset_s(address, g_shareMemoryLength, MAGIC, g_shareMemoryLength);
    if (ret != 0) {
        std::cerr << "Failed to write fd memory " << fd << ", ret=" << ret << std::endl;
        return ret;
    }

    volatile auto arr = static_cast<char *>(address);
    for (int i = 0; i < g_shareMemoryLength; i++) {
        if (arr[i] != MAGIC) {
            std::cerr << "Failed to read fd memory " << fd << std::endl;
            std::cerr << "index: " << i << std::endl;
            std::cerr << "expect: 0x" << std::hex << MAGIC << std::dec << std::endl;
            std::cerr << "actual: 0x" << std::hex << arr[i] << std::dec << std::endl;
            return 1;
        }
    }

    ret = munmap(address, g_shareMemoryLength);
    if (ret != 0) {
        std::cerr << "munmap failed. errno=" << errno << std::endl;
        return errno;
    }
    close(fd);
    return 0;
}

int SystemMemoryMapFrequently(const std::string &name)
{
    int i = 0;
    while (g_shareMemoryThreadRunning) {
        auto tmp = name + "_" + std::to_string(i);
        auto ret = SystemMemoryMapper(tmp);
        if (ret != 0) {
            std::cerr << "SystemMemoryMapper failed. errno=" << errno << " name=" << tmp << std::endl;
            return ret;
        }
        ++i;
    }
    std::cout << "System memory map worker run successfully." << std::endl;
    std::cout << "system mmap unmap " << i << " times" << std::endl;
    return 0;
}

int CreateShareMemory()
{
    auto ret = ubsmem_shmem_allocate("default", SHARE_MEMORY_NAME, g_shareMemoryLength, 0600, g_shareMemoryCreateFlags);
    if (ret != 0 && ret != UBSM_ERR_ALREADY_EXIST) {
        std::cerr << "Failed to allocate " << SHARE_MEMORY_NAME << ", ret=" << ret << std::endl;
        return ret;
    }
    std::cout << "Create share memory successfully." << std::endl;
    return 0;
}

int DestroyShareMemory()
{
    auto ret = ubsmem_shmem_deallocate(SHARE_MEMORY_NAME);
    if (ret != 0) {
        std::cerr << "Failed to deallocate " << SHARE_MEMORY_NAME << ", ret=" << ret << std::endl;
        return ret;
    }
    std::cout << "Destroy share memory end." << std::endl;
    return 0;
}

int ParseParameters(int argc, char *argv[])
{
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <iterations> <length> <createFlags> <logLevel>" << std::endl;
        return 1;
    }
    try {
        g_iterations = std::stoi(argv[1]);
        g_shareMemoryLength = std::stoull(argv[2]);
        g_shareMemoryCreateFlags = std::stoull(argv[3]);
        g_logLevel = std::stoi(argv[4]);
    } catch (...) {
        std::cerr << "Please enter integer value." << std::endl;
        return 1;
    }
    std::cout << "Parse parameters successfully." << std::endl;
    return 0;
}

int InitializeUbsMem()
{
    auto ret = ubsmem_set_logger_level(g_logLevel);
    if (ret != 0) {
        std::cerr << "ubsmem_set_logger_level failed. ret=" << ret << std::endl;
        return ret;
    }
    ubsmem_options_t options{};
    ret = ubsmem_init_attributes(&options);
    if (ret != 0) {
        std::cerr << "ubsmem_init_attributes failed, ret=" << ret << std::endl;
        return ret;
    }
    ret = ubsmem_initialize(&options);
    if (ret != 0) {
        std::cerr << "ubsmem_initialize failed, ret=" << ret << std::endl;
        return ret;
    }
    std::cout << "UbsMem initialize successfully." << std::endl;
    return 0;
}

int RunTestCase()
{
    std::thread shmMapperThread(UbsMemShareMemoryMapFrequently, SHARE_MEMORY_NAME);
    std::thread sysMapperThread(SystemMemoryMapFrequently, SYS_MEMORY_NAME);

    shmMapperThread.join();
    sysMapperThread.join();

    return 0;
}

int main(int argc, char *argv[])
{
    auto ret = ParseParameters(argc, argv);
    if (ret != 0) {
        return ret;
    }

    ret = InitializeUbsMem();
    if (ret != 0) {
        return ret;
    }

    ret = CreateShareMemory();
    if (ret != 0) {
        std::cout << "Failed to create share memory. " << std::endl;
        return 1;
    }

    ret = RunTestCase();
    if (ret != 0) {
        std::cout << "Failed to run test case. " << std::endl;
        DestroyShareMemory();
        return ret;
    }

    return DestroyShareMemory();
}
