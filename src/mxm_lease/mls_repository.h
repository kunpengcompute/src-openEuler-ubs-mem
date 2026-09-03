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
#ifndef OCK_MEM_LEASE_REPOSITORY_H
#define OCK_MEM_LEASE_REPOSITORY_H

#include <unistd.h>
#include <string>
#include "record_store.h"

namespace ock::lease::service {

class MLSRepository {
public:
    static MLSRepository &GetInstance()
    {
        static MLSRepository instance;
        return instance;
    }

    static int32_t Init();
    int32_t PreAddMemRecord(const ock::ubsm::LeaseMallocInput &input);
    int32_t UpdateMemResultRecord(const std::string &name, const ock::ubsm::LeaseMallocResult &result);
    int32_t UpdateMemRecordState(const std::string &name, const ock::ubsm::RecordState &state);
    int32_t AddMemRecord(const ock::ubsm::MemLeaseInfo &info);
    int32_t UpdateMemRecord(const std::string &name, const ock::ubsm::AppContext &context, uint64_t size);
    int32_t DeleteMemRecord(const std::string &name);
    std::vector<ock::ubsm::MemLeaseInfo> RecoverMemRecord();

private:
    MLSRepository() = default;
    MLSRepository(const MLSRepository &other) = delete;
    MLSRepository(MLSRepository &&other) = delete;
    MLSRepository &operator=(const MLSRepository &other) = delete;
    MLSRepository &operator=(MLSRepository &&other) noexcept = delete;
};

} // namespace ock::lease::service
#endif