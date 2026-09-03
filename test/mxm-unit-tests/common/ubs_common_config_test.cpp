/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "ubs_common_config.h"

using testing::Test;

namespace UT {
using namespace ock::ubsm;

class UbsCommonConfigTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(UbsCommonConfigTestSuite, TestSetGetSwitch)
{
    bool original = UbsCommonConfig::GetInstance().GetLeaseCachedSwitch();
    UbsCommonConfig::GetInstance().SetLeaseCacheSwitch(true);
    ASSERT_TRUE(UbsCommonConfig::GetInstance().GetLeaseCachedSwitch());
    UbsCommonConfig::GetInstance().SetLeaseCacheSwitch(original);

    original = UbsCommonConfig::GetInstance().GetTlsSwitch();
    UbsCommonConfig::GetInstance().SetTlsSwitch(true);
    ASSERT_TRUE(UbsCommonConfig::GetInstance().GetTlsSwitch());
    UbsCommonConfig::GetInstance().SetTlsSwitch(original);
}

TEST_F(UbsCommonConfigTestSuite, TestSetCipherSuit)
{
    ock::hcom::UBSHcomNetCipherSuite originalSuite = UbsCommonConfig::GetInstance().GetCipherSuit();
    std::string originalChiper;
    if (originalSuite == ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128) {
        originalChiper = CIPHER_STR_AES_GCM_128;
    } else if (originalSuite == ock::hcom::UBSHcomNetCipherSuite::AES_GCM_256) {
        originalChiper = CIPHER_STR_AES_GCM_256;
    } else if (originalSuite == ock::hcom::UBSHcomNetCipherSuite::AES_CCM_128) {
        originalChiper = CIPHER_STR_AES_CCM_128;
    } else if (originalSuite == ock::hcom::UBSHcomNetCipherSuite::CHACHA20_POLY1305) {
        originalChiper = CIPHER_STR_CHACHA20_POLY1305;
    } else {
        originalChiper = "";
    }
    UbsCommonConfig::GetInstance().SetCipherSuit(CIPHER_STR_AES_GCM_128);
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCipherSuit(), ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128);

    UbsCommonConfig::GetInstance().SetCipherSuit(CIPHER_STR_AES_GCM_256);
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCipherSuit(), ock::hcom::UBSHcomNetCipherSuite::AES_GCM_256);

    UbsCommonConfig::GetInstance().SetCipherSuit(CIPHER_STR_AES_CCM_128);
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCipherSuit(), ock::hcom::UBSHcomNetCipherSuite::AES_CCM_128);

    UbsCommonConfig::GetInstance().SetCipherSuit(CIPHER_STR_CHACHA20_POLY1305);
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCipherSuit(), ock::hcom::UBSHcomNetCipherSuite::CHACHA20_POLY1305);

    UbsCommonConfig::GetInstance().SetCipherSuit("");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCipherSuit(), ock::hcom::UBSHcomNetCipherSuite::AES_GCM_128);

    UbsCommonConfig::GetInstance().SetCipherSuit(originalChiper);
}

TEST_F(UbsCommonConfigTestSuite, TestSetGetPath)
{
    std::string original = UbsCommonConfig::GetInstance().GetCaPath();
    UbsCommonConfig::GetInstance().SetCaPath("/test/ca");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCaPath(), "/test/ca");
    UbsCommonConfig::GetInstance().SetCaPath(original);

    original = UbsCommonConfig::GetInstance().GetCrlPath();
    UbsCommonConfig::GetInstance().SetCrlPath("/test/crl");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCrlPath(), "/test/crl");
    UbsCommonConfig::GetInstance().SetCrlPath(original);

    original = UbsCommonConfig::GetInstance().GetCertPath();
    UbsCommonConfig::GetInstance().SetCertPath("/test/cert");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetCertPath(), "/test/cert");
    UbsCommonConfig::GetInstance().SetCertPath(original);

    original = UbsCommonConfig::GetInstance().GetKeyPath();
    UbsCommonConfig::GetInstance().SetKeyPath("/test/key");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetKeyPath(), "/test/key");
    UbsCommonConfig::GetInstance().SetKeyPath(original);

    original = UbsCommonConfig::GetInstance().GetKeypassPath();
    UbsCommonConfig::GetInstance().SetKeypassPath("/test/keypass");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetKeypassPath(), "/test/keypass");
    UbsCommonConfig::GetInstance().SetKeypassPath(original);

    original = UbsCommonConfig::GetInstance().GetKsfMasterPath();
    UbsCommonConfig::GetInstance().SetKsfMasterPath("/test/ksfmaster");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetKsfMasterPath(), "/test/ksfmaster");
    UbsCommonConfig::GetInstance().SetKsfMasterPath(original);

    original = UbsCommonConfig::GetInstance().GetKsfStandbyPath();
    UbsCommonConfig::GetInstance().SetKsfStandbyPath("/test/ksfstandby");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetKsfStandbyPath(), "/test/ksfstandby");
    UbsCommonConfig::GetInstance().SetKsfStandbyPath(original);
}

TEST_F(UbsCommonConfigTestSuite, TestSetGetLockPath)
{
    std::string original = UbsCommonConfig::GetInstance().GetLockCaPath();
    UbsCommonConfig::GetInstance().SetLockCaPath("/test/lockca");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockCaPath(), "/test/lockca");
    UbsCommonConfig::GetInstance().SetLockCaPath(original);

    original = UbsCommonConfig::GetInstance().GetLockCrlPath();
    UbsCommonConfig::GetInstance().SetLockCrlPath("/test/lockcrl");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockCrlPath(), "/test/lockcrl");
    UbsCommonConfig::GetInstance().SetLockCrlPath(original);

    original = UbsCommonConfig::GetInstance().GetLockCertPath();
    UbsCommonConfig::GetInstance().SetLockCertPath("/test/lockcert");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockCertPath(), "/test/lockcert");
    UbsCommonConfig::GetInstance().SetLockCertPath(original);

    original = UbsCommonConfig::GetInstance().GetLockKeyPath();
    UbsCommonConfig::GetInstance().SetLockKeyPath("/test/lockkey");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockKeyPath(), "/test/lockkey");
    UbsCommonConfig::GetInstance().SetLockKeyPath(original);

    original = UbsCommonConfig::GetInstance().GetLockKeypassPath();
    UbsCommonConfig::GetInstance().SetLockKeypassPath("/test/lockkeypass");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockKeypassPath(), "/test/lockkeypass");
    UbsCommonConfig::GetInstance().SetLockKeypassPath(original);

    original = UbsCommonConfig::GetInstance().GetLockKsfMasterPath();
    UbsCommonConfig::GetInstance().SetLockKsfMasterPath("/test/lockksfmaster");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockKsfMasterPath(), "/test/lockksfmaster");
    UbsCommonConfig::GetInstance().SetLockKsfMasterPath(original);

    original = UbsCommonConfig::GetInstance().GetLockKsfStandbyPath();
    UbsCommonConfig::GetInstance().SetLockKsfStandbyPath("/test/lockksfstandby");
    ASSERT_EQ(UbsCommonConfig::GetInstance().GetLockKsfStandbyPath(), "/test/lockksfstandby");
    UbsCommonConfig::GetInstance().SetLockKsfStandbyPath(original);
}
} // namespace UT
