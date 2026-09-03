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
#include <securec.h>
#include "log.h"
#include "record_store.h"
#include "ubse_mem_adapter.h"

#include "region_repository.h"
namespace ock::share::service {
using namespace ock::ubsm;
using namespace ock::mxmd;

bool ToNodeIds(const SHMRegionDesc &region, std::vector<uint32_t> &nodeIds, std::vector<bool> &affinitys)
{
    if (region.num > MEM_TOPOLOGY_MAX_HOSTS) {
        DBG_AUDITERROR("num is " << region.num << ", max is " << MEM_TOPOLOGY_MAX_HOSTS);
        return false;
    }
    for (int i = 0; i < region.num; i++) {
        uint32_t nodeId{1u};
        if (!StrUtil::StrToUint(std::string(region.nodeId[i]), nodeId)) {
            DBG_LOGERROR("Invalid node ID, nodeId[" << i << "]: " << region.nodeId[i]);
            return false;
        }
        nodeIds.push_back(nodeId);
        affinitys.push_back(region.affinity[i]);
    }
    return true;
}

static bool CheckRegionMatched(const CreateRegionInput &input, RegionInfo &output, const SHMRegionDesc &example)
{
    int i;
    bool flag[MEM_TOPOLOGY_MAX_HOSTS] = {false};
    bool affinity[MEM_TOPOLOGY_MAX_HOSTS] = {false};
    if (example.num > MEM_TOPOLOGY_MAX_HOSTS) {
        DBG_LOGERROR("Impossible.");
        return false;
    }

    for (int j = 0; j < input.nodeIds.size(); j++) {
        for (i = 0; i < example.num; i++) {
            if (flag[i]) {
                continue;
            }
            uint32_t nodeId{1u};
            if (!StrUtil::StrToUint(std::string(example.nodeId[i]), nodeId)) {
                DBG_LOGERROR("Invalid node ID, nodeId[" << i << "]: " << example.nodeId[i]);
                return false;
            }
            if (nodeId == input.nodeIds[j]) {
                flag[i] = true;
                affinity[i] = input.affinity[j];
                break;
            }
        }
        DBG_LOGINFO("CheckRegionMatched nodeId is " << input.nodeIds[j] << ",affinity is " << input.affinity[j]);
        if (i == example.num) {
            return false;
        }
    }

    output.region.perfLevel = example.perfLevel;
    output.region.type = example.type;
    int num = 0;
    for (i = 0; i < example.num; i++) {
        if (!flag[i]) {
            continue;
        }
        auto ret = strcpy_s(output.region.nodeId[num], MEM_MAX_ID_LENGTH, example.nodeId[i]);
        if (ret != 0) {
            DBG_LOGERROR("node id copy error, ret:" << ret);
            return false;
        }
        ret = strcpy_s(output.region.hostName[num], MAX_HOST_NAME_DESC_LENGTH, example.hostName[i]);
        if (ret != 0) {
            DBG_LOGERROR("host name copy error, ret:" << ret);
            return false;
        }
        output.region.affinity[num] = affinity[i];
        num++;
    }
    output.region.num = num;
    return true;
}

int TransRecordToRegionCache(const CreateRegionInput &input, RegionInfo &region, const SHMRegions &regions)
{
    region.name = input.name;
    region.size = input.size;

    for (int i = 0; i < regions.num; i++) {
        if (CheckRegionMatched(input, region, regions.region[i])) {
            return 0;
        }
    }

    DBG_LOGERROR("find designated region fail.");
    return -1;
}

bool RegionRepository::Initial()
{
    return true;
}

bool RegionRepository::UpdateRegionInfo(const RegionInfo &region)
{
    std::vector<uint32_t> nodeIds;
    std::vector<bool> affinitys;
    auto ret = ToNodeIds(region.region, nodeIds, affinitys);
    if (!ret) {
        DBG_LOGERROR("ToNodeIds failed.");
        return false;
    }

    CreateRegionInput input(region.name, region.size, nodeIds, affinitys);
    auto hr = RecordStore::GetInstance().AddRegionRecord(input);
    if (hr != 0) {
        DBG_LOGERROR("Add region record failed, res is " << hr);
        return false;
    }
    return true;
}

bool RegionRepository::RemoveRegionInfo(const std::string name)
{
    auto hr = RecordStore::GetInstance().DelRegionRecord(name);
    if (hr != 0) {
        DBG_LOGERROR("Del region record failed, ret=" << hr);
        return false;
    }

    return true;
}

bool RegionRepository::Recovery(std::vector<RegionInfo> &regions)
{
    std::vector<CreateRegionInput> list = RecordStore::GetInstance().ListRegionRecord();

    std::string baseNid;
    SHMRegions regionList;
    auto hr = mxm::UbseMemAdapter::LookupRegionList(regionList);
    if (hr != 0) {
        DBG_LOGERROR("LookupShareRegions failed, ret: " << hr);
        return false;
    }

    for (auto elem : list) {
        RegionInfo region;
        hr = TransRecordToRegionCache(elem, region, regionList);
        if (hr != 0) {
            DBG_LOGERROR("TransRecordToRegion failed, ret: " << hr);
            return false;
        }
        regions.push_back(region);
    }

    return true;
}

} // namespace ock::share::service
