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

#include "region_manager.h"
#include "rack_mem_err.h"
#include "util/functions.h"

namespace ock::share::service {

using namespace ock::common;

int32_t RegionManager::Initial()
{
    bool ret = RegionRepository::GetInstance().Initial();
    if (!ret) {
        DBG_LOGERROR("SHMRepository Initial failed");
        return MXM_ERR_PARAM_INVALID;
    }
    return HOK;
}

bool RegionManager::RecoverFromRamFS()
{
    std::vector<RegionInfo> regions;

    bool ret = RegionRepository::GetInstance().Recovery(regions);
    if (!ret) {
        DBG_LOGERROR("Recovery failed");
        return false;
    }

    for (const auto &elem : regions) {
        if (mRegionMap.find(elem.name) != mRegionMap.end()) {
            DBG_LOGERROR("The region has already recoverd!" << elem.name);
            return false;
        }
        mRegionMap[elem.name] = elem;
    }

    return true;
}

int32_t RegionManager::Recovery()
{
    bool ret = GetInstance().RecoverFromRamFS();
    if (!ret) {
        DBG_LOGERROR("Recovery failed ");
        return MXM_ERR_PARAM_INVALID;
    }
    return HOK;
}
} // namespace ock::share::service