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
#include "ock_service_adapter.h"
namespace ock {
namespace daemon {

OckServiceArgs *OckServiceAdapter::CreateServiceArgs(const std::vector<std::string> &argNames)
{
    std::vector<std::string> args;
    auto argKeys = argNames;
    if (argKeys.empty()) {
        using namespace ock::common::ConfConstant;
        argKeys = {"", MXMD_DAEMON_BINPATH.first, MXMD_SERVER_LOG_LEVEL.first};
    }
    for (auto &name : argKeys) {
        auto config = Configuration::GetInstance();
        if (config == nullptr) {
            return nullptr;
        }
        std::string singleArg = config->GetConvertedValue(name);
        args.push_back(singleArg);
    }
    OckServiceArgs *serviceArg = new (std::nothrow) OckServiceArgs();
    if (serviceArg == nullptr) {
        return nullptr;
    }
    HRESULT hr = serviceArg->PutArgs(args);
    if (hr != 0) {
        SAFE_DELETE(serviceArg);
        return nullptr;
    }
    return serviceArg;
}
} // namespace daemon
} // namespace ock