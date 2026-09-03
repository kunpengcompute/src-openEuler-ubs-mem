/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */

#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "BorrowAppMetaMgr.h"

#define MOCKER_CPP(api, TT) (MOCKCPP_NS::mockAPI((#api), (reinterpret_cast<TT>(api))))

namespace UT {
using namespace ock::mxmd;

class AppBorrowMetaMgrTest : public testing::Test {
protected:
    BorrowAppMetaMgr &metaMgr = BorrowAppMetaMgr::GetInstance();
    const void *validAddr = reinterpret_cast<const void *>(0x12345678);
    const void *anotherAddr = reinterpret_cast<const void *>(0x87654321);
    AppBorrowMetaDesc validDesc;

    void SetUp() override
    {
        validDesc = AppBorrowMetaDesc(0);
    }

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(AppBorrowMetaMgrTest, AddMetaNullAddress)
{
    uint32_t result = metaMgr.AddMeta(nullptr, validDesc);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(AppBorrowMetaMgrTest, AddMetaValidAddress)
{
    uint32_t result = metaMgr.AddMeta(validAddr, validDesc);
    EXPECT_EQ(result, UBSM_OK);
    metaMgr.RemoveMeta(validAddr);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaHasUnmappedNullAddress)
{
    int32_t result = metaMgr.SetMetaHasUnmapped(nullptr, true);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaHasUnmappedAddressNotFound)
{
    int32_t result = metaMgr.SetMetaHasUnmapped(validAddr, true);
    EXPECT_EQ(result, MXM_ERR_LEASE_NOT_FOUND);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaHasUnmappedSuccess)
{
    metaMgr.AddMeta(validAddr, validDesc);
    int32_t result = metaMgr.SetMetaHasUnmapped(validAddr, true);
    EXPECT_EQ(result, UBSM_OK);
    EXPECT_TRUE(metaMgr.GetMetaHasUnmapped(validAddr));
    metaMgr.RemoveMeta(validAddr);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaIsLockAddressNullAddress)
{
    int32_t result = metaMgr.SetMetaIsLockAddress(nullptr, true);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaIsLockAddressAddressNotFound)
{
    int32_t result = metaMgr.SetMetaIsLockAddress(validAddr, true);
    EXPECT_EQ(result, MXM_ERR_LEASE_NOT_FOUND);
}

TEST_F(AppBorrowMetaMgrTest, SetMetaIsLockAddressSuccess)
{
    metaMgr.AddMeta(validAddr, validDesc);
    int32_t result = metaMgr.SetMetaIsLockAddress(validAddr, true);
    EXPECT_EQ(result, UBSM_OK);
    EXPECT_TRUE(metaMgr.GetMetaIsLockAddress(validAddr));
    metaMgr.RemoveMeta(validAddr);
}

TEST_F(AppBorrowMetaMgrTest, GetMetaHasUnmappedNullAddress)
{
    bool result = metaMgr.GetMetaHasUnmapped(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(AppBorrowMetaMgrTest, GetMetaIsLockAddressNullAddress)
{
    bool result = metaMgr.GetMetaIsLockAddress(nullptr);
    EXPECT_FALSE(result);
}

TEST_F(AppBorrowMetaMgrTest, RemoveMetaNullAddress)
{
    uint32_t result = metaMgr.RemoveMeta(nullptr);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(AppBorrowMetaMgrTest, RemoveMetaSuccess)
{
    metaMgr.AddMeta(validAddr, validDesc);
    uint32_t result = metaMgr.RemoveMeta(validAddr);
    EXPECT_EQ(result, UBSM_OK);

    // Verify removal
    AppBorrowMetaDesc desc;
    EXPECT_EQ(metaMgr.GetMeta(validAddr, desc), MXM_ERR_LEASE_NOT_FOUND);
}

TEST_F(AppBorrowMetaMgrTest, GetMetaNullAddress)
{
    AppBorrowMetaDesc desc;
    uint32_t result = metaMgr.GetMeta(nullptr, desc);
    EXPECT_EQ(result, MXM_ERR_PARAM_INVALID);
}

TEST_F(AppBorrowMetaMgrTest, GetMetaAddressNotFound)
{
    AppBorrowMetaDesc desc;
    uint32_t result = metaMgr.GetMeta(validAddr, desc);
    EXPECT_EQ(result, MXM_ERR_LEASE_NOT_FOUND);
}

TEST_F(AppBorrowMetaMgrTest, ConcurrentAccessDifferentSegments)
{
    const void *addrSeg1 = reinterpret_cast<const void *>(0x1000);
    const void *addrSeg2 = reinterpret_cast<const void *>(0x20000000);

    // Should not block each other since different segments
    std::thread t1([&]() {
        metaMgr.AddMeta(addrSeg1, validDesc);
        metaMgr.SetMetaHasUnmapped(addrSeg1, true);
    });

    std::thread t2([&]() {
        metaMgr.AddMeta(addrSeg2, validDesc);
        metaMgr.SetMetaIsLockAddress(addrSeg2, true);
    });

    t1.join();
    t2.join();

    EXPECT_TRUE(metaMgr.GetMetaHasUnmapped(addrSeg1));
    EXPECT_TRUE(metaMgr.GetMetaIsLockAddress(addrSeg2));
}
} // namespace UT