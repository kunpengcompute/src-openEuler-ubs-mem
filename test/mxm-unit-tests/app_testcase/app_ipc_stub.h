/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#ifndef MXM_APP_IPC_STUB_H
#define MXM_APP_IPC_STUB_H

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ipc_proxy.h"
#include "mx_shm.h"
#include "mxm_msg.h"

#include "mxm_lease/ipc_handler.h"
#include "mxm_lease/ipc_server.h"

#include "mxm_message/message_op.h"
#include "mxm_shm/ipc_handler.h"
#include "mxm_shm/ipc_server.h"

#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif

using namespace ock::mxmd;
namespace UT {

void ClientIpcStubSetUdsInfo(uint32_t pid, uint32_t uid, uint32_t gid);

void MxmComStopIpcClientStub();

int MxmComIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response);

int MxmShmIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response);

int ClientIpcLeaseMallocMemStub(MsgBase *request, MsgBase *response);

int ClientIpcLeaseFreeMemStub(MsgBase *request, MsgBase *response);

int ClientIpcShmLookRegionListStub(MsgBase *request, MsgBase *response);

int ClientIpcShmLookRegionInfoStub(MsgBase *request, MsgBase *response);

int ClientIpcShmCreateStub(MsgBase *request, MsgBase *response);

int ClientIpcShmDeleteStub(MsgBase *request, MsgBase *response);

int ClientIpcShmMmapStub(MsgBase *request, MsgBase *response);

int ClientIpcShmUnmmapStub(MsgBase *request, MsgBase *response);

int ClientIpcGetShmMemFaultStatusStub(MsgBase *request, MsgBase *response);

int ClientIpcLookupClusterStatisticStub(MsgBase *request, MsgBase *response);

int ClientIpcQueryLeaseRecordStub(MsgBase *request, MsgBase *response);

int ClientIpcForceFreeCachedMemoryStub(MsgBase *request, MsgBase *response);

} // namespace UT

#endif