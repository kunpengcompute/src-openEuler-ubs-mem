/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * MemFabric_Hybrid is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/

#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif

char *DecryptPassword(const char *encrypted_data, size_t encrypted_len, size_t *p_out_len)
{
    if (encrypted_data == nullptr || p_out_len == nullptr) {
        return nullptr;
    }
    char *plaintext = new (std::nothrow) char[encrypted_len];
    if (!plaintext) {
        return nullptr;
    }

    // 简单反转字节顺序
    std::reverse_copy(encrypted_data, encrypted_data + encrypted_len, plaintext);
    // 设置输出长度
    *p_out_len = encrypted_len;
    return plaintext;
}

#ifdef __cplusplus
}
#endif