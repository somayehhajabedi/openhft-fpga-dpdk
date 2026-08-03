/*
 * SPSC Ring Buffer Unit Tests
 *
 * Purpose
 * -------
 * These tests verify the correctness of the lock-free
 * Single Producer Single Consumer (SPSC) ring buffer.
 *
 * The following behaviors are validated:
 *
 *  - Empty queue after construction
 *  - Push operation
 *  - Pop operation
 *  - FIFO ordering
 *  - Full queue detection
 *  - Empty queue detection
 *
 * These tests validate functional correctness only.
 *
 * Performance, throughput, latency, cache behavior,
 * and backpressure are measured separately using
 * dedicated Google Benchmark benchmarks.
 */

#include <gtest/gtest.h>

#include "common/spsc_ring_buffer.hpp"

TEST(SPSCRingBufferTest, EmptyAfterConstruction)
{
    SPSCRingBuffer<int, 8> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0U);
}

TEST(SPSCRingBufferTest, PushOneElement)
{
    SPSCRingBuffer<int, 8> queue;

    EXPECT_TRUE(queue.tryPush(42));

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1U);
}

TEST(SPSCRingBufferTest, PushAndPop)
{
    SPSCRingBuffer<int, 8> queue;

    ASSERT_TRUE(queue.tryPush(123));

    int value{};

    ASSERT_TRUE(queue.tryPop(value));

    EXPECT_EQ(value, 123);

    EXPECT_TRUE(queue.empty());
}

TEST(SPSCRingBufferTest, FIFOOrder)
{
    SPSCRingBuffer<int, 8> queue;

    for (int i = 0; i < 5; ++i)
    {
        ASSERT_TRUE(queue.tryPush(i));
    }

    for (int i = 0; i < 5; ++i)
    {
        int value{};

        ASSERT_TRUE(queue.tryPop(value));

        EXPECT_EQ(value, i);
    }
}

TEST(SPSCRingBufferTest, FullQueue)
{
    SPSCRingBuffer<int, 4> queue;

    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_TRUE(queue.tryPush(3));

    EXPECT_TRUE(queue.full());

    EXPECT_FALSE(queue.tryPush(4));
}

TEST(SPSCRingBufferTest, EmptyQueue)
{
    SPSCRingBuffer<int, 8> queue;

    int value{};

    EXPECT_FALSE(queue.tryPop(value));
}

TEST(SPSCRingBufferTest, ReportsUsableCapacity)
{
    SPSCRingBuffer<int, 4> queue;

    // One slot is intentionally left unused to distinguish full from empty.
    EXPECT_EQ(queue.capacity(), 3U);
}

TEST(SPSCRingBufferTest, PreservesFIFOOrderAfterWrapAround)
{
    SPSCRingBuffer<int, 4> queue;

    ASSERT_TRUE(queue.tryPush(1));
    ASSERT_TRUE(queue.tryPush(2));
    ASSERT_TRUE(queue.tryPush(3));

    EXPECT_TRUE(queue.full());

    int value{};

    ASSERT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 1);

    ASSERT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 2);

    // head_ wraps back to the beginning of the underlying array.
    ASSERT_TRUE(queue.tryPush(4));
    ASSERT_TRUE(queue.tryPush(5));

    EXPECT_TRUE(queue.full());

    ASSERT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 3);

    ASSERT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 4);

    ASSERT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 5);

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0U);
}



