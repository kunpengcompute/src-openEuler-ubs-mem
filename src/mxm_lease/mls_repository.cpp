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
#include "mls_repository.h"

namespace ock::lease::service {
using namespace ock::ubsm;

int32_t MLSRepository::Init()
{
    return 0;
}

int32_t MLSRepository::PreAddMemRecord(const ock::ubsm::LeaseMallocInput &input)
{
    return ock::ubsm::RecordStore::GetInstance().AddMemLeaseInput(input);
}

int32_t MLSRepository::UpdateMemResultRecord(const std::string &name, const ock::ubsm::LeaseMallocResult &result)
{
    return ock::ubsm::RecordStore::GetInstance().AddMemLeaseResult(name, result);
}

int32_t MLSRepository::UpdateMemRecordState(const std::string &name, const ock::ubsm::RecordState &state)
{
    return ock::ubsm::RecordStore::GetInstance().UpdateMemLeaseRecordState(name, state);
}

int32_t MLSRepository::AddMemRecord(const MemLeaseInfo &info)
{
    return RecordStore::GetInstance().AddMemLeaseRecord(info);
}

int32_t MLSRepository::UpdateMemRecord(const std::string &name, const AppContext &context, uint64_t size)
{
    return RecordStore::GetInstance().UpdateMemLeaseRecord(name, context, size);
}

int32_t MLSRepository::DeleteMemRecord(const std::string &name)
{
    return RecordStore::GetInstance().DelMemLeaseRecord(name);
}

std::vector<MemLeaseInfo> MLSRepository::RecoverMemRecord()
{
    return RecordStore::GetInstance().ListMemLeaseRecord();
}

} // namespace ock::lease::service