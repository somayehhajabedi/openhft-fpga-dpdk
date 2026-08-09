/*
 * Market Data Pipeline Latency Benchmark
 *
 * Purpose
 * -------
 * Measures end-to-end latency from MarketDataPipeline::submit()
 * to EventConsumer::consume().
 *
 * The benchmark records per-event latency samples and later
 * calculates latency percentiles such as p50, p95, p99 and p99.9.
 * 
 * 100,000 events
     │
     ├── t0 before submit
     │
     ▼
MarketDataPipeline
     │
     ├── SPSC
     ├── worker thread
     ├── Dispatcher
     │
     ▼
EventConsumer
     │
     └── t1
          ↓
      t1 - t0
          ↓
   latenciesNs[]
          ↓
        sort
          ↓
┌─────────────────────┐
│ p50                  │
│ p95                  │
│ p99                  │
│ p99.9                │
│ max                  │
└─────────────────────┘
 */



#include <benchmark/benchmark.h>

#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <algorithm>

namespace
{

constexpr std::size_t MessageCount = 100'000;

using Clock = std::chrono::steady_clock;

std::array<Clock::time_point, MessageCount> submitTimes;
std::array<std::uint64_t, MessageCount> latenciesNs;

class LatencyConsumer final : public EventConsumer
{
public:
    void consume(
        const MarketDataEvent& event) override
    {
        const std::size_t index =
            static_cast<std::size_t>(
                event.orderId - 1);

        const auto endTime =
            Clock::now();

        const auto latency =
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                endTime - submitTimes[index]);

        latenciesNs[index] =
            static_cast<std::uint64_t>(
                latency.count());

        consumedCount_.fetch_add(
            1,
            std::memory_order_release);
    }

    [[nodiscard]]
    std::size_t consumedCount() const noexcept
    {
        return consumedCount_.load(
            std::memory_order_acquire);
    }

private:
    std::atomic<std::size_t> consumedCount_{0};
};

std::uint64_t percentile(double p)
{
    const std::size_t index =
        static_cast<std::size_t>(
            p * static_cast<double>(MessageCount - 1));

    return latenciesNs[index];
}

void runLatencyBenchmark()
{
    LatencyConsumer consumer;

    MarketDataPipeline pipeline(
        consumer);

    pipeline.start();

    for (std::size_t i = 0;
         i < MessageCount;
         ++i)
    {
        const MarketDataEvent event{
            .type = MarketDataEventType::AddOrder,
            .orderId =
                static_cast<OrderId>(i + 1)
        };

        while (true)
        {
            submitTimes[i] =
                Clock::now();

            if (pipeline.submit(event))
            {
                break;
            }

            // Queue full: retry until the event is accepted.
        }
    }

    while (
        consumer.consumedCount() <
        MessageCount)
    {
        std::this_thread::yield();
    }

    pipeline.stop();
}

void runUncontendedLatencyBenchmark()
{
    LatencyConsumer consumer;

    MarketDataPipeline pipeline(consumer);

    pipeline.start();

    for (std::size_t i = 0;
         i < MessageCount;
         ++i)
    {
        const MarketDataEvent event{
            .type = MarketDataEventType::AddOrder,
            .orderId =
                static_cast<OrderId>(i + 1)
        };

        submitTimes[i] = Clock::now();

        while (!pipeline.submit(event))
        {
            // Retry if queue is temporarily full.
        }

        // Do not submit the next event until this one
        // has reached the consumer.
        while (consumer.consumedCount() < i + 1)
        {
            // Busy-spin intentionally.
        }
    }

    pipeline.stop();
}


void BM_MarketDataPipelineLatency(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        runLatencyBenchmark();

        state.PauseTiming();

        std::sort(
            latenciesNs.begin(),
            latenciesNs.end());

        state.counters["p50_ns"] =
            static_cast<double>(
                percentile(0.50));

        state.counters["p95_ns"] =
            static_cast<double>(
                percentile(0.95));

        state.counters["p99_ns"] =
            static_cast<double>(
                percentile(0.99));

        state.counters["p999_ns"] =
            static_cast<double>(
                percentile(0.999));

        state.counters["max_ns"] =
            static_cast<double>(
                latenciesNs.back());

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

void BM_MarketDataPipelineUncontendedLatency(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        runUncontendedLatencyBenchmark();

        state.PauseTiming();

        std::sort(
            latenciesNs.begin(),
            latenciesNs.end());

        state.counters["p50_ns"] =
            static_cast<double>(percentile(0.50));

        state.counters["p95_ns"] =
            static_cast<double>(percentile(0.95));

        state.counters["p99_ns"] =
            static_cast<double>(percentile(0.99));

        state.counters["p999_ns"] =
            static_cast<double>(percentile(0.999));

        state.counters["max_ns"] =
            static_cast<double>(latenciesNs.back());

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

} // namespace

BENCHMARK(BM_MarketDataPipelineLatency)
    ->UseRealTime();

BENCHMARK(BM_MarketDataPipelineUncontendedLatency)
    ->UseRealTime();

BENCHMARK_MAIN();
