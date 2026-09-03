/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#include "ubse_mem_adapter_stub.h"
#include <fcntl.h>
#include <unistd.h>

namespace UT {
int32_t LookupRegionListStub(SHMRegions &regions)
{
    printf("in LookupRegionListStub\n");
    regions.num = 1;
    regions.region[0].num = 1;
    regions.region[0].type = ALL2ALL_SHARE;
    regions.region[0].affinity[0] = true;
    std::string nodeId = "1";
    auto ret = strcpy_s(regions.region[0].nodeId[0], MEM_MAX_ID_LENGTH, nodeId.c_str());
    if (ret != EOK) {
        return ret;
    }
    return 0;
}

int ShmImportStub(const ock::mxm::ImportShmParam &param, ock::mxm::AttachShmResult &result)
{
    result.memIds.emplace_back(0);
    result.shmSize = 128 * 1024 * 1024;
    result.unitSize = 128 * 1024 * 1024;
    return 0;
}

int ShmCreateStub(const ock::mxm::CreateShmParam &param)
{
    printf("in ShmCreateStub\n");
    return 0;
}

int ShmDeleteStub(const std::string &name, const ock::ubsm::AppContext &appContext)
{
    printf("in ShmDeleteStub\n");
    return 0;
}

int ShmGetUserDataStub(const std::string &name, ubs_mem_shm_desc_t &shmDes)
{
    printf("in ShmGetUserDataStub\n");
    return 0;
}

int MemFdCreateWithCandidateStub(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                 const uint32_t *slot_ids, uint32_t slot_cnt, ubs_mem_fd_desc_t *fd_desc)
{
    printf("in MemFdCreateWithCandidateStub\n");
    if (fd_desc != nullptr) {
        fd_desc->memid_cnt = 1;
        fd_desc->memids[0] = 0;
    }
    return 0;
}

int32_t MemFdCreateWithLenderStub(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                  const ubs_mem_lender_t *lender, uint32_t lender_cnt, ubs_mem_fd_desc_t *fd_desc)
{
    printf("in MemFdCreateWithLenderStub\n");
    fd_desc->memid_cnt = 1;
    fd_desc->memids[0] = 0;
    return 0;
}

int FdPermissionChangeStub(const std::string &name, const ock::ubsm::AppContext &appContext, mode_t mode)
{
    printf("in FdPermissionChangeStub\n");
    return 0;
}
} // namespace UT
