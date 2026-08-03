/*
 * Lock-Free Single Producer Single Consumer Ring Buffer
 *
 * Design Goals
 * ------------
 * - Wait-free push/pop
 * - Fixed capacity
 * - No dynamic allocation
 * - Cache-line aligned
 * - False-sharing resistant
 * - Acquire/Release synchronization
 *
 * Intended Usage
 * --------------
 * Producer Thread
 *        │
 *        ▼
 *   SPSC Ring Buffer
 *        │
 *        ▼
 * Consumer Thread
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

#include "common/spsc_queue_metrics.hpp"

template<typename T, std::size_t Capacity>
class SPSCRingBuffer
{
    static_assert(
        Capacity > 1,
        "Capacity must be greater than one");

public:
    SPSCRingBuffer() = default;

    bool tryPush(const T& value)
    {
        const std::size_t head =
            head_.load(std::memory_order_relaxed);

        const std::size_t nextHead =
            increment(head);

        const std::size_t tail =
            tail_.load(std::memory_order_acquire);

        if (nextHead == tail)
        {
            metrics_.pushFailures.fetch_add(
                1,
                std::memory_order_relaxed);

            return false;
        }

        buffer_[head] = value;

        head_.store(
            nextHead,
            std::memory_order_release);

        metrics_.pushCount.fetch_add(
            1,
            std::memory_order_relaxed);

        // Calculate the new depth without reloading head_ and tail_.
        const std::size_t currentSize =
            nextHead >= tail
                ? nextHead - tail
                : Capacity - tail + nextHead;

        const std::size_t previousHigh =
            metrics_.highWaterMark.load(
                std::memory_order_relaxed);


        // Only the producer updates highWaterMark.
        // Relaxed ordering is sufficient for this SPSC implementation.
        if (currentSize > previousHigh)
        {
            metrics_.highWaterMark.store(
                currentSize,
                std::memory_order_relaxed);
        }

        return true;
    } 

    bool tryPop(T& value)
    {
        const std::size_t tail =
            tail_.load(std::memory_order_relaxed);

        const std::size_t head =
            head_.load(std::memory_order_acquire);

        if (tail == head)
        {
            metrics_.popFailures.fetch_add(
                1,
                std::memory_order_relaxed);

            return false;
        }

        value = buffer_[tail];

        tail_.store(
            increment(tail),
            std::memory_order_release);

        metrics_.popCount.fetch_add(
            1,
            std::memory_order_relaxed);

        return true;
    }

    [[nodiscard]]
    const SPSCQueueMetrics& metrics() const noexcept
    {
        return metrics_;
    }

    [[nodiscard]] bool empty() const
    {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool full() const
    {
        const std::size_t head =
            head_.load(std::memory_order_acquire);

        return increment(head) ==
               tail_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size() const
    {
        const std::size_t head =
            head_.load(std::memory_order_acquire);

        const std::size_t tail =
            tail_.load(std::memory_order_acquire);

        if (head >= tail)
            return head - tail;

        return Capacity - tail + head;
    }

    [[nodiscard]] constexpr std::size_t capacity() const
    {
        return Capacity - 1;
    }

    

private:
    static constexpr std::size_t increment(
        std::size_t index)
    {
        return (index + 1) % Capacity;
    }

    alignas(64)
    std::atomic<std::size_t> head_{0};

    alignas(64)
    std::atomic<std::size_t> tail_{0};

    alignas(64)
    std::array<T, Capacity> buffer_;

    SPSCQueueMetrics metrics_;
};