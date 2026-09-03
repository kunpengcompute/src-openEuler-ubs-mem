#include "common_def.h"
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

using testing::Test;

namespace UT {

class CommonDefTestSuite : public Test {
protected:
    void SetUp() override {}
    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(CommonDefTestSuite, TestSafeFreeNull)
{
    void *ptr = nullptr;
    SafeFree(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeFreeNonNull)
{
    void *ptr = malloc(100);
    EXPECT_NE(ptr, nullptr);
    SafeFree(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteNull)
{
    int *ptr = nullptr;
    SafeDelete(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteNonNull)
{
    int *ptr = new int(42);
    EXPECT_NE(ptr, nullptr);
    SafeDelete(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteArrayNull)
{
    int *ptr = nullptr;
    SafeDeleteArray(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteArrayNonNull)
{
    int *ptr = new int[10];
    EXPECT_NE(ptr, nullptr);
    SafeDeleteArray(ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteArrayWithLenNull)
{
    int *ptr = nullptr;
    SafeDeleteArray(ptr, static_cast<size_t>(5));
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestSafeDeleteArrayWithLenZero)
{
    int *ptr = new int[10];
    SafeDeleteArray(ptr, static_cast<size_t>(0));
    EXPECT_NE(ptr, nullptr);
    delete[] ptr;
}

TEST_F(CommonDefTestSuite, TestSafeDeleteArrayWithLenNonNull)
{
    int *ptr = new int[10];
    SafeDeleteArray(ptr, static_cast<size_t>(10));
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(CommonDefTestSuite, TestPairHash)
{
    PairHash<int, int> hasher;
    auto h1 = hasher({1, 2});
    auto h2 = hasher({1, 2});
    EXPECT_EQ(h1, h2);
    auto h3 = hasher({2, 1});
    EXPECT_NE(h1, h3);
}

TEST_F(CommonDefTestSuite, TestPairMap)
{
    PairMap<int, int, std::string> map;
    map[{1, 2}] = "hello";
    map[{3, 4}] = "world";
    EXPECT_EQ(map.size(), static_cast<size_t>(2));
    auto key1 = std::make_pair(1, 2);
    EXPECT_EQ(map[key1], "hello");
    auto key2 = std::make_pair(3, 4);
    EXPECT_EQ(map[key2], "world");
}
} // namespace UT
