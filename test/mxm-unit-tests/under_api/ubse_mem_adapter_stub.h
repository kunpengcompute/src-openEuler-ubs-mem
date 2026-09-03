/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifndef UBSE_MEM_MXMD_STUB_H
#define UBSE_MEM_MXMD_STUB_H

#include <securec.h>
#include <sys/types.h>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include "mx_shm.h"
#include "under_api/ubse/ubse_mem_adapter.h"

namespace UT {

int32_t LookupRegionListStub(SHMRegions &regions);
int ShmImportStub(const ock::mxm::ImportShmParam &param, ock::mxm::AttachShmResult &result);
int ShmCreateStub(const ock::mxm::CreateShmParam &param);
int ShmDeleteStub(const std::string &name, const ock::ubsm::AppContext &appContext);
int ShmGetUserDataStub(const std::string &name, ubs_mem_shm_desc_t &shmDes);
int MemFdCreateWithCandidateStub(const char *name, uint64_t size, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                 const uint32_t *slot_ids, uint32_t slot_cnt, ubs_mem_fd_desc_t *fd_desc);
int32_t MemFdCreateWithLenderStub(const char *name, const ubs_mem_fd_owner_t *owner, mode_t mode,
                                  const ubs_mem_lender_t *lender, uint32_t lender_cnt, ubs_mem_fd_desc_t *fd_desc);
int FdPermissionChangeStub(const std::string &name, const ock::ubsm::AppContext &appContext, mode_t mode);

} // namespace UT

#endif