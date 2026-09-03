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

#ifndef OCK_MEM_REGION_MANAGER_H
#define OCK_MEM_REGION_MANAGER_H

#include <map>
#include <mutex>
#include <string>

#include "log.h"
#include "region_repository.h"

namespace ock::share::service {

class RegionManager {
public:
    static RegionManager &GetInstance()
    {
        static RegionManager instance;
        return instance;
    }

    static int32_t Initial();

    bool CreateRegionInfo(const RegionInfo &region)
    {
        std::lock_guard<std::mutex> lock(mapMutex);
        if (mRegionMap.find(region.name) == mRegionMap.end()) {
            mRegionMap[region.name] = region;
        } else {
            DBG_LOGERROR("The region has already been created, name=" << region.name);
            return false;
        }
        bool res = RegionRepository::GetInstance().UpdateRegionInfo(region);
        if (!res) {
            mRegionMap.erase(region.name);
            DBG_LOGERROR("Failed to record region, name=" << region.name);
            return false;
        }
        return true;
    }

    bool DeleteRegionInfo(const std::string name)
    {
        std::lock_guard<std::mutex> lock(mapMutex);
        if (mRegionMap.find(name) != mRegionMap.end()) {
            RegionInfo region = mRegionMap[name];
            mRegionMap.erase(name);
            bool res = RegionRepository::GetInstance().RemoveRegionInfo(name);
            if (!res) {
                mRegionMap[region.name] = region;
                DBG_LOGERROR("Failed to Remove region, name=" << region.name);
                return false;
            }
            return true;
        } else {
            DBG_LOGERROR("The region is not find, name=" << name);
            return false;
        }
    }

    bool GetRegionInfo(const std::string &name, RegionInfo &region)
    {
        std::lock_guard<std::mutex> lock(mapMutex);
        if (mRegionMap.find(name) != mRegionMap.end()) {
            region = mRegionMap[name];
            return true;
        } else {
            DBG_LOGERROR("The region has not been find, name=" << name);
            return false;
        }
    }

    static int32_t Recovery();

private:
    RegionManager() {}

    bool RecoverFromRamFS();

    RegionManager(const RegionManager &other) = delete;
    RegionManager(RegionManager &&other) = delete;
    RegionManager &operator=(const RegionManager &other) = delete;
    RegionManager &operator=(RegionManager &&other) noexcept = delete;

    std::mutex mapMutex;
    std::map<std::string, RegionInfo> mRegionMap;
};
} // namespace ock::share::service
#endif // OCK_MEM_REGION_MANAGER_H