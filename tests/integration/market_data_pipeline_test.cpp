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

#include <cstddef>

namespace
{

class TestConsumer final : public EventConsumer
{
public:
    void consume(
        const MarketDataEvent& event) override
    {
        lastEvent = event;
        ++consumedCount;
    }

    MarketDataEvent lastEvent{};

    std::size_t consumedCount{0};
};

} // namespace

TEST(
    MarketDataPipelineTest,
    SubmitsAndDispatchesEvent)
{
    TestConsumer consumer;

    MarketDataPipeline pipeline(
        consumer);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder
    };

    ASSERT_TRUE(
        pipeline.submit(event));

    pipeline.start();

    EXPECT_EQ(
        consumer.consumedCount,
        1U);

    EXPECT_EQ(
        consumer.lastEvent.type,
        MarketDataEventType::AddOrder);
}