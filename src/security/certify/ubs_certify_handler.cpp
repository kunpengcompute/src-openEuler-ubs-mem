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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "ubs_certify_handler.h"
#include <pthread.h>
#include <atomic>
#include "log.h"
#include "openssl_dl.h"
#include "ubs_common_config.h"

namespace ock::ubsm {

static pthread_t g_verifyTaskId;
static pthread_mutex_t g_verifyMutex;
static pthread_cond_t g_verifyCond;
static std::atomic_bool g_verifyThreadRunning(false);
const char *VERIFY_TASK_NAME = "ubsm_cert_verify_task";
const char *CA_FILE_NAME = "Ubsm CA Cert";
const char *CERT_FILE_NAME = "Ubsm Server Cert";
#ifndef DEBUG_MEM_UT
const int64_t CERT_VERIFY_PERIOD_SECONDS = 24 * 3600;
#else
const int64_t CERT_VERIFY_PERIOD_SECONDS = 15;
#endif
const int64_t CERT_EXPIRE_THRESHOLD = 43200;

static int32_t DoVerify(void)
{
    int32_t ret = 0;
    if (UbsCommonConfig::GetInstance().GetCrlPath().empty()) {
        ret = VerifyCertificate(UbsCommonConfig::GetInstance().GetCaPath().c_str(),
                                UbsCommonConfig::GetInstance().GetCertPath().c_str(), nullptr, CA_FILE_NAME,
                                CERT_FILE_NAME, CERT_EXPIRE_THRESHOLD);
    } else {
        ret = VerifyCertificate(
            UbsCommonConfig::GetInstance().GetCaPath().c_str(), UbsCommonConfig::GetInstance().GetCertPath().c_str(),
            UbsCommonConfig::GetInstance().GetCrlPath().c_str(), CA_FILE_NAME, CERT_FILE_NAME, CERT_EXPIRE_THRESHOLD);
    }
    if (ret != 0) {
        DBG_LOGERROR("Schedule VerifyCertificate failed");
    }
    return ret;
}

static void *CertVerifyScheduleTask(void *arg)
{
    g_verifyThreadRunning.store(true);
    while (g_verifyThreadRunning.load()) {
        int32_t ret = DoVerify();
        if (ret != 0) {
            DBG_LOGERROR("Schedule VerifyCertificate failed");
        }
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += CERT_VERIFY_PERIOD_SECONDS;

        pthread_mutex_lock(&g_verifyMutex);
        pthread_cond_timedwait(&g_verifyCond, &g_verifyMutex, &ts);
        pthread_mutex_unlock(&g_verifyMutex);
    }
    return nullptr;
}

int UbsCertifyHandler::StartScheduledCertVerify()
{
    if (g_verifyThreadRunning.load()) {
        DBG_LOGINFO("Cert verify schedule task already running");
        return 0;
    }
    if (!UbsCommonConfig::GetInstance().GetTlsSwitch()) {
        DBG_LOGINFO("Tls disable, skip cert verify schedule task");
        return 0;
    }
    int32_t ret = DoVerify();
    if (ret != 0) {
        DBG_LOGERROR("First verifyCertificate failed, stop init schedule task");
        return ret;
    }
    pthread_mutex_init(&g_verifyMutex, nullptr);
    pthread_cond_init(&g_verifyCond, nullptr);
    ret = pthread_create(&g_verifyTaskId, nullptr, CertVerifyScheduleTask, nullptr);
    if (ret != 0) {
        DBG_LOGERROR("Failed to init schedule task, pthread_create failed, ret: " << ret);
        pthread_cond_destroy(&g_verifyCond);
        pthread_mutex_destroy(&g_verifyMutex);
        return ret;
    }
    pthread_setname_np(g_verifyTaskId, VERIFY_TASK_NAME);
    DBG_LOGINFO("Init cert verify schedule task success");
    return 0;
}

void UbsCertifyHandler::StopScheduledCertVerify()
{
    if (!g_verifyThreadRunning.load()) {
        return;
    }
    g_verifyThreadRunning.store(false);
    pthread_mutex_lock(&g_verifyMutex);
    pthread_cond_signal(&g_verifyCond);
    pthread_mutex_unlock(&g_verifyMutex);

    if (g_verifyTaskId != 0) {
        pthread_join(g_verifyTaskId, nullptr);
    }
    pthread_mutex_destroy(&g_verifyMutex);
    pthread_cond_destroy(&g_verifyCond);
    CleanupOpensslDl();
}

} // namespace ock::ubsm