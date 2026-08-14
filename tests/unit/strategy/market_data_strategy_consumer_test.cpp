#include <gtest/gtest.h>

#include "strategy/market_data_strategy_consumer.hpp"
#include "strategy/simple_threshold_strategy.hpp"

namespace
{

class RecordingOrderIntentSink final : public OrderIntentSink
{
public:
    bool submit(
        const OrderIntent& intent) override
    {
        lastIntent = intent;
        submitted = true;
        return true;
    }

    OrderIntent lastIntent{};
    bool submitted{false};
};

} // namespace

TEST(
    MarketDataStrategyConsumerTest,
    UpdatesBookAndForwardsGeneratedIntent)
{
    ArrayOrderBook book;

    MarketDataBookConsumer bookConsumer(book);

    SimpleThresholdStrategy strategy(
        1001,
        100,
        10);

    StrategyEngine strategyEngine(strategy);

    RecordingOrderIntentSink intentSink;

    MarketDataStrategyConsumer consumer(
        bookConsumer,
        strategyEngine,
        intentSink);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 5001,
        .accountId = 0,
        .side = Side::Sell,
        .price = 99,
        .quantity = 50
    };

    consumer.consume(event);

    // Verify market book was updated first.
    const PriceLevel* bestAsk =
        book.bestAsk();

    ASSERT_NE(bestAsk, nullptr);
    ASSERT_NE(bestAsk->head, nullptr);

    EXPECT_EQ(bestAsk->head->id, 5001U);
    EXPECT_EQ(bestAsk->head->price, 99);
    EXPECT_EQ(bestAsk->head->quantity, 50U);

    // Verify strategy generated and forwarded an intent.
    ASSERT_TRUE(intentSink.submitted);

    EXPECT_EQ(intentSink.lastIntent.accountId, 1001U);
    EXPECT_EQ(intentSink.lastIntent.side, Side::Buy);
    EXPECT_EQ(intentSink.lastIntent.price, 99);
    EXPECT_EQ(intentSink.lastIntent.quantity, 10U);
}
