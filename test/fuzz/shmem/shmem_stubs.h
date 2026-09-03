/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef UBSMEM_STUB_H
#define UBSMEM_STUB_H

#include "ipc_proxy.h"
#include "mx_shm.h"
#include "mxm_msg.h"

#include "mxm_lease/ipc_handler.h"
#include "mxm_lease/ipc_server.h"

#include "mxm_message/message_op.h"
#include "mxm_shm/ipc_handler.h"
#include "mxm_shm/ipc_server.h"
#include "region_manager.h"
#include "ubse_mem_adapter.h"

#define filter_all_hosts_in_attr(list, attr, index) UBSM_OK

using namespace ock::mxmd;
using namespace ock::mxm;
using namespace ock::share::service;
namespace FUZZ {

int32_t LookupRegionListFuzzStub(SHMRegions &regions);

int FilterAllHostsInAttrStub(SHMRegions *list, const ubsmem_region_attributes_t *reg_attr, int &index);

void MxmComStopIpcClientStub();

int MxmComIpcClientSendStub(uint16_t opCode, MsgBase *request, MsgBase *response);

int ClientIpcLeaseMallocMemStub(MsgBase *request, MsgBase *response);

int ClientIpcLeaseFreeMemStub(MsgBase *request, MsgBase *response);

int ClientIpcShmLookRegionListStub(MsgBase *request, MsgBase *response);

int ClientIpcShmLookRegionInfoStub(MsgBase *request, MsgBase *response);

int ClientIpcShmCreateStub(MsgBase *request, MsgBase *response);

int ClientIpcShmDeleteStub(MsgBase *request, MsgBase *response);

int ClientIpcShmMmapStub(MsgBase *request, MsgBase *response);

int ClientIpcShmUnmmapStub(MsgBase *request, MsgBase *response);

int ClientIpcGetShmMemFaultStatusStub(MsgBase *request, MsgBase *response);

int RegionLookupRegionStub(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);

int ShmImportStub(const ImportShmParam &param, ImportShmResult &result);

int ShmImportStub2(const ImportShmParam &param, ImportShmResult &result);

int LeaseMallocStub(const LeaseMallocParam &param, ock::mxm::LeaseMallocResult &result);

int ShmCreateStub(const MsgBase *req, MsgBase *rsp, const MxmComUdsInfo &udsInfo);
} // namespace FUZZ

#endif // UBSMEM_STUB_H
