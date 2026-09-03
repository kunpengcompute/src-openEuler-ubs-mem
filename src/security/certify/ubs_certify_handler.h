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
#ifndef UBS_CERTIFY_HANDLER_H
#define UBS_CERTIFY_HANDLER_H

#include <unistd.h>
#include <cstdint>

namespace ock::ubsm {

class UbsCertifyHandler {
public:
    static UbsCertifyHandler &GetInstance()
    {
        static UbsCertifyHandler instance;
        return instance;
    }

    int StartScheduledCertVerify();
    void StopScheduledCertVerify();

private:
    UbsCertifyHandler() = default;
    ~UbsCertifyHandler() = default;
};

} // namespace ock::ubsm

#endif