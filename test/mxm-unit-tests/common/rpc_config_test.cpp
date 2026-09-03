/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "rpc_config.h"

using testing::Test;

namespace UT {
using namespace ock::rpc;

class RpcConfigTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(RpcConfigTestSuite, TestGetNodes)
{
    std::vector<RpcNode> nodes;
    RpcNode node;
    NetRpcConfig::GetInstance().GetNodes(nodes, node);

    std::vector<RpcNode> remoteNodes = NetRpcConfig::GetInstance().GetRemoteNodes();
    RpcNode localNode = NetRpcConfig::GetInstance().GetLocalNode();
    ASSERT_EQ(node.name, localNode.name);
    ASSERT_EQ(nodes.size(), remoteNodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        ASSERT_EQ(nodes[i].name, remoteNodes[i].name);
    }
}

TEST_F(RpcConfigTestSuite, TestParseRpcNodeFromId)
{
    RpcNode node{"0.0.0.0:0", "0.0.0.0", 0};
    ASSERT_EQ(NetRpcConfig::GetInstance().ParseRpcNodeFromId("0", node), MXM_ERR_PARAM_INVALID);
    ASSERT_EQ(NetRpcConfig::GetInstance().ParseRpcNodeFromId("1.1.1.1:-1", node), MXM_ERR_PARAM_INVALID);
    ASSERT_EQ(NetRpcConfig::GetInstance().ParseRpcNodeFromId("1.1.1.1:1", node), UBSM_OK);
    ASSERT_EQ(node.name, "1.1.1.1:1");
}
} // namespace UT
