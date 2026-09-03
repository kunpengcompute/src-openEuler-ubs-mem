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

#ifndef MXM_MSG_H
#define MXM_MSG_H

#include "ubs_mem_def.h"
#include "message_op.h"
#include "mx_shm.h"
#include "mxm_msg_base.h"
#include "mxm_msg_packer.h"
#include "rpc_config.h"
#include "ubs_common_types.h"

namespace ock {
namespace mxmd {
using namespace ock::ubsm;

struct CheckMemoryLeaseRequest : MsgBase {
    std::vector<std::string> names_;
    CheckMemoryLeaseRequest() : MsgBase(0, IPC_CHECK_MEMORY_LEASE, 0) {}
    explicit CheckMemoryLeaseRequest(const std::vector<std::string> &names)
        : MsgBase(0, IPC_CHECK_MEMORY_LEASE, 0),
          names_(names){};
    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(names_);
        return UBSM_OK;
    }
    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(names_);
        return UBSM_OK;
    }
};

struct CheckShareMemoryMapRequest : MsgBase {
    std::vector<std::string> names_;
    CheckShareMemoryMapRequest() : MsgBase(0, IPC_CHECK_SHARE_MEMORY, 0) {}
    explicit CheckShareMemoryMapRequest(const std::vector<std::string> &names)
        : MsgBase(0, IPC_CHECK_SHARE_MEMORY, 0),
          names_(names){};
    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(names_);
        return UBSM_OK;
    }
    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(names_);
        return UBSM_OK;
    }
};

struct CommonRequest : MsgBase {
    uint32_t input_;

    CommonRequest() : MsgBase{0, MXM_MSG_BUTT, 0} {};
    explicit CommonRequest(int32_t input) : MsgBase{0, MXM_MSG_BUTT, 0}, input_(input){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(input_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(input_);
        return UBSM_OK;
    }
};

struct CommonResponse : MsgBase {
    int32_t errCode_;

    CommonResponse() noexcept : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    explicit CommonResponse(int32_t errCode) : MsgBase{0, MXM_MSG_BUTT, 0}, errCode_(errCode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct ShmemAllocateRequest : MsgBase {
    std::string regionName_;
    std::string shmName_;
    size_t size_;
    mode_t mode_;
    uint64_t flag_;

    ShmemAllocateRequest() : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0} {};
    ShmemAllocateRequest(const std::string &regionName, const std::string &shmName, size_t size, mode_t mode,
                         uint64_t flag)
        : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0},
          regionName_(regionName),
          shmName_(shmName),
          size_(size),
          mode_(mode),
          flag_(flag){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(shmName_);
        packer.Serialize(size_);
        packer.Serialize(mode_);
        packer.Serialize(flag_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(shmName_);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(mode_);
        unpacker.Deserialize(flag_);
        return UBSM_OK;
    }
};

struct ShmemDeallocateRequest : MsgBase {
    std::string shmName_;

    ShmemDeallocateRequest() : MsgBase{0, MXM_MSG_SHM_DEALLOCATE, 0} {};
    explicit ShmemDeallocateRequest(const std::string &shmName)
        : MsgBase{0, MXM_MSG_SHM_DEALLOCATE, 0},
          shmName_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(shmName_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(shmName_);
        return UBSM_OK;
    }
};

/* old rackMem interface */

struct AppMallocMemoryRequest : MsgBase {
    uint64_t size_;
    uint16_t perflevel_;
    uint16_t isNuma_;
    std::string regionName_;

    AppMallocMemoryRequest() : MsgBase{0, IPC_MALLOC_MEMORY, 0} {};
    AppMallocMemoryRequest(uint64_t size, uint64_t perflevel, bool isNuma, const std::string &regionName)
        : MsgBase{0, IPC_MALLOC_MEMORY, 0},
          size_(size),
          perflevel_(perflevel),
          isNuma_(isNuma),
          regionName_(regionName)
    {
    }

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(size_);
        packer.Serialize(perflevel_);
        packer.Serialize(isNuma_);
        packer.Serialize(regionName_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(perflevel_);
        unpacker.Deserialize(isNuma_);
        unpacker.Deserialize(regionName_);
        return UBSM_OK;
    }
};

struct AppMallocMemoryWithLocRequest : MsgBase {
    uint64_t size_;
    uint16_t isNuma_;
    uint32_t slotId_;
    uint32_t socketId_;
    uint32_t numaId_;
    uint32_t portId_;

    AppMallocMemoryWithLocRequest() : MsgBase{0, IPC_MALLOC_MEMORY_LOC, 0} {};
    AppMallocMemoryWithLocRequest(uint64_t size, bool isNuma, uint32_t slotId, uint32_t socketId, uint32_t numaId,
                                  uint32_t portId)
        : MsgBase{0, IPC_MALLOC_MEMORY_LOC, 0},
          size_(size),
          isNuma_(isNuma),
          slotId_(slotId),
          socketId_(socketId),
          numaId_(numaId),
          portId_(portId)
    {
    }

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(size_);
        packer.Serialize(isNuma_);
        packer.Serialize(slotId_);
        packer.Serialize(socketId_);
        packer.Serialize(numaId_);
        packer.Serialize(portId_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(isNuma_);
        unpacker.Deserialize(slotId_);
        unpacker.Deserialize(socketId_);
        unpacker.Deserialize(numaId_);
        unpacker.Deserialize(portId_);
        return UBSM_OK;
    }
};

struct AppFreeMemoryRequest : MsgBase {
    std::vector<uint64_t> memIds_{};
    std::string regionName_;
    std::string name_;

    AppFreeMemoryRequest() : MsgBase{0, IPC_FREE_RACKMEM, 0} {};
    AppFreeMemoryRequest(std::vector<uint64_t> memids, const std::string &regionName, const std::string &name)
        : MsgBase{0, IPC_FREE_RACKMEM, 0},
          memIds_(memids),
          regionName_(regionName),
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(memIds_);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(memIds_);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct AppMallocMemoryResponse : MsgBase {
    int32_t errCode_;
    std::vector<uint64_t> memIds_{};
    int64_t numaId_;
    bool isNuma_;
    size_t unitSize_;
    std::string name_;

    AppMallocMemoryResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    AppMallocMemoryResponse(int32_t errCode, std::vector<uint64_t> &memIds, int64_t numaId, bool isNuma,
                            size_t unitSize, const std::string name)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          memIds_(memIds),
          numaId_(numaId),
          isNuma_(isNuma),
          unitSize_(unitSize),
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(memIds_);
        packer.Serialize(numaId_);
        packer.Serialize(isNuma_);
        packer.Serialize(unitSize_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(memIds_);
        unpacker.Deserialize(numaId_);
        unpacker.Deserialize(isNuma_);
        unpacker.Deserialize(unitSize_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct AppQueryClusterInfoResponse : MsgBase {
    int32_t errCode_;
    ubsmemClusterInfo info_{};

    AppQueryClusterInfoResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    AppQueryClusterInfoResponse(int32_t errCode, ubsmemClusterInfo &info)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          info_(info){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(info_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(info_);
        return UBSM_OK;
    }
};

struct AppQueryCachedMemoryResponse : MsgBase {
    int32_t errCode_;
    std::vector<std::string> records_{};

    AppQueryCachedMemoryResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    AppQueryCachedMemoryResponse(int32_t errCode, std::vector<std::string> &records)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          records_(records){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(records_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(records_);
        return UBSM_OK;
    }
};

/* old rackMemShm interface */

struct ShmLookRegionListRequest : MsgBase {
    std::string baseNid_;
    int32_t regionType_;

    ShmLookRegionListRequest() : MsgBase{0, IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, 0} {};
    ShmLookRegionListRequest(const std::string &baseNid, int32_t regionType)
        : MsgBase{0, IPC_RACKMEMSHM_LOOKUP_SHAREREGIONS, 0},
          baseNid_(baseNid),
          regionType_(regionType){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(baseNid_);
        packer.Serialize(regionType_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(baseNid_);
        unpacker.Deserialize(regionType_);
        return UBSM_OK;
    }
};

struct ShmCreateRegionRequest : MsgBase {
    std::string regionName_;
    SHMRegionDesc region_{};

    ShmCreateRegionRequest() : MsgBase{0, IPC_REGION_CREATE_REGION, 0} {};
    ShmCreateRegionRequest(const std::string &regionName, const SHMRegionDesc &region)
        : MsgBase{0, IPC_REGION_CREATE_REGION, 0},
          regionName_(regionName),
          region_(region){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(region_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(region_);
        return UBSM_OK;
    }
};

struct ShmLookupRegionRequest : MsgBase {
    std::string regionName_;

    ShmLookupRegionRequest() : MsgBase{0, IPC_REGION_LOOKUP_REGION, 0} {};
    explicit ShmLookupRegionRequest(const std::string &regionName)
        : MsgBase{0, IPC_REGION_LOOKUP_REGION, 0},
          regionName_(regionName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        return UBSM_OK;
    }
};

struct ShmDestroyRegionRequest : MsgBase {
    std::string regionName_;

    ShmDestroyRegionRequest() : MsgBase{0, IPC_REGION_DESTROY_REGION, 0} {};
    explicit ShmDestroyRegionRequest(const std::string &regionName)
        : MsgBase{0, IPC_REGION_DESTROY_REGION, 0},
          regionName_(regionName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        return UBSM_OK;
    }
};

struct ShmCreateRequest : MsgBase {
    std::string regionName_;
    std::string name_;
    uint64_t size_;
    std::string baseNid_;
    SHMRegionDesc regionDesc_;
    uint64_t flags_;
    mode_t mode_;

    ShmCreateRequest() : MsgBase{0, IPC_RACKMEMSHM_CREATE, 0} {};
    ShmCreateRequest(const std::string &regionName, const std::string &name, uint64_t size, const std::string &baseNid,
                     const SHMRegionDesc &regionDesc, uint64_t flags, mode_t mode)
        : MsgBase{0, IPC_RACKMEMSHM_CREATE, 0},
          regionName_(regionName),
          name_(name),
          size_(size),
          baseNid_(baseNid),
          regionDesc_(regionDesc),
          flags_(flags),
          mode_(mode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        packer.Serialize(size_);
        packer.Serialize(baseNid_);
        packer.Serialize(regionDesc_);
        packer.Serialize(flags_);
        packer.Serialize(mode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(baseNid_);
        unpacker.Deserialize(regionDesc_);
        unpacker.Deserialize(flags_);
        unpacker.Deserialize(mode_);
        return UBSM_OK;
    }
};

struct ShmCreateWithProviderRequest : MsgBase {
    std::string hostName_;
    uint32_t socketId_;
    uint32_t numaId_;
    uint32_t portId_;
    std::string name_;
    uint64_t size_;
    uint64_t flags_;
    mode_t mode_;

    ShmCreateWithProviderRequest() : MsgBase{0, IPC_RACKMEMSHM_CREATE_WITH_PROVIDER, 0} {};
    ShmCreateWithProviderRequest(const std::string &hostName, uint32_t socketId, uint32_t numaId, uint32_t portId,
                                 const std::string &name, uint64_t size, uint64_t flags, mode_t mode) noexcept
        : MsgBase{0, IPC_RACKMEMSHM_CREATE_WITH_PROVIDER, 0},
          hostName_(std::move(hostName)),
          socketId_(socketId),
          numaId_(numaId),
          portId_(portId),
          name_(std::move(name)),
          size_(size),
          flags_(flags),
          mode_(mode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(hostName_);
        packer.Serialize(socketId_);
        packer.Serialize(numaId_);
        packer.Serialize(portId_);
        packer.Serialize(name_);
        packer.Serialize(size_);
        packer.Serialize(flags_);
        packer.Serialize(mode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(hostName_);
        unpacker.Deserialize(socketId_);
        unpacker.Deserialize(numaId_);
        unpacker.Deserialize(portId_);
        unpacker.Deserialize(name_);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(flags_);
        unpacker.Deserialize(mode_);
        return UBSM_OK;
    }
};

struct ShmDeleteRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmDeleteRequest() : MsgBase{0, IPC_RACKMEMSHM_DELETE, 0} {};
    explicit ShmDeleteRequest(const std::string &regionName, const std::string &name)
        : MsgBase{0, IPC_RACKMEMSHM_DELETE, 0},
          regionName_(regionName),
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmMapRequest : MsgBase {
    std::string regionName_;
    std::string name_;
    uint64_t size_;
    mode_t mode_;
    int prot_;

    ShmMapRequest() : MsgBase{0, IPC_RACKMEMSHM_MMAP, 0} {};
    ShmMapRequest(const std::string &regionName, const std::string &shmName, uint64_t size, mode_t mode, int prot)
        : MsgBase{0, IPC_RACKMEMSHM_MMAP, 0},
          regionName_(regionName),
          name_(shmName),
          size_(size),
          mode_(mode),
          prot_(prot){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        packer.Serialize(size_);
        packer.Serialize(mode_);
        packer.Serialize(prot_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        unpacker.Deserialize(size_);
        unpacker.Deserialize(mode_);
        unpacker.Deserialize(prot_);
        return UBSM_OK;
    }
};

struct ShmUnmapRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmUnmapRequest() : MsgBase{0, IPC_RACKMEMSHM_UNMMAP, 0} {};
    explicit ShmUnmapRequest(const std::string &regionName, const std::string &name)
        : MsgBase{0, IPC_RACKMEMSHM_UNMMAP, 0},
          regionName_(regionName),
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmQueryMemFaultStatusRequest : MsgBase {
    std::string name_;

    ShmQueryMemFaultStatusRequest() : MsgBase{0, IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS, 0} {};
    explicit ShmQueryMemFaultStatusRequest(const std::string &name)
        : MsgBase{0, IPC_RACKMEMSHM_QUERY_MEM_FAULT_STATUS, 0},
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct PingRequest : MsgBase {
    std::string nodeId_;

    PingRequest() : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0} {};
    explicit PingRequest(const std::string &nodeId) : MsgBase{0, MXM_MSG_BUTT, 0}, nodeId_(nodeId){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(nodeId_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(nodeId_);
        return UBSM_OK;
    }
};

struct VoteRequest : MsgBase {
    std::string nodeId_;
    std::string masterNode_;
    int term_;

    VoteRequest() : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0} {};
    VoteRequest(const std::string &nodeId, const std::string &masterNode, int term)
        : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0},
          nodeId_(nodeId),
          masterNode_(masterNode),
          term_(term){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(nodeId_);
        packer.Serialize(masterNode_);
        packer.Serialize(term_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(nodeId_);
        unpacker.Deserialize(masterNode_);
        unpacker.Deserialize(term_);
        return UBSM_OK;
    }
};

struct BroadcastRequest : MsgBase {
    std::string nodeId_;
    bool isSeverInited_;
    std::map<std::string, ock::rpc::ClusterNode> nodes_;

    BroadcastRequest() : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0}, isSeverInited_(false){};
    BroadcastRequest(const std::string &nodeId, const std::map<std::string, ock::rpc::ClusterNode> &nodes,
                     bool isSeverInited)
        : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0},
          nodeId_(nodeId),
          isSeverInited_(isSeverInited),
          nodes_(nodes)
    {
    }

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(nodeId_);
        packer.Serialize(nodes_);
        packer.Serialize(isSeverInited_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(nodeId_);
        unpacker.Deserialize(nodes_);
        unpacker.Deserialize(isSeverInited_);
        return UBSM_OK;
    }
};

struct TransElectedRequest : MsgBase {
    std::string nodeId_;
    std::vector<std::string> nodes_;
    int term_;

    TransElectedRequest() : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0} {};
    TransElectedRequest(const std::string &nodeId, const std::vector<std::string> &nodes, int term)
        : MsgBase{0, MXM_MSG_SHM_ALLOCATE, 0},
          nodeId_(nodeId),
          nodes_(nodes),
          term_(term)
    {
    }

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(nodeId_);
        packer.Serialize(nodes_);
        packer.Serialize(term_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(nodeId_);
        unpacker.Deserialize(nodes_);
        unpacker.Deserialize(term_);
        return UBSM_OK;
    }
};

struct ShmLookRegionListResponse : MsgBase {
    int32_t errCode_;
    SHMRegions regions_{};

    ShmLookRegionListResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmLookRegionListResponse(int32_t errCode, SHMRegions &regions)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          regions_(regions){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(regions_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(regions_);
        return UBSM_OK;
    }
};

struct ShmCreateRegionResponse : MsgBase {
    int32_t errCode_;

    ShmCreateRegionResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    explicit ShmCreateRegionResponse(int32_t errCode) : MsgBase{0, MXM_MSG_BUTT, 0}, errCode_(errCode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct ShmLookupRegionResponse : MsgBase {
    int32_t errCode_;
    SHMRegionDesc region_{};

    ShmLookupRegionResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmLookupRegionResponse(int32_t errCode, const SHMRegionDesc &region)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          region_(region){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(region_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(region_);
        return UBSM_OK;
    }
};

struct ShmDestroyRegionResponse : MsgBase {
    int32_t errCode_;

    ShmDestroyRegionResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    explicit ShmDestroyRegionResponse(int32_t errCode) : MsgBase{0, MXM_MSG_BUTT, 0}, errCode_(errCode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct ShmMapResponse : MsgBase {
    int32_t errCode_;
    std::vector<uint64_t> memIds_{};
    uint64_t shmSize_;
    uint64_t unitSize_;
    uint64_t flag_;
    int oflag_;

    ShmMapResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmMapResponse(int32_t errCode, std::vector<uint64_t> &memIds, uint64_t shmSize, uint64_t unitSize, int flag,
                   int oflag)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          memIds_(memIds),
          shmSize_(shmSize),
          unitSize_(unitSize),
          flag_(flag),
          oflag_(oflag){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(memIds_);
        packer.Serialize(shmSize_);
        packer.Serialize(unitSize_);
        packer.Serialize(flag_);
        packer.Serialize(oflag_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(memIds_);
        unpacker.Deserialize(shmSize_);
        unpacker.Deserialize(unitSize_);
        unpacker.Deserialize(flag_);
        unpacker.Deserialize(oflag_);
        return UBSM_OK;
    }
};

struct ShmQueryMemFaultStatusResponse : MsgBase {
    int32_t errCode_;
    bool isMemFault_;

    ShmQueryMemFaultStatusResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmQueryMemFaultStatusResponse(int32_t errCode, bool isMemFault)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          isMemFault_(isMemFault){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(isMemFault_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(isMemFault_);
        return UBSM_OK;
    }
};

struct ShmWriteLockRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmWriteLockRequest() : MsgBase{0, IPC_RACKMEMSHM_WRITELOCK, 0} {};
    ShmWriteLockRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_WRITELOCK, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmWriteLockResponse : MsgBase {
    int32_t errCode_;

    ShmWriteLockResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct ShmReadLockRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmReadLockRequest() : MsgBase{0, IPC_RACKMEMSHM_READLOCK, 0} {};
    ShmReadLockRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_READLOCK, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmReadLockResponse : MsgBase {
    int32_t errCode_;

    ShmReadLockResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct ShmUnLockRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmUnLockRequest() : MsgBase{0, IPC_RACKMEMSHM_UNLOCK, 0} {};
    ShmUnLockRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_UNLOCK, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmUnLockResponse : MsgBase {
    int32_t errCode_;

    ShmUnLockResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        return UBSM_OK;
    }
};

struct RpcQueryInfoResponse : MsgBase {
    int32_t errCode_;
    std::string name_;

    RpcQueryInfoResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    RpcQueryInfoResponse(int32_t errCode, const std::string &name)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          name_(name){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct DLockClientReinitRequest : MsgBase {
    std::string serverIp_;

    DLockClientReinitRequest() : MsgBase{0, MXM_MSG_BUTT, 0} {};
    explicit DLockClientReinitRequest(const std::string &serverIp) : MsgBase{0, MXM_MSG_BUTT, 0}, serverIp_(serverIp){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(serverIp_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(serverIp_);
        return UBSM_OK;
    }
};

struct DLockClientReinitResponse : MsgBase {
    int32_t errCode_;
    int32_t dLockCode_;

    DLockClientReinitResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };

    DLockClientReinitResponse(int32_t errCode, int32_t dLockerrCode)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          dLockCode_(dLockerrCode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(dLockCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(dLockCode_);
        return UBSM_OK;
    }
};

struct LockRequest : MsgBase {
    std::string memName_;
    bool isExclusive_;
    uint32_t pid_;
    uint32_t uid_;
    uint32_t gid_;

    LockRequest() : MsgBase{0, MXM_MSG_BUTT, 0} {};
    explicit LockRequest(const std::string &memName, bool isExclusive, uint32_t pid, uint32_t uid, uint32_t gid)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          memName_(memName),
          isExclusive_(isExclusive),
          pid_(pid),
          uid_(uid),
          gid_(gid){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(memName_);
        packer.Serialize(isExclusive_);
        packer.Serialize(pid_);
        packer.Serialize(uid_);
        packer.Serialize(gid_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(memName_);
        unpacker.Deserialize(isExclusive_);
        unpacker.Deserialize(pid_);
        unpacker.Deserialize(uid_);
        unpacker.Deserialize(gid_);
        return UBSM_OK;
    }
};

struct UnLockRequest : MsgBase {
    std::string memName_;
    uint32_t pid_;
    uint32_t uid_;
    uint32_t gid_;

    UnLockRequest() : MsgBase{0, MXM_MSG_BUTT, 0} {};
    explicit UnLockRequest(const std::string &memName, uint32_t pid, uint32_t uid, uint32_t gid)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          memName_(memName),
          pid_(pid),
          uid_(uid),
          gid_(gid){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(memName_);
        packer.Serialize(pid_);
        packer.Serialize(uid_);
        packer.Serialize(gid_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(memName_);
        unpacker.Deserialize(pid_);
        unpacker.Deserialize(uid_);
        unpacker.Deserialize(gid_);
        return UBSM_OK;
    }
};
struct DLockResponse : MsgBase {
    int32_t errCode_;
    int32_t dLockCode_;

    DLockResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };

    DLockResponse(int32_t errCode, int32_t dLockerrCode)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          dLockCode_(dLockerrCode){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(dLockCode_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(dLockCode_);
        return UBSM_OK;
    }
};

struct RpcJoinInfoResponse : MsgBase {
    int32_t errCode_;
    int32_t nodetype_;

    RpcJoinInfoResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    RpcJoinInfoResponse(int32_t errCode, int32_t nodetype)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          nodetype_(nodetype){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(nodetype_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(nodetype_);
        return UBSM_OK;
    }
};

struct RpcVoteInfoResponse : MsgBase {
    int32_t errCode_;
    std::string name_;
    bool isGranted_;

    RpcVoteInfoResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    RpcVoteInfoResponse(int32_t errCode, const std::string &name, bool granted)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          name_(name),
          isGranted_(granted){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(name_);
        packer.Serialize(isGranted_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(name_);
        unpacker.Deserialize(isGranted_);
        return UBSM_OK;
    }
};

struct ShmAttachRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmAttachRequest() : MsgBase{0, IPC_RACKMEMSHM_ATTACH, 0} {};
    ShmAttachRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_ATTACH, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmAttachResponse : MsgBase {
    int32_t errCode_;
    std::vector<uint64_t> memIds_{};
    uint64_t shmSize_;
    uint64_t unitSize_;
    uint64_t flag_;
    int oflag_;

    ShmAttachResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmAttachResponse(int32_t errCode, std::vector<uint64_t> &memIds, uint64_t shmSize, uint64_t unitSize, int flag,
                      int oflag)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          memIds_(memIds),
          shmSize_(shmSize),
          unitSize_(unitSize),
          flag_(flag),
          oflag_(oflag){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(memIds_);
        packer.Serialize(shmSize_);
        packer.Serialize(unitSize_);
        packer.Serialize(flag_);
        packer.Serialize(oflag_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(memIds_);
        unpacker.Deserialize(shmSize_);
        unpacker.Deserialize(unitSize_);
        unpacker.Deserialize(flag_);
        unpacker.Deserialize(oflag_);
        return UBSM_OK;
    }
};

struct ShmDetachRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmDetachRequest() : MsgBase{0, IPC_RACKMEMSHM_DETACH, 0} {};
    ShmDetachRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_DETACH, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmLookupRequest : MsgBase {
    std::string regionName_;
    std::string name_;

    ShmLookupRequest() : MsgBase{0, IPC_RACKMEMSHM_LOOKUP, 0} {};
    ShmLookupRequest(const std::string &regionName, const std::string &shmName)
        : MsgBase{0, IPC_RACKMEMSHM_LOOKUP, 0},
          regionName_(regionName),
          name_(shmName){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(name_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(name_);
        return UBSM_OK;
    }
};

struct ShmLookupResponse : MsgBase {
    int32_t errCode_;
    ubsmem_shmem_info_t shmInfo_{};

    ShmLookupResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmLookupResponse(int32_t errCode, ubsmem_shmem_info_t &shmInfo)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          shmInfo_(shmInfo){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(shmInfo_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(shmInfo_);
        return UBSM_OK;
    }
};

struct QueryNodeRequest : MsgBase {
    // true  is queryMasterNode
    // false is queryClientNode
    bool queryMasterNodeFlag_;

    QueryNodeRequest() : MsgBase{0, IPC_RACKMEMSHM_QUERY_NODE, 0} {};
    explicit QueryNodeRequest(bool queryMasterNodeFlag)
        : MsgBase{0, IPC_RACKMEMSHM_QUERY_NODE, 0},
          queryMasterNodeFlag_(queryMasterNodeFlag){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(queryMasterNodeFlag_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(queryMasterNodeFlag_);
        return UBSM_OK;
    }
};

struct QueryNodeResponse : MsgBase {
    int32_t errCode_;
    std::string nodeId_;
    bool nodeIsReady_;

    QueryNodeResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    QueryNodeResponse(int32_t errCode, const std::string &nodeId, bool nodeIsReady)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          nodeId_(nodeId),
          nodeIsReady_(nodeIsReady){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(nodeId_);
        packer.Serialize(nodeIsReady_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(nodeId_);
        unpacker.Deserialize(nodeIsReady_);
        return UBSM_OK;
    }
};

struct QueryDlockStatusResponse : MsgBase {
    int32_t errCode_;
    bool isReady_;

    QueryDlockStatusResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    QueryDlockStatusResponse(int32_t errCode, bool isReady)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          isReady_(isReady){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(isReady_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(isReady_);
        return UBSM_OK;
    }
};

struct ShmListLookupRequest : MsgBase {
    std::string regionName_;
    std::string prefix_;

    ShmListLookupRequest() : MsgBase{0, IPC_RACKMEMSHM_LOOKUP_LIST, 0} {};
    ShmListLookupRequest(const std::string &regionName, const std::string &shmPrefix)
        : MsgBase{0, IPC_RACKMEMSHM_LOOKUP_LIST, 0},
          regionName_(regionName),
          prefix_(shmPrefix){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(regionName_);
        packer.Serialize(prefix_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(regionName_);
        unpacker.Deserialize(prefix_);
        return UBSM_OK;
    }
};

struct ShmListLookupResponse : MsgBase {
    int32_t errCode_;
    std::vector<ubsmem_shmem_desc_t> shmNames_{};

    ShmListLookupResponse() : MsgBase{0, MXM_MSG_BUTT, 0}
    {
        errCode_ = UBSM_ERR_UNIMPL;
    };
    ShmListLookupResponse(int32_t errCode, std::vector<ubsmem_shmem_desc_t> &shmNames)
        : MsgBase{0, MXM_MSG_BUTT, 0},
          errCode_(errCode),
          shmNames_(shmNames){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(shmNames_);
        return UBSM_OK;
    }

    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(shmNames_);
        return UBSM_OK;
    }
};

struct LookupSlotIdResponse : MsgBase {
    int32_t errCode_;
    uint32_t slotId_;

    LookupSlotIdResponse() : MsgBase(0, MXM_MSG_BUTT, 0)
    {
        errCode_ = UBSM_ERR_UNIMPL;
        slotId_ = 0;
    };
    LookupSlotIdResponse(int errCode, uint32_t slotId)
        : MsgBase(0, MXM_MSG_BUTT, 0),
          errCode_(errCode),
          slotId_(slotId){};

    int32_t Serialize(NetMsgPacker &packer) const override
    {
        packer.Serialize(msgVer);
        packer.Serialize(opCode);
        packer.Serialize(destRankId);
        packer.Serialize(errCode_);
        packer.Serialize(slotId_);
        return UBSM_OK;
    }
    int32_t Deserialize(NetMsgUnpacker &unpacker) override
    {
        unpacker.Deserialize(msgVer);
        unpacker.Deserialize(opCode);
        unpacker.Deserialize(destRankId);
        unpacker.Deserialize(errCode_);
        unpacker.Deserialize(slotId_);
        return UBSM_OK;
    }
};

MsgBase *CreateRequestByOpCode(int16_t op);

MsgBase *CreateResponseByOpCode(int16_t op);

MsgBase *CreateRequestByOpCodeInner(int16_t op);

MsgBase *CreateResponseByOpCodeInner(int16_t op);

} // namespace mxmd
} // namespace ock
#endif
