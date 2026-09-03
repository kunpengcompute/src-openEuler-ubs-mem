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
#ifndef OCK_COMMON_CONFIGURATION_CONVERTER_H
#define OCK_COMMON_CONFIGURATION_CONVERTER_H

#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "functions.h"
#include "log_adapter.h"
#include "ref.h"

namespace ock {
namespace common {

class Converter : public Referable {
public:
    ~Converter() override = default;

    virtual std::string Convert(const std::string &str)
    {
        return str;
    }
};

using ConverterPtr = Ref<Converter>;

class ConLogLevel : public Converter {
public:
    static ConverterPtr Create()
    {
        return ConverterPtr(new (std::nothrow) ConLogLevel());
    }
    ConLogLevel() = default;
    ~ConLogLevel() override = default;
    std::string Convert(const std::string &str) override
    {
        return std::to_string(LogAdapter::StringToLogLevel(str));
    }
};
} // namespace common
} // namespace ock
#endif