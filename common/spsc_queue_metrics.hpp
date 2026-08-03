#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>



/*
 * Runtime statistics collected by SPSCRingBuffer.
 *
 * These counters provide lightweight observability
 * without affecting queue correctness.
 *
 * Metrics are intended to be periodically sampled by
 * monitoring components such as Prometheus exporters.
 */
struct SPSCQueueMetrics
{
    std::atomic<std::uint64_t> pushCount{0};

    std::atomic<std::uint64_t> popCount{0};

    std::atomic<std::uint64_t> pushFailures{0};

    std::atomic<std::uint64_t> popFailures{0};

    std::atomic<std::size_t> highWaterMark{0};
};
