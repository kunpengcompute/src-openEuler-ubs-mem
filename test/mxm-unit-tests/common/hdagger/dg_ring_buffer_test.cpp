/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 */
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>

#include "common/hdagger/container/dg_ring_buffer.h"

using testing::Test;

namespace UT {
using namespace ock::dagger;

class DgRingBufferTestSuite : public Test {
protected:
    void SetUp() override {}

    void TearDown() override
    {
        mockcpp::GlobalMockObject::verify();
        mockcpp::GlobalMockObject::reset();
    }
};

TEST_F(DgRingBufferTestSuite, TestRingBufferInit)
{
    RingBuffer<uint32_t> rb(0);
    ASSERT_NE(rb.Initialize(), 0);
    rb.Capacity(10);
    ASSERT_EQ(rb.Initialize(), 0);
    ASSERT_EQ(rb.Capacity(), 10);

    RingBuffer<uint32_t> rb1(100);
    ASSERT_EQ(rb1.Initialize(), 0);

    rb1.Capacity(10);
    ASSERT_NE(rb1.Capacity(), 10);
    ASSERT_EQ(rb1.Capacity(), 100);
}

TEST_F(DgRingBufferTestSuite, TestRingBufferExecuteFront)
{
    RingBuffer<uint32_t> rb(2);
    ASSERT_EQ(rb.Initialize(), 0);
    ASSERT_EQ(rb.Size(), 0);

    /* test PushFront */
    ASSERT_EQ(rb.PushFront(0), true);
    ASSERT_EQ(rb.Size(), 1);
    ASSERT_EQ(rb.PushFront(1), true);
    ASSERT_EQ(rb.Size(), 2);
    ASSERT_EQ(rb.PushFront(2), false);
    ASSERT_EQ(rb.Size(), 2);

    /* test PopFront */
    uint32_t item = 0;
    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(item, 1);
    ASSERT_EQ(rb.Size(), 1);

    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(item, 0);
    ASSERT_EQ(rb.Size(), 0);

    ASSERT_EQ(rb.PopFront(item), false);

    /* test PushFrontN */
    uint32_t items[3] = {0, 1, 2};
    ASSERT_EQ(rb.PushFrontN(items, 3), false);
    ASSERT_EQ(rb.PushFrontN(items, 2), true);
    ASSERT_EQ(rb.Size(), 2);

    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(item, 1);
    ASSERT_EQ(rb.Size(), 1);

    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(item, 0);
    ASSERT_EQ(rb.Size(), 0);

    ASSERT_EQ(rb.PopFront(item), false);

    /* test PopFrontN */
    uint32_t items1[3] = {0, 1, 2};
    ASSERT_EQ(rb.PushFrontN(items, 2), true);
    ASSERT_EQ(rb.PopFrontN(items1, 3), false);
    ASSERT_EQ(rb.PopFrontN(items1, 2), true);
    ASSERT_EQ(rb.Size(), 0);
    ASSERT_EQ(items1[0], 1);
    ASSERT_EQ(items1[1], 0);

    /* test PopFrontNFlex */
    uint32_t n = 3;
    ASSERT_EQ(rb.PushFrontN(items, 2), true);
    ASSERT_EQ(rb.PopFrontNFlex(items1, n), true);
    ASSERT_EQ(rb.Size(), 0);
    ASSERT_EQ(n, 2);
}

TEST_F(DgRingBufferTestSuite, TestRingBufferExecuteBack)
{
    RingBuffer<uint32_t> rb(2);
    ASSERT_EQ(rb.Initialize(), 0);
    ASSERT_EQ(rb.Size(), 0);

    /* test PushBack */
    ASSERT_EQ(rb.PushBack(0), true);
    ASSERT_EQ(rb.Size(), 1);
    ASSERT_EQ(rb.PushBack(1), true);
    ASSERT_EQ(rb.Size(), 2);
    ASSERT_EQ(rb.PushBack(2), false);
    ASSERT_EQ(rb.Size(), 2);

    uint32_t item = 0;
    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(rb.PopFront(item), true);
    ASSERT_EQ(rb.Size(), 0);

    /* test PushBackN */
    uint32_t items[3] = {0, 1, 2};
    uint32_t items1[3] = {0, 1, 2};
    ASSERT_EQ(rb.PushBackN(items, 2), true);
    ASSERT_EQ(rb.Size(), 2);
    ASSERT_EQ(rb.PushBackN(items, 2), false);
    ASSERT_EQ(rb.Size(), 2);
    ASSERT_EQ(rb.PopFrontN(items1, 3), false);
    ASSERT_EQ(rb.PopFrontN(items1, 2), true);
    ASSERT_EQ(rb.Size(), 0);
    ASSERT_EQ(items1[0], 0);
    ASSERT_EQ(items1[1], 1);
}

TEST_F(DgRingBufferTestSuite, TestRingBufferQueue)
{
    uint32_t item1 = 1;
    const uint32_t item2 = 2;
    uint32_t item3 = 3;
    const uint32_t item4 = 4;

    RingBufferBlockingQueue<uint32_t> queue(16);
    ASSERT_EQ(queue.Initialize(), 0);
    ASSERT_EQ(queue.Enqueue(item1), true);
    ASSERT_EQ(queue.Enqueue(item2), true);
    ASSERT_EQ(queue.EnqueueFirst(item3), true);
    ASSERT_EQ(queue.EnqueueFirst(item4), true);

    uint32_t out = 0;
    ASSERT_EQ(queue.Dequeue(out), true);
    ASSERT_EQ(out, 4);
    ASSERT_EQ(queue.Dequeue(out), true);
    ASSERT_EQ(out, 3);
    ASSERT_EQ(queue.Dequeue(out), true);
    ASSERT_EQ(out, 1);
    ASSERT_EQ(queue.Dequeue(out), true);
    ASSERT_EQ(out, 2);
}
} // namespace UT
