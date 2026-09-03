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
#ifndef OCK_SERVICE_ADAPTER_H
#define OCK_SERVICE_ADAPTER_H

#include <string>
#include <vector>
#include "log.h"
#include "service/service.h"
#include "util/common_headers.h"

namespace ock {
namespace daemon {
class OckServiceArgs {
public:
    OckServiceArgs() = default;
    ~OckServiceArgs()
    {
        ock::common::DeleteStrArray(mArgc, mArgv);
    }

    HRESULT PutArgs(const std::vector<std::string> &args)
    {
        return ock::common::CreateArgs(mArgc, mArgv, args);
    }

    int Argc() const
    {
        return mArgc;
    }

    char **Argv() const
    {
        return mArgv;
    }

private:
    int mArgc = 0;
    char **mArgv = nullptr;
};
class OckServiceAdapter {
public:
    OckServiceAdapter(const std::string &name, const std::string &libPath) : mName(name), mLibPath(libPath) {}
    ~OckServiceAdapter() = default;

    std::string Name() const
    {
        return mName;
    }

    std::string LibPath() const
    {
        return mLibPath;
    }

    static OckServiceArgs *CreateServiceArgs(const std::vector<std::string> &argNames);

protected:
    std::string mName;
    std::string mLibPath;
};
} // namespace daemon
} // namespace ock
#endif // OCK_SERVICE_ADAPTER_H