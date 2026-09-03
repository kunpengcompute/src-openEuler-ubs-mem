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

#ifndef OCK_MEM_REGION_REPOSITORY_H
#define OCK_MEM_REGION_REPOSITORY_H

#include <string>
#include <vector>
#include "mx_shm.h"

namespace ock::share::service {

struct RegionInfo {
    std::string name;
    uint64_t size;
    SHMRegionDesc region;
    RegionInfo() noexcept = default;
    RegionInfo(std::string nm, uint64_t sz, SHMRegionDesc rg) : name{nm}, size{sz}, region{rg} {}
};

class RegionRepository {
public:
    static RegionRepository &GetInstance()
    {
        static RegionRepository instance;
        return instance;
    }

    bool Initial();

    bool UpdateRegionInfo(const RegionInfo &region);

    bool RemoveRegionInfo(const std::string name);

    bool Recovery(std::vector<RegionInfo> &regions);

private:
    RegionRepository() = default;
    RegionRepository(const RegionRepository &other) = delete;
    RegionRepository(RegionRepository &&other) = delete;
    RegionRepository &operator=(const RegionRepository &other) = delete;
    RegionRepository &operator=(RegionRepository &&other) noexcept = delete;
};
} // namespace ock::share::service
#endif // OCK_MEM_REGION_REPOSITORY_H