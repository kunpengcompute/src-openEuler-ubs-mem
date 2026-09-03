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

#include "openssl_dl.h"
#include <dlfcn.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include "log.h"

static void *g_cryptoHandle = nullptr;
static void *g_sslHandle = nullptr;

using X509_STORE_CTX = struct x509_store_ctx_st;
using X509_CRL = struct x509_crl;
using X509 = struct x509_st;
using X509_STORE = struct x509_store;
using PEM_PASSWORD_CB = struct pem_password_cb;
using ASN1_TIME = struct asn1_string_st;

using FuncX509StoreNew = X509_STORE *(*)(void);
using FuncX509StoreCtxNew = X509_STORE_CTX *(*)(void);
using FuncX509VerifyCert = int (*)(X509_STORE_CTX *ctx);
using FuncX509VerifyCertErrorString = const char *(*)(long n);
using FuncX509StoreCtxInit = int (*)(X509_STORE_CTX *ctx, X509_STORE *store, X509 *x, void *untrusted);
using FuncX509StoreCtxGetError = int (*)(const X509_STORE_CTX *ctx);
using FuncPemReadX509 = X509 *(*)(FILE *, X509_CRL **x, PEM_PASSWORD_CB *cb, void *u);
using FuncPemReadX509Crl = X509_CRL *(*)(FILE *, X509_CRL **x, PEM_PASSWORD_CB *cb, void *u);
using FuncX509StoreSetFlags = void (*)(X509_STORE *xs, unsigned long flags);
using FuncX509StoreAddCert = int (*)(X509_STORE *xs, X509 *x);
using FuncX509StoreAddCrl = int (*)(X509_STORE *xs, X509_CRL *x);
using FuncX509Free = void (*)(X509 *x);
using FuncX509CrlFree = void (*)(X509_CRL *x);
using FuncX509StoreFree = void (*)(X509_STORE *xs);
using FuncX509StoreCtxFree = void (*)(X509_STORE_CTX *ctx);
using FuncX509Get0NotAfter = const ASN1_TIME *(*)(const X509 *x);
using FuncX509CmpTime = int (*)(const ASN1_TIME *asn1Time, time_t *inTm);
using FuncAsn1TimeToTm = int (*)(const ASN1_TIME *s, struct tm *tm);

static FuncX509StoreNew g_x509StoreNew = nullptr;
static FuncX509StoreCtxNew g_x509StoreCtxNew = nullptr;
static FuncPemReadX509 g_pemReadX509 = nullptr;
static FuncPemReadX509Crl g_pemReadX509Crl = nullptr;
static FuncX509StoreAddCert g_x509StoreAddCert = nullptr;
static FuncX509StoreAddCrl g_x509StoreAddCrl = nullptr;
static FuncX509StoreSetFlags g_x509StoreSetFlags = nullptr;
static FuncX509StoreCtxInit g_x509StoreCtxInit = nullptr;
static FuncX509VerifyCert g_x509VerifyCert = nullptr;
static FuncX509StoreCtxGetError g_x509StoreCtxGetError = nullptr;
static FuncX509VerifyCertErrorString g_x509VerifyCertErrorString = nullptr;
static FuncX509Free g_x509Free = nullptr;
static FuncX509CrlFree g_x509CrlFree = nullptr;
static FuncX509StoreFree g_x509StoreFree = nullptr;
static FuncX509StoreCtxFree g_x509StoreCtxFree = nullptr;
static FuncX509Get0NotAfter g_x509Get0NotAfter = nullptr;
static FuncX509CmpTime g_x509CmpTime = nullptr;
static FuncAsn1TimeToTm g_asn1TimeToTm = nullptr;

const unsigned long X509_V_FLAG_CRL_CHECK = 4UL;
const int64_t MINUTES_PER_HOUR = 60LL;
const int64_t HOURS_PER_DAY = 24LL;
const int64_t MINUTES_PER_DAY = HOURS_PER_DAY * MINUTES_PER_HOUR;

std::recursive_mutex g_opensslMutex;

void CleanupOpensslDl()
{
    std::lock_guard<std::recursive_mutex> guard(g_opensslMutex);
    if (g_cryptoHandle != nullptr) {
        dlclose(g_cryptoHandle);
        g_cryptoHandle = nullptr;
    }
    if (g_sslHandle != nullptr) {
        dlclose(g_sslHandle);
        g_sslHandle = nullptr;
    }

    g_x509StoreNew = nullptr;
    g_x509StoreCtxNew = nullptr;
    g_pemReadX509 = nullptr;
    g_pemReadX509Crl = nullptr;
    g_x509StoreAddCert = nullptr;
    g_x509StoreAddCrl = nullptr;
    g_x509StoreSetFlags = nullptr;
    g_x509StoreCtxInit = nullptr;
    g_x509VerifyCert = nullptr;
    g_x509StoreCtxGetError = nullptr;
    g_x509VerifyCertErrorString = nullptr;
    g_x509Free = nullptr;
    g_x509CrlFree = nullptr;
    g_x509StoreFree = nullptr;
    g_x509StoreCtxFree = nullptr;
    g_x509Get0NotAfter = nullptr;
    g_x509CmpTime = nullptr;
    g_asn1TimeToTm = nullptr;
}

int LoadOpensslFunc()
{
    g_x509StoreNew = (FuncX509StoreNew)dlsym(g_cryptoHandle, "X509_STORE_new");
    if (g_x509StoreNew == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_new: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreCtxNew = (FuncX509StoreCtxNew)dlsym(g_cryptoHandle, "X509_STORE_CTX_new");
    if (g_x509StoreCtxNew == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_CTX_new: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_pemReadX509 = (FuncPemReadX509)dlsym(g_cryptoHandle, "PEM_read_X509");
    if (g_pemReadX509 == nullptr) {
        DBG_LOGERROR("Failed to load PEM_read_X509: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_pemReadX509Crl = (FuncPemReadX509Crl)dlsym(g_cryptoHandle, "PEM_read_X509_CRL");
    if (g_pemReadX509Crl == nullptr) {
        DBG_LOGERROR("Failed to load PEM_read_X509_CRL: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreAddCert = (FuncX509StoreAddCert)dlsym(g_cryptoHandle, "X509_STORE_add_cert");
    if (g_x509StoreAddCert == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_add_cert: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreAddCrl = (FuncX509StoreAddCrl)dlsym(g_cryptoHandle, "X509_STORE_add_crl");
    if (g_x509StoreAddCrl == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_add_crl: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreSetFlags = (FuncX509StoreSetFlags)dlsym(g_cryptoHandle, "X509_STORE_set_flags");
    if (g_x509StoreSetFlags == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_set_flags: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreCtxInit = (FuncX509StoreCtxInit)dlsym(g_cryptoHandle, "X509_STORE_CTX_init");
    if (g_x509StoreCtxInit == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_CTX_init: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509VerifyCert = (FuncX509VerifyCert)dlsym(g_cryptoHandle, "X509_verify_cert");
    if (g_x509VerifyCert == nullptr) {
        DBG_LOGERROR("Failed to load X509_verify_cert: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreCtxGetError = (FuncX509StoreCtxGetError)dlsym(g_cryptoHandle, "X509_STORE_CTX_get_error");
    if (g_x509StoreCtxGetError == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_CTX_get_error: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509VerifyCertErrorString = (FuncX509VerifyCertErrorString)dlsym(g_cryptoHandle, "X509_verify_cert_error_string");
    if (g_x509VerifyCertErrorString == nullptr) {
        DBG_LOGERROR("Failed to load X509_verify_cert_error_string: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509Free = (FuncX509Free)dlsym(g_cryptoHandle, "X509_free");
    if (g_x509Free == nullptr) {
        DBG_LOGERROR("Failed to load X509_free: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509CrlFree = (FuncX509CrlFree)dlsym(g_cryptoHandle, "X509_CRL_free");
    if (g_x509CrlFree == nullptr) {
        DBG_LOGERROR("Failed to load X509_CRL_free: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreFree = (FuncX509StoreFree)dlsym(g_cryptoHandle, "X509_STORE_free");
    if (g_x509StoreFree == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_free: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509StoreCtxFree = (FuncX509StoreCtxFree)dlsym(g_cryptoHandle, "X509_STORE_CTX_free");
    if (g_x509StoreCtxFree == nullptr) {
        DBG_LOGERROR("Failed to load X509_STORE_CTX_free: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509Get0NotAfter = (FuncX509Get0NotAfter)dlsym(g_cryptoHandle, "X509_get0_notAfter");
    if (g_x509Get0NotAfter == nullptr) {
        DBG_LOGERROR("Failed to load X509_getm_notAfter: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_x509CmpTime = (FuncX509CmpTime)dlsym(g_cryptoHandle, "X509_cmp_time");
    if (g_x509CmpTime == nullptr) {
        DBG_LOGERROR("Failed to load X509_cmp_time: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }
    g_asn1TimeToTm = (FuncAsn1TimeToTm)dlsym(g_cryptoHandle, "ASN1_TIME_to_tm");
    if (g_asn1TimeToTm == nullptr) {
        DBG_LOGERROR("Failed to load ASN1_TIME_to_tm: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }

    return 0;
}

int InitOpensslDl()
{
    std::lock_guard<std::recursive_mutex> guard(g_opensslMutex);
    if (g_cryptoHandle != nullptr || g_sslHandle != nullptr) {
        return 0;
    }
#ifdef DEBUG_MEM_UT
    g_cryptoHandle = dlopen(ARCH_LIB_DIR "/libcrypto.so", RTLD_NOW);
    g_sslHandle = dlopen(ARCH_LIB_DIR "/libssl.so", RTLD_NOW);
#else
    g_cryptoHandle = dlopen(ARCH_LIB_DIR "/libcrypto.so.3", RTLD_NOW);
    g_sslHandle = dlopen(ARCH_LIB_DIR "/libssl.so.3", RTLD_NOW);
#endif
    if (g_cryptoHandle == nullptr || g_sslHandle == nullptr) {
        DBG_LOGERROR("Failed to load OpenSSL: " << dlerror());
        CleanupOpensslDl();
        return -1;
    }

    return LoadOpensslFunc();
}

void VerifyFreeRes(X509_STORE *store, X509 *caCert, X509 *cert, X509_CRL *crl, X509_STORE_CTX *ctx)
{
    if (cert != nullptr) {
        g_x509Free(cert);
    }
    if (caCert != nullptr) {
        g_x509Free(caCert);
    }
    if (crl != nullptr) {
        g_x509CrlFree(crl);
    }
    if (store != nullptr) {
        g_x509StoreFree(store);
    }
    if (ctx != nullptr) {
        g_x509StoreCtxFree(ctx);
    }
}

bool CheckExpired(X509 *cert, int64_t expireThreshold, const std::string &fileName)
{
    struct tm asnTm = {0};
    auto asnExpireTime = g_x509Get0NotAfter(cert);
    if (asnExpireTime == nullptr) {
        DBG_LOGERROR("Failed to get notAfter time");
        return false;
    }
    auto ret = g_x509CmpTime(asnExpireTime, nullptr);
    if (ret == -1) {
        DBG_LOGERROR("[" << fileName.c_str() << "] is expired");
        return false;
    }
    ret = g_asn1TimeToTm(asnExpireTime, &asnTm);
    if (ret != 1) {
        DBG_LOGERROR("Failed to convert asn1 time to tm, ret: " << ret);
        return false;
    }
    std::chrono::time_point<std::chrono::system_clock> expiredTime =
        std::chrono::system_clock::from_time_t(mktime(&asnTm));
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::minutes>(expiredTime - now);
    auto totalMinutes = duration.count();
    auto days = totalMinutes / MINUTES_PER_DAY;
    auto hours = (totalMinutes % MINUTES_PER_DAY) / MINUTES_PER_HOUR;
    auto minutes = totalMinutes % MINUTES_PER_HOUR;
    if (totalMinutes <= expireThreshold) {
        DBG_LOGWARN("[" << fileName.c_str() << "] will expire in " << days << " days " << hours << " hours " << minutes
                        << " minutes");
    }
    return true;
}

int VerifyCertificate(const char *caPath, const char *certPath, const char *crlPath, const char *caFileName,
                      const char *certFileName, int64_t expireThreshold)
{
    auto ret = InitOpensslDl();
    if (ret != 0) {
        DBG_LOGERROR("InitOpensslDl failed, ret: " << ret);
        return -1;
    }

    X509_STORE *store = nullptr;
    X509 *caCert = nullptr;
    X509 *cert = nullptr;
    X509_CRL *crl = nullptr;
    X509_STORE_CTX *ctx = nullptr;
    do {
        store = g_x509StoreNew();
        if (store == nullptr) {
            DBG_LOGERROR("Failed to open store");
            ret = -1;
            break;
        }

        FILE *fp = fopen(caPath, "r");
        if (fp == nullptr) {
            DBG_LOGERROR("Failed to open CA file: " << caPath);
            ret = -1;
            break;
        }
        caCert = g_pemReadX509(fp, nullptr, nullptr, nullptr);
        fclose(fp);
        if (caCert == nullptr) {
            DBG_LOGERROR("Failed to pem read ca");
            ret = -1;
            break;
        }
        if (!CheckExpired(caCert, expireThreshold, caFileName)) {
            DBG_LOGERROR("CA cert expire check failed");
            ret = -1;
            break;
        }
        ret = g_x509StoreAddCert(store, caCert);
        if (ret != 1) {
            DBG_LOGERROR("Failed to store and add cert, ret: " << ret);
            break;
        }

        if (crlPath != nullptr) {
            fp = fopen(crlPath, "r");
            if (fp == nullptr) {
                DBG_LOGERROR("Failed to open crl file: " << crlPath);
                ret = -1;
                break;
            }
            crl = g_pemReadX509Crl(fp, nullptr, nullptr, nullptr);
            fclose(fp);
            if (crl == nullptr) {
                DBG_LOGERROR("Failed to read crl");
                ret = -1;
                break;
            }

            ret = g_x509StoreAddCrl(store, crl);
            if (ret != 1) {
                DBG_LOGERROR("Failed to store and add crl, ret: " << ret);
                break;
            }
            g_x509StoreSetFlags(store, X509_V_FLAG_CRL_CHECK);
        }

        fp = fopen(certPath, "r");
        if (fp == nullptr) {
            DBG_LOGERROR("Failed to open cert file: " << certPath);
            ret = -1;
            break;
        }
        cert = g_pemReadX509(fp, nullptr, nullptr, nullptr);
        fclose(fp);
        if (cert == nullptr) {
            DBG_LOGERROR("Failed to pem read cert");
            ret = -1;
            break;
        }
        if (!CheckExpired(cert, expireThreshold, certFileName)) {
            DBG_LOGERROR("Cert expire check failed");
            ret = -1;
            break;
        }

        ctx = g_x509StoreCtxNew();
        if (ctx == nullptr) {
            DBG_LOGERROR("Failed to new ctx");
            ret = -1;
            break;
        }
        ret = g_x509StoreCtxInit(ctx, store, cert, nullptr);
        if (ret != 1) {
            DBG_LOGERROR("Failed to init store, ret: " << ret);
            break;
        }
    } while (0);

    if (ret != 1) {
        VerifyFreeRes(store, caCert, cert, crl, ctx);
        return ret;
    }

    int result = g_x509VerifyCert(ctx);
    int err = g_x509StoreCtxGetError(ctx);

    VerifyFreeRes(store, caCert, cert, crl, ctx);
    if (result != 1) {
        DBG_LOGERROR("Certificate is invalid: " << g_x509VerifyCertErrorString(err));
        return -1;
    }
    DBG_LOGINFO("Certificate is valid");
    return 0;
}