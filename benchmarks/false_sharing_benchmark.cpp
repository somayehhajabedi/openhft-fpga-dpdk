
/*
 * False Sharing Benchmark
 *
 * Purpose
 * -------
 * This benchmark demonstrates the performance impact of false sharing
 * in multi-threaded applications.
 *
 * False sharing occurs when multiple threads modify independent variables
 * that reside on the same CPU cache line. Although the variables are
 * logically unrelated, the processor must repeatedly transfer ownership
 * of the shared cache line between cores, introducing unnecessary cache
 * coherency traffic and increasing execution time.
 *
 * Benchmark Design
 * ----------------
 * Two benchmark scenarios are compared:
 *
 *   1. BM_FalseSharing
 *
 *      Two threads increment two independent atomic counters stored inside
 *      the same cache line.
 *
 *      SharedCounters
 *      +-----------------------------------------------+
 *      | counter1 | counter2 |      same cache line     |
 *      +-----------------------------------------------+
 *
 *      Since both counters share the same cache line,
 *      every write performed by one thread invalidates
 *      the cache line used by the other thread.
 *
 *
 *   2. BM_PaddedCounters
 *
 *      Each atomic counter is aligned to its own cache line
 *      using alignas(64).
 *
 *      PaddedCounters
 *
 *      +-------------+                     +-------------+
 *      | counter1    |                     | counter2    |
 *      | Cache Line  |                     | Cache Line  |
 *      +-------------+                     +-------------+
 *
 *      Each thread updates a different cache line,
 *      avoiding unnecessary cache coherency traffic.
 *
 *
 * Measurement
 * -----------
 * Google Benchmark measures the execution time of both scenarios.
 *
 * Linux perf can be used to observe cache behavior:
 *
 *     perf stat \
 *         -e cache-references,cache-misses \
 *         ./false_sharing_benchmark
 *
 *
 * Expected Result
 * ---------------
 * The padded version should generally outperform the false-sharing version
 * because cache-line contention is eliminated.
 *
 * The exact improvement depends on:
 *
 *   - CPU architecture
 *   - Cache hierarchy
 *   - Number of cores
 *   - Hyper-Threading
 *   - Operating system scheduling
 *
 *
 * Practical Relevance
 * -------------------
 * False sharing is a common performance issue in low-latency systems,
 * trading engines, lock-free queues, and high-performance concurrent
 * applications.
 *
 * Eliminating false sharing is often a low-cost optimization that can
 * significantly improve scalability without changing application logic.
 */
 

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstddef>
#include <thread>
#include <cstdint>

namespace
{

constexpr std::size_t Iterations = 10'000'000;

struct SharedCounters
{
    std::atomic<std::uint64_t> first{0};
    std::atomic<std::uint64_t> second{0};
};

struct PaddedCounters
{
    alignas(64) std::atomic<std::uint64_t> first{0};
    alignas(64) std::atomic<std::uint64_t> second{0};
};

} // namespace

void runFalseSharingWorkload()
{
    SharedCounters counters;

    std::thread firstThread(
        [&counters]()
        {
            for (std::size_t i = 0; i < Iterations; ++i)
            {
                counters.first.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
        });

    std::thread secondThread(
        [&counters]()
        {
            for (std::size_t i = 0; i < Iterations; ++i)
            {
                counters.second.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
        });

    firstThread.join();
    secondThread.join();

    benchmark::DoNotOptimize(
        counters.first.load(std::memory_order_relaxed));

    benchmark::DoNotOptimize(
        counters.second.load(std::memory_order_relaxed));
}

void runPaddedWorkload()
{
    PaddedCounters counters;

    std::thread firstThread(
        [&counters]()
        {
            for (std::size_t i = 0; i < Iterations; ++i)
            {
                counters.first.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
        });

    std::thread secondThread(
        [&counters]()
        {
            for (std::size_t i = 0; i < Iterations; ++i)
            {
                counters.second.fetch_add(
                    1,
                    std::memory_order_relaxed);
            }
        });

    firstThread.join();
    secondThread.join();

    benchmark::DoNotOptimize(
        counters.first.load(std::memory_order_relaxed));

    benchmark::DoNotOptimize(
        counters.second.load(std::memory_order_relaxed));
}

void BM_FalseSharing(benchmark::State& state)
{
    for (auto _ : state)
    {
        runFalseSharingWorkload();
    }
}

void BM_PaddedCounters(benchmark::State& state)
{
    for (auto _ : state)
    {
        runPaddedWorkload();
    }
}



BENCHMARK(BM_FalseSharing)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_PaddedCounters)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();