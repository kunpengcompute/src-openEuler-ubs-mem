/*
* Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#ifndef MXM_RPC_TEST_H
#define MXM_RPC_TEST_H
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "mxm_rpc_server_interface.h"
#include "rpc_config.h"

namespace UT {
#ifndef MOCKER_CPP
#define MOCKER_CPP(api, TT) MOCKCPP_NS::mockAPI(#api, reinterpret_cast<TT>(api))
#endif
class RpcTestSuite : public testing::Test {
public:
    void SetUp() override
    {
        GlobalMockObject::reset();
        ock::rpc::NetRpcConfig::GetInstance().SetLocalNode(std::make_pair("127.0.0.1", 1234));
        std::vector<std::pair<std::string, uint16_t>> nodes;
        nodes.push_back(std::make_pair("127.0.0.1", 12345));
        ock::rpc::NetRpcConfig::GetInstance().SetRemoteNodes(nodes);
    };

    void TearDown() override
    {
        GlobalMockObject::verify();
        GlobalMockObject::reset();
        ock::com::rpc::MxmComStopRpcServer();
    };
};
} // namespace UT
#endif // MXM_RPC_TEST_H