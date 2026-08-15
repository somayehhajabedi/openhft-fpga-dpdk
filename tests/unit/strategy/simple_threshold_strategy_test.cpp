#include <gtest/gtest.h>

#include "strategy/simple_threshold_strategy.hpp"

TEST(
    SimpleThresholdStrategyTest,
    GeneratesBuyIntentForSellAtOrBelowThreshold)
{
    constexpr AccountId Account = 1001;
    constexpr Price Threshold = 100;
    constexpr Quantity QuantityToBuy = 10;

    SimpleThresholdStrategy strategy(
        Account,
        Threshold,
        QuantityToBuy);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .newOrderId = 0,
        .accountId = 0,
        .side = Side::Sell,
        .symbol = {
             'A', 'A', 'P', 'L',
             ' ', ' ', ' ', ' '
        },
        .price = 99,
        .quantity = 50
    };

    const auto intent =
        strategy.onMarketData(event);

    ASSERT_TRUE(intent.has_value());

    EXPECT_EQ(intent->accountId, Account);
    EXPECT_EQ(intent->side, Side::Buy);
    EXPECT_EQ(intent->symbol, event.symbol);
    EXPECT_EQ(intent->price, 99);
    EXPECT_EQ(intent->quantity, QuantityToBuy);
}

TEST(
    SimpleThresholdStrategyTest,
    IgnoresSellAboveThreshold)
{
    SimpleThresholdStrategy strategy(
        1001,
        100,
        10);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .side = Side::Sell,
        .price = 101,
        .quantity = 50
    };

    EXPECT_FALSE(
        strategy.onMarketData(event).has_value());
}

TEST(
    SimpleThresholdStrategyTest,
    IgnoresBuyOrders)
{
    SimpleThresholdStrategy strategy(
        1001,
        100,
        10);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .side = Side::Buy,
        .price = 99,
        .quantity = 50
    };

    EXPECT_FALSE(
        strategy.onMarketData(event).has_value());
}
