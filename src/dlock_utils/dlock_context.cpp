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
#include "dlock_context.h"
#include "dlock_executor.h"

using namespace ock::dlock_utils;

int32_t DLockContext::InitDlockClient()
{
    auto serverIp = cfg.serverIp;
    auto clientNum = cfg.dlockClientNum;
    if (clientNum >= MAX_CLIENT_NUM) {
        DBG_LOGERROR("DlockClient clientNum invalid, clientNum=" << clientNum << ", max allowed=" << MAX_CLIENT_NUM);
        return MXM_ERR_PARAM_INVALID;
    }

    DBG_LOGINFO("Init clients, serverIp=" << serverIp << ", serverId=" << cfg.serverId << ", clientNum=" << clientNum);
    for (uint32_t i = 0; i < clientNum; i++) {
        int32_t clientId = 0;
        auto ret = DLockExecutor::ClientInitWrapper(&clientId, serverIp.c_str());
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("Failed to init dlock client, index:" << i << " serverIp: " << serverIp << " ret: " << ret);
            if (ret == dlock::DLOCK_SERVER_NO_RESOURCE) {
                DBG_LOGERROR("Client_init dlock no resource, ret " << ret);
            }
            return ret;
        }
        index2ClientId.push_back(clientId);
        auto clientDesc = new (std::nothrow) ClientDesc(clientId);
        if (clientDesc == nullptr) {
            ret = DLockExecutor::GetInstance().DLockClientDeInitFunc(clientId);
            if (ret != dlock::DLOCK_SUCCESS) {
                DBG_LOGERROR("Failed to DLockClientDeInitFunc clientId: " << i << " ret: " << ret);
            }
            DBG_LOGERROR("DlockClient memory application failed");
            return MXM_ERR_MALLOC_FAIL;
        }
        dlockClients.push_back(clientDesc);
        DBG_LOGDEBUG("Init dlock client success, index:" << i << " serverIp:" << serverIp << " ret: " << ret);
    }
    DBG_LOGINFO("Successfully initialized all " << clientNum << " DLock clients, Initializing dlock clients completed");
    return UBSM_OK;
}

void DLockContext::UnInitDlockClient()
{
    if (DLockExecutor::GetInstance().DLockClientDeInitFunc == nullptr) {
        DBG_LOGERROR("DLockClientDeInitFunc is nullptr, init first.");
        return;
    }
    auto clientNum = index2ClientId.size();
    DBG_LOGINFO("Uninit DLock clients, clientNum=" << clientNum);
    for (uint32_t i = 0; i < clientNum; i++) {
        auto ret = DLockExecutor::GetInstance().DLockClientDeInitFunc(index2ClientId[i]);
        if (ret != dlock::DLOCK_SUCCESS) {
            DBG_LOGERROR("Failed to DeInit dlock client, index:" << i << " ret: " << ret);
            continue;
        }
        DBG_LOGINFO("Successfully to deinit dlock client, index: " << i);
    }
    DBG_LOGINFO("DLock clients uninitialization completed");
}

DLockContext::~DLockContext()
{
    for (auto &client : dlockClients) {
        if (client != nullptr) {
            delete client;
            client = nullptr;
        }
    }
}