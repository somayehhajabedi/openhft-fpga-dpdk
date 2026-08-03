/*
 * Lock-Free Queue Benchmark
 *
 * This benchmark compares:
 *
 *   std::queue + std::mutex
 *
 *                versus
 *
 *   SPSCRingBuffer
 *
 * Metrics:
 *
 * - Throughput
 * - Average latency
 * - Cache misses
 * - Branch misses
 * - CPU cycles
 *
 * perf stat is used for hardware performance counters.
 
 *
 * Purpose
 * -------
 * This benchmark compares two producer-consumer queue designs:
 *
 *   1. std::queue protected by std::mutex
 *   2. SPSCRingBuffer using lock-free SPSC synchronization
 *
 * Both benchmarks transfer real Order objects from the project between
 * one producer thread and one consumer thread.
 *
 * The benchmark measures:
 *
 *   - End-to-end message throughput
 *   - Producer/consumer synchronization overhead
 *   - The cost of mutex-based coordination
 *   - The benefit of fixed-capacity, allocation-free SPSC transfer
 *
 * The SPSC queue is intended for pipeline boundaries such as:
 *
 *   Market Data / Parser Thread
 *              |
 *              v
 *       SPSC Ring Buffer
 *              |
 *              v
 *      Matching Engine Thread
 *
 * Backpressure behavior, queue metrics, latency percentiles, and monitoring
 * hooks are evaluated in later stages of P17.
 */


#include <benchmark/benchmark.h>

#include "common/spsc_ring_buffer.hpp"
#include "orderbook/software/order.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <immintrin.h>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>



namespace
{

template<typename T, std::size_t Capacity>
class BoundedMutexQueue
{
    static_assert(
        Capacity > 0,
        "Capacity must be greater than zero");

public:

    void push(const T& value)
    {
        std::unique_lock<std::mutex> lock(
            mutex_);

        notFull_.wait(
            lock,
            [this]
            {
                return size_ < Capacity;
            });

        buffer_[head_] = value;

        head_ = increment(head_);

        ++size_;

        lock.unlock();

        notEmpty_.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(
            mutex_);

        notEmpty_.wait(
            lock,
            [this]
            {
                return size_ > 0;
            });

        T value = std::move(
            buffer_[tail_]);

        tail_ = increment(tail_);

        --size_;

        lock.unlock();

        notFull_.notify_one();

        return value;
    }

private:

    std::size_t increment(
        std::size_t index) const noexcept
    {
        return (index + 1) % Capacity;
    }

private:

    std::array<T, Capacity> buffer_{};

    std::size_t head_{0};

    std::size_t tail_{0};

    std::size_t size_{0};

    std::mutex mutex_;

    std::condition_variable notEmpty_;

    std::condition_variable notFull_;
};

struct MutexQueue
{
    std::queue<Order> queue;

    std::mutex mutex;

    std::condition_variable condition;
};

constexpr std::size_t MessageCount = 1'000'000;
constexpr std::size_t QueueCapacity = 4096;
using LockFreeQueue =
    SPSCRingBuffer<Order, QueueCapacity>;

using BoundedQueue =
    BoundedMutexQueue<Order, QueueCapacity - 1>;



using LockFreeQueue =
    SPSCRingBuffer<Order, QueueCapacity>;

Order makeOrder(std::uint64_t id)
{
    return Order{
        .id = id,
        .account_id = 100,
        .side = Side::Buy,
        .price = 1000,
        .quantity = 100,
        .level = nullptr,
        .prev = nullptr,
        .next = nullptr
    };
}

/*
 * Executes the mutex-based producer-consumer benchmark.
 *
 * Design
 * ------
 * A single producer thread generates Order objects and inserts them
 * into a shared std::queue protected by std::mutex.
 *
 * A single consumer thread removes orders from the queue after being
 * notified through std::condition_variable.
 *
 * Synchronization
 * ---------------
 * - std::mutex protects the shared queue.
 * - std::condition_variable blocks the consumer while the queue is empty.
 * - producerFinished signals that no more orders will be produced.
 *
 * Purpose
 * -------
 * This implementation serves as the baseline for comparison against
 * the lock-free SPSC ring buffer implementation.
 *
 * The benchmark measures the synchronization overhead introduced by
 * mutex locking and condition variable wake-ups during message passing.
 *
 * Note
 * ----
 * This benchmark uses an unbounded std::queue. A bounded mutex queue
 * with the same capacity as the SPSC ring buffer may be added later
 * for a fairer comparison.
 */

void runMutexQueue()
{
    MutexQueue queue;

    std::atomic<bool> producerFinished{false};

    std::thread producer(
        [&]()
        {
            for (std::size_t i = 0;
                 i < MessageCount;
                 ++i)
            {
                Order order =
                    makeOrder(i);

                {
                    std::lock_guard<std::mutex> lock(
                        queue.mutex);

                    queue.queue.push(order);
                }

                queue.condition.notify_one();
            }

            producerFinished.store(
                true,
                std::memory_order_release);

            queue.condition.notify_one();
        });

    std::thread consumer(
        [&]()
        {
            std::size_t consumed = 0;

            while (consumed < MessageCount)
            {
                std::unique_lock<std::mutex> lock(
                    queue.mutex);

                queue.condition.wait(
                    lock,
                    [&]()
                    {
                        return !queue.queue.empty() ||
                               producerFinished.load(
                                   std::memory_order_acquire);
                    });

                while (!queue.queue.empty())
                {
                    Order order =
                        queue.queue.front();

                    queue.queue.pop();

                    lock.unlock();

                    benchmark::DoNotOptimize(order.id);

                    ++consumed;

                    lock.lock();
                }
            }
        });

    producer.join();
    consumer.join();
}


/*
 * Executes the bounded mutex-based producer-consumer benchmark.
 *
 * Design
 * ------
 * A single producer thread publishes Order objects into a fixed-capacity
 * ring buffer protected by a mutex.
 *
 * A single consumer thread removes orders in FIFO order.
 *
 * Synchronization
 * ---------------
 * - std::mutex protects the shared ring-buffer state.
 * - notEmpty_ blocks the consumer while the queue is empty.
 * - notFull_ blocks the producer while the queue is full.
 *
 * Backpressure
 * ------------
 * The queue has the same usable capacity as the lock-free SPSC queue.
 * When full, the producer blocks until the consumer releases capacity.
 *
 * Purpose
 * -------
 * This benchmark provides a fairer baseline than the unbounded std::queue
 * implementation because both bounded implementations use:
 *
 * - Fixed-capacity storage
 * - No dynamic allocation during message transfer
 * - Identical Order payloads
 * - One producer and one consumer
 *
 * The primary remaining difference is the synchronization mechanism:
 * mutex and condition variables versus acquire/release atomics.
 */
void runBoundedMutexQueue()
{
    BoundedQueue queue;

    std::thread producer(
        [&]()
        {
            for (std::size_t i = 0;
                 i < MessageCount;
                 ++i)
            {
                queue.push(
                    makeOrder(i));
            }
        });

    std::thread consumer(
        [&]()
        {
            for (std::size_t consumed = 0;
                 consumed < MessageCount;
                 ++consumed)
            {
                Order order =
                    queue.pop();

                benchmark::DoNotOptimize(
                    order.id);
            }
        });

    producer.join();
    consumer.join();
}


/*
 * Executes the lock-free SPSC producer-consumer benchmark.
 *
 * Design
 * ------
 * A single producer thread creates Order objects and publishes them into
 * a fixed-capacity SPSCRingBuffer.
 *
 * A single consumer thread removes the orders in FIFO order.
 *
 * Synchronization
 * ---------------
 * - No mutex or condition variable is used.
 * - Queue visibility is coordinated through acquire/release atomics.
 * - When the queue is full or empty, the active thread applies a short
 *   processor pause before retrying.
 *
 * Backpressure
 * ------------
 * If the consumer cannot keep up and the queue becomes full, tryPush()
 * returns false. The producer then busy-spins with _mm_pause() until
 * capacity becomes available.
 *
 * This provides deterministic bounded backpressure without dynamic
 * allocation or kernel-assisted blocking.
 *
 * Purpose
 * -------
 * This workload is compared with the mutex-protected std::queue baseline
 * to measure the benefit of allocation-free SPSC message transfer.
 */
void runLockFreeQueue()
{
    LockFreeQueue queue;

    std::thread producer(
        [&]()
        {
            for (std::size_t i = 0;
                 i < MessageCount;
                 ++i)
            {
                const Order order =
                    makeOrder(i);

                while (!queue.tryPush(order))
                {
                    _mm_pause();
                }
            }
        });

    std::thread consumer(
        [&]()
        {
            std::size_t consumed = 0;
            Order order{};

            while (consumed < MessageCount)
            {
                if (queue.tryPop(order))
                {
                    benchmark::DoNotOptimize(order.id);
                    ++consumed;
                }
                else
                {
                    _mm_pause();
                }
            }
        });

    producer.join();
    consumer.join();
}

void BM_MutexQueue(benchmark::State& state)
{
    for (auto _ : state)
    {
        runMutexQueue();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

void BM_BoundedMutexQueue(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        runBoundedMutexQueue();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

void BM_LockFreeQueue(benchmark::State& state)
{
    for (auto _ : state)
    {
        runLockFreeQueue();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

}

BENCHMARK(BM_MutexQueue)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();


BENCHMARK(BM_BoundedMutexQueue)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();


BENCHMARK(BM_LockFreeQueue)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();





