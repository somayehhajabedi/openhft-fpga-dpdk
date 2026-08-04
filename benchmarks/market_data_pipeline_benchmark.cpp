/*
 * Market Data Pipeline Benchmark
 * ==============================
 *
 * Purpose
 * -------
 * Measures end-to-end throughput of the asynchronous market-data
 * pipeline, including:
 *
 * - MarketDataPipeline::submit()
 * - Lock-free SPSC queue transport
 * - Worker-thread busy polling
 * - Dispatcher processing
 * - EventConsumer invocation
 *
 * The benchmark uses one producer thread and the pipeline's dedicated
 * consumer thread.
 */

#include <benchmark/benchmark.h>

#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>

namespace
{

constexpr std::size_t MessageCount = 1'000'000;

class BenchmarkConsumer final : public EventConsumer
{
public:
    void consume(
        const MarketDataEvent& event) override
    {
        benchmark::DoNotOptimize(event.type);

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

void runMarketDataPipeline()
{
    BenchmarkConsumer consumer;

    MarketDataPipeline pipeline(
        consumer);

    pipeline.start();

    for (std::size_t i = 0;
         i < MessageCount;
         ++i)
    {
        const MarketDataEvent event{
            .type = MarketDataEventType::AddOrder
        };

        while (!pipeline.submit(event))
        {
            // Queue is full. The producer busy-spins until
            // the consumer releases capacity.
        }
    }

    while (consumer.consumedCount() < MessageCount)
    {
        // Wait until all submitted events are consumed.
        std::this_thread::yield();
    }

    pipeline.stop();
}

void BM_MarketDataPipeline(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        runMarketDataPipeline();
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations() * MessageCount));
}

} // namespace

BENCHMARK(BM_MarketDataPipeline)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();