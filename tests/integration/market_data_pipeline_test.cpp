/*
 * Market Data Pipeline Integration Tests
 *
 * Purpose
 * -------
 * Verifies that MarketDataEvent objects can be submitted to the
 * MarketDataPipeline, transferred through the SPSC queue, consumed
 * by the Dispatcher, and delivered to an EventConsumer.
 */

#include <gtest/gtest.h>

#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/market_data_pipeline.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

namespace
{

class TestConsumer final : public EventConsumer
{
public:
    void consume(
        const MarketDataEvent& event) override
    {
        lastEventType.store(
            event.type,
            std::memory_order_relaxed);

        consumedCount.fetch_add(
            1,
            std::memory_order_release);
    }

    std::atomic<MarketDataEventType> lastEventType{
        MarketDataEventType::AddOrder};

    std::atomic<std::size_t> consumedCount{0};
};

} // namespace

TEST(
    MarketDataPipelineTest,
    SubmitsAndDispatchesEvent)
{
    using namespace std::chrono_literals;

    TestConsumer consumer;

    MarketDataPipeline pipeline(
        consumer);

    pipeline.start();

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder
    };

    ASSERT_TRUE(
        pipeline.submit(event));

    const auto deadline =
        std::chrono::steady_clock::now() + 100ms;

    while (
        consumer.consumedCount.load(
            std::memory_order_acquire) == 0 &&
        std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    pipeline.stop();

    EXPECT_EQ(
        consumer.consumedCount.load(
            std::memory_order_acquire),
        1U);

    EXPECT_EQ(
        consumer.lastEventType.load(
            std::memory_order_relaxed),
        MarketDataEventType::AddOrder);
}
