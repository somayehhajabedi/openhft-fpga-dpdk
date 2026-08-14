#include <gtest/gtest.h>

#include "strategy/strategy_engine.hpp"

namespace
{

class TestStrategy final : public IStrategy
{
public:
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) override
    {
        if (event.price != 42)
        {
            return std::nullopt;
        }

        return OrderIntent{
            .accountId = 1001,
            .side = Side::Buy,
            .price = event.price,
            .quantity = 10
        };
    }
};

} // namespace

TEST(
    StrategyEngineTest,
    ForwardsMarketDataToStrategy)
{
    TestStrategy strategy;

    StrategyEngine engine(strategy);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .side = Side::Sell,
        .price = 42,
        .quantity = 100
    };

    const auto intent =
        engine.onMarketData(event);

    ASSERT_TRUE(intent.has_value());

    EXPECT_EQ(intent->accountId, 1001U);
    EXPECT_EQ(intent->side, Side::Buy);
    EXPECT_EQ(intent->price, 42);
    EXPECT_EQ(intent->quantity, 10U);
}

TEST(
    StrategyEngineTest,
    ReturnsNoIntentWhenStrategyProducesNone)
{
    TestStrategy strategy;

    StrategyEngine engine(strategy);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .side = Side::Sell,
        .price = 50,
        .quantity = 100
    };

    EXPECT_FALSE(
        engine.onMarketData(event).has_value());
}
