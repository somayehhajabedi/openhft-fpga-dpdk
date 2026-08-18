#include <gtest/gtest.h>

#include "orderbook/software/array_order_book.hpp"
#include "strategy/simple_threshold_strategy.hpp"


TEST(
    SimpleThresholdStrategyTest,
    GeneratesBuyIntentForSellAtOrBelowThreshold)
{
    constexpr AccountId Account = 1001;
    constexpr Price Threshold = 100;
    constexpr Quantity QuantityToBuy = 10;

    ArrayOrderBook marketBook;

    SimpleThresholdStrategy strategy(
        marketBook,
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

    // Populate the market-data order book.
    // The strategy now reads bestAsk() from MarketView.
    Order order{};

    order.id = event.orderId;
    order.account_id = event.accountId;
    order.side = event.side;
    order.price = event.price;
    order.quantity = event.quantity;

    order.level = nullptr;
    order.prev = nullptr;
    order.next = nullptr;

    marketBook.addOrder(&order);

    const auto intent =
        strategy.onMarketData(event);

    ASSERT_TRUE(intent.has_value());

    EXPECT_EQ(
        intent->accountId,
        Account);

    EXPECT_EQ(
        intent->side,
        Side::Buy);

    EXPECT_EQ(
        intent->symbol,
        event.symbol);

    EXPECT_EQ(
        intent->price,
        99);

    EXPECT_EQ(
        intent->quantity,
        QuantityToBuy);
}


TEST(
    SimpleThresholdStrategyTest,
    IgnoresSellAboveThreshold)
{
    ArrayOrderBook marketBook;

    SimpleThresholdStrategy strategy(
        marketBook,
        1,
        100,
        10);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .side = Side::Sell,
        .price = 101,
        .quantity = 50
    };

    Order order{};

    order.id = event.orderId;
    order.side = event.side;
    order.price = event.price;
    order.quantity = event.quantity;

    order.level = nullptr;
    order.prev = nullptr;
    order.next = nullptr;

    marketBook.addOrder(&order);

    EXPECT_FALSE(
        strategy.onMarketData(event).has_value());
}


TEST(
    SimpleThresholdStrategyTest,
    IgnoresBuyOrders)
{
    ArrayOrderBook marketBook;

    SimpleThresholdStrategy strategy(
        marketBook,
        1001,
        100,
        10);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .side = Side::Buy,
        .price = 99,
        .quantity = 50
    };

    Order order{};

    order.id = event.orderId;
    order.side = event.side;
    order.price = event.price;
    order.quantity = event.quantity;

    order.level = nullptr;
    order.prev = nullptr;
    order.next = nullptr;

    marketBook.addOrder(&order);

    EXPECT_FALSE(
        strategy.onMarketData(event).has_value());
}