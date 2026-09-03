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
#ifndef OPENSSL_DL_H
#define OPENSSL_DL_H

#include <unistd.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

int InitOpensslDl();

void CleanupOpensslDl();

int VerifyCertificate(const char *caPath, const char *certPath, const char *crlPath, const char *caFileName,
                      const char *certFileName, int64_t expireThreshold);

#ifdef __cplusplus
}
#endif

#endif