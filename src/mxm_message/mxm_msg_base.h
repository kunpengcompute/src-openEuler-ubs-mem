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
#ifndef MXM_MSG_BASE_H
#define MXM_MSG_BASE_H

#include <unistd.h>
#include "mxm_msg_packer.h"

namespace ock {
namespace mxmd {
struct MsgBase {
    int16_t msgVer = 0;
    int16_t opCode = -1;
    uint32_t destRankId = 0;

    MsgBase() = default;
    MsgBase(int16_t ver, int16_t op, uint32_t dst) : msgVer(ver), opCode(op), destRankId(dst) {}
    virtual int32_t Serialize(NetMsgPacker &packer) const = 0;
    virtual int32_t Deserialize(NetMsgUnpacker &packer) = 0;

    virtual ~MsgBase() = default;
};

enum MXM_MSG_OPCODE : int16_t {
    MXM_MSG_SHM_ALLOCATE = 66,
    MXM_MSG_SHM_DEALLOCATE,

    MXM_MSG_BUTT,
};

} // namespace mxmd
} // namespace ock

#endif // MXMD_MSG_BASE_H
