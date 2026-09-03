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

#ifndef RPC_CONFIG_H
#define RPC_CONFIG_H
#include <string>
#include <vector>
#include "ubs_mem_def.h"
#include "log.h"
#include "rack_mem_err.h"

using namespace ock::common;

namespace ock::rpc {
constexpr auto IP_PORT_SIZE = 2;
struct RpcNode {
    std::string name;
    std::string ip;
    uint16_t port;
};

enum class NodeType { ELIGIBLE_NODE = 0, VOTING_ONLY_NODE = 1 };

struct ClusterNode {
    std::string id;
    std::string ip;
    uint16_t port;
    bool isMaster;
    long lastSeen;
    bool isActive;
    NodeType type;
};

class NetRpcConfig {
public:
    static NetRpcConfig &GetInstance()
    {
        static NetRpcConfig config;
        return config;
    }

    void SetLocalNode(const std::pair<std::string, uint16_t> &ipAndPort)
    {
        mNode = {ipAndPort.first + ":" + std::to_string(ipAndPort.second), ipAndPort.first, ipAndPort.second};
    }

    void SetRemoteNodes(const std::vector<std::pair<std::string, uint16_t>> &ipAndPortList)
    {
        for (auto &item : ipAndPortList) {
            RpcNode node = {item.first + ":" + std::to_string(item.second), item.first, item.second};
            mNodes.push_back(node);
        }
    }

    void GetNodes(std::vector<RpcNode> &nodes, RpcNode &node)
    {
        nodes = mNodes;
        node = mNode;
    }

    int32_t ParseRpcNodeFromId(const std::string &nodeId, RpcNode &node)
    {
        DBG_LOGDEBUG("Parsing rpc node info, nodeID=" << nodeId);
        std::vector<std::string> result;
        std::stringstream ss(nodeId);
        std::string item;
        while (std::getline(ss, item, ':')) {
            result.push_back(item);
        }
        if (result.size() != IP_PORT_SIZE) {
            DBG_LOGERROR("Invalid nodeId received, nodeId=" << nodeId);
            return MXM_ERR_PARAM_INVALID;
        }
        node.name = nodeId;
        node.ip = result.front();
        uint32_t port = 0;
        if (!dagger::StrUtil::StrToUint(result.back(), port)) {
            DBG_LOGERROR("Invalid port received, nodeId=" << nodeId);
            return MXM_ERR_PARAM_INVALID;
        }
        if (port > UINT16_MAX) {
            DBG_LOGERROR("Invalid node ID received, nodeId=" << nodeId);
            return MXM_ERR_PARAM_INVALID;
        }
        node.port = static_cast<uint16_t>(port);
        DBG_LOGDEBUG("Successfully extracted IP=" << node.ip << ", Port=" << node.port << " from nodeId=" << nodeId);
        return MXM_OK;
    }

    const RpcNode &GetLocalNode() const
    {
        return mNode;
    }

    const std::vector<RpcNode> &GetRemoteNodes() const
    {
        return mNodes;
    }

private:
    NetRpcConfig() = default;
    ~NetRpcConfig() = default;
    std::vector<RpcNode> mNodes;
    RpcNode mNode;
};
} // namespace ock::rpc

#endif // RPC_CONFIG_H
