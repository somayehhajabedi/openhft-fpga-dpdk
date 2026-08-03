/*
 * SPSC Queue Metrics Tests
 *
 * Purpose
 * -------
 * Verifies that runtime metrics exposed by the
 * SPSCRingBuffer accurately reflect queue activity.
 *
 * The following behaviors are validated:
 *
 *  - Successful push operations
 *  - Successful pop operations
 *  - Push failures when the queue is full
 *  - Pop failures when the queue is empty
 *  - High water mark tracking
 *
 * These tests verify only metric correctness.
 * Queue functionality is validated separately in
 * spsc_ring_buffer_test.cpp.
 */

#include <gtest/gtest.h>

#include "common/spsc_ring_buffer.hpp"


TEST(SPSCQueueMetricsTest, CountsSuccessfulPushes)
{
    SPSCRingBuffer<int, 8> queue;

    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_TRUE(queue.tryPush(3));

    EXPECT_EQ(
        queue.metrics().pushCount.load(),
        3U);

    EXPECT_EQ(
        queue.metrics().pushFailures.load(),
        0U);
}

TEST(SPSCQueueMetricsTest, CountsSuccessfulPops)
{
    SPSCRingBuffer<int, 8> queue;

    queue.tryPush(1);
    queue.tryPush(2);

    int value{};

    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_TRUE(queue.tryPop(value));

    EXPECT_EQ(
        queue.metrics().popCount.load(),
        2U);

    EXPECT_EQ(
        queue.metrics().popFailures.load(),
        0U);
}

TEST(SPSCQueueMetricsTest, CountsPushFailures)
{
    SPSCRingBuffer<int, 4> queue;

    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_TRUE(queue.tryPush(3));

    EXPECT_FALSE(queue.tryPush(4));

    EXPECT_EQ(
        queue.metrics().pushFailures.load(),
        1U);
}

TEST(SPSCQueueMetricsTest, CountsPopFailures)
{
    SPSCRingBuffer<int, 8> queue;

    int value{};

    EXPECT_FALSE(queue.tryPop(value));

    EXPECT_EQ(
        queue.metrics().popFailures.load(),
        1U);
}

TEST(SPSCQueueMetricsTest, TracksHighWaterMark)
{
    SPSCRingBuffer<int, 8> queue;

    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_TRUE(queue.tryPush(3));

    int value{};

    EXPECT_TRUE(queue.tryPop(value));

    EXPECT_TRUE(queue.tryPush(4));

    EXPECT_EQ(
        queue.metrics()
            .highWaterMark.load(),
        3U);
}

TEST(SPSCQueueMetricsTest, TracksMixedOperations)
{
    SPSCRingBuffer<int, 4> queue;

    int value{};

    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));

    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(queue.tryPush(3));
    EXPECT_TRUE(queue.tryPush(4));

    // Queue usable capacity is 3, so this push must fail.
    EXPECT_FALSE(queue.tryPush(5));

    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 2);

    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 3);

    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 4);

    EXPECT_FALSE(queue.tryPop(value));

    EXPECT_EQ(
        queue.metrics().pushCount.load(
            std::memory_order_relaxed),
        4U);

    EXPECT_EQ(
        queue.metrics().popCount.load(
            std::memory_order_relaxed),
        4U);

    EXPECT_EQ(
        queue.metrics().pushFailures.load(
            std::memory_order_relaxed),
        1U);

    EXPECT_EQ(
        queue.metrics().popFailures.load(
            std::memory_order_relaxed),
        1U);

    EXPECT_EQ(
        queue.metrics().highWaterMark.load(
            std::memory_order_relaxed),
        3U);
}




