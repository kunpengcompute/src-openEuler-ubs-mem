#include "openssl_dl.h"
#include <gtest/gtest.h>

static const char *TEST_CERT_DIR = "../3rdparty/build/test_certs";

TEST(OpensslDlTest, InitAndCleanup)
{
    int ret = InitOpensslDl();
    EXPECT_EQ(ret, 0);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, InitTwice)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    EXPECT_EQ(InitOpensslDl(), 0);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateInvalidPath)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    int ret = VerifyCertificate("/nonexistent/ca.pem", "/nonexistent/cert.pem", nullptr, "CA", "CERT", 43200);
    EXPECT_NE(ret, 0);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateNoCrl)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    std::string base = TEST_CERT_DIR;
    std::string caFile = base + "/ca/ca.pem";
    std::string certFile = base + "/certs/cert1.pem";
    int ret = VerifyCertificate(caFile.c_str(), certFile.c_str(), nullptr, "ca.pem", "cert1.pem", 43200);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateWithCrl)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    std::string base = TEST_CERT_DIR;
    std::string caFile = base + "/ca/ca.pem";
    std::string certFile = base + "/certs/cert1.pem";
    std::string crlFile = base + "/crl/ca.crl.pem";
    int ret = VerifyCertificate(caFile.c_str(), certFile.c_str(), crlFile.c_str(), "ca.pem", "cert1.pem", 43200);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateExpireCheck)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    std::string base = TEST_CERT_DIR;
    std::string caFile = base + "/ca/ca.pem";
    std::string certFile = base + "/certs/cert1.pem";
    int ret = VerifyCertificate(caFile.c_str(), certFile.c_str(), nullptr, "ca.pem", "cert1.pem", 99999999);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateInvalidCaPath)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    int ret =
        VerifyCertificate("/tmp/nonexistent_ca.pem", "/tmp/nonexistent_cert.pem", nullptr, "ca.pem", "cert.pem", 43200);
    EXPECT_NE(ret, 0);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateCertExpired)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    std::string base = TEST_CERT_DIR;
    std::string caFile = base + "/ca/ca.pem";
    std::string certFile = base + "/certs/cert2.pem";
    int ret = VerifyCertificate(caFile.c_str(), certFile.c_str(), nullptr, "ca.pem", "cert2.pem", 0);
    CleanupOpensslDl();
}

TEST(OpensslDlTest, VerifyCertificateWithCrlCheckExpired)
{
    EXPECT_EQ(InitOpensslDl(), 0);
    std::string base = TEST_CERT_DIR;
    std::string caFile = base + "/ca/ca.pem";
    std::string certFile = base + "/certs/cert1.pem";
    std::string crlFile = base + "/crl/ca.crl.pem";
    int ret = VerifyCertificate(caFile.c_str(), certFile.c_str(), crlFile.c_str(), "ca.pem", "cert1.pem", 0);
    CleanupOpensslDl();
}
