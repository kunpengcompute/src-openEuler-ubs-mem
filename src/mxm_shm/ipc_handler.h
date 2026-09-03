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

#ifndef MXM_SERVER_IPC_HANDLER_H
#define MXM_SERVER_IPC_HANDLER_H

#include <unistd.h>
#include <array>
#include <mutex>
#include <string>
#include "log.h"
#include "mxm_ipc_server_interface.h"
#include "mxm_msg.h"
#include "ubse_mem_adapter.h"

namespace ock::share::service {
using namespace ock::mxmd;
using namespace ock::com;
using namespace ock::common;

static constexpr uint32_t MUTEX_HASH_SIZE = 32;

class MxmServerMsgHandle {
public:
    static int ShmLookRegionList(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmCreate(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmCreateWithProvider(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmDelete(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmMap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmUnmap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int RegionLookupRegionList(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int RegionCreateRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int RegionLookupRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int RegionDestroyRegion(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int ShmWriteLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmReadLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmUnLock(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmQueryNode(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmQueryDlockStatus(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int ShmAttach(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmDetach(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmListLookup(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int ShmLookup(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int RpcQueryNodeInfo(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

    static int AppCheckShareMemoryMap(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
    static int LookupLocalSlotId(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

private:
    static std::mutex &GetMutexByName(const std::string &name) noexcept;
    static std::array<std::mutex, MUTEX_HASH_SIZE> mutexArray;
};
} // namespace ock::share::service

#endif