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
#ifndef MEMORYFABRIC_RMLIBOBMMEXECUTOR_H
#define MEMORYFABRIC_RMLIBOBMMEXECUTOR_H

#include <cstddef>
#include "rack_mem_err.h"
#include "rack_mem_libobmm.h"

namespace ock::mxmd {
using ObmmOwnershipSwitch = int (*)(int fd, void *start, void *end, int prot);
class RmLibObmmExecutor {
public:
    ObmmOwnershipSwitch obmmOwnershipSwitch{};
    uint32_t Initialize();
    uint32_t Exit();
    static RmLibObmmExecutor &GetInstance()
    {
        static RmLibObmmExecutor instance;
        return instance;
    }
    RmLibObmmExecutor(const RmLibObmmExecutor &other) = default;
    RmLibObmmExecutor(RmLibObmmExecutor &&other) = default;
    RmLibObmmExecutor &operator=(const RmLibObmmExecutor &other) = default;
    RmLibObmmExecutor &operator=(RmLibObmmExecutor &&other) noexcept = default;

    DAGGER_DEFINE_REF_COUNT_FUNCTIONS
private:
    DAGGER_DEFINE_REF_COUNT_VARIABLE;
    void *handle{nullptr};
    RmLibObmmExecutor() = default;
};
} // namespace ock::mxmd
#endif // MEMORYFABRIC_RMLIBOBMMEXECUTOR_H