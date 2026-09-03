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

#include <fcntl.h>

#include "RackMemShm.h"
#include "ShmMetaDataMgr.h"
#include "log.h"
#include "mx_shm.h"
#include "rack_mem_lib.h"
#include "rack_mem_macros.h"

using namespace ock::mxmd;
using namespace ock::common;

int RpcQueryNodeInfo()
{
    if (RackMemLib::GetInstance().StartRackMem() != 0) {
        DBG_LOGERROR("init mem lib error!");
        return MXM_ERR_MEMLIB;
    }
    std::string name;
    auto ret = RackMemShm::GetInstance().RpcQueryInfo(name);
    if (ret != 0) {
        DBG_LOGERROR("RpcQueryInfo failed, ret is " << ret);
        return ret;
    }
    DBG_LOGINFO("RpcQueryInfo success, remote node name is " << name);
    return 0;
}