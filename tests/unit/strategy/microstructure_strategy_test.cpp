/*
 * MicrostructureStrategy Tests
 *
 * Verifies trading decisions generated from Level-1 market
 * microstructure features.
 *
 * The tests cover strong bid imbalance producing a buy signal,
 * strong ask imbalance producing a sell signal, and balanced
 * market conditions producing no trading intent.
 */



#include <gtest/gtest.h>

#include "orderbook/software/array_order_book.hpp"
#include "strategy/microstructure_strategy.hpp"


TEST(
    MicrostructureStrategyTest,
    GeneratesBuyIntentForStrongBidImbalance)
{
    constexpr AccountId Account = 1001;
    constexpr Quantity OrderQuantity = 10;

    ArrayOrderBook marketBook;

    MicrostructureStrategy strategy(
        marketBook,
        Account,
        OrderQuantity,
        0.70,
        0.30);

    Order bidOrder{};
    bidOrder.id = 1;
    bidOrder.side = Side::Buy;
    bidOrder.price = 100;
    bidOrder.quantity = 900;

    bidOrder.level = nullptr;
    bidOrder.prev = nullptr;
    bidOrder.next = nullptr;

    Order askOrder{};
    askOrder.id = 2;
    askOrder.side = Side::Sell;
    askOrder.price = 101;
    askOrder.quantity = 300;

    askOrder.level = nullptr;
    askOrder.prev = nullptr;
    askOrder.next = nullptr;

    marketBook.addOrder(&bidOrder);
    marketBook.addOrder(&askOrder);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 2,
        .side = Side::Sell,
        .symbol = {
            'A', 'A', 'P', 'L',
            ' ', ' ', ' ', ' '
        },
        .price = 101,
        .quantity = 300
    };

    /*
     * Bid-side imbalance is 0.75 and the microprice
     * is above the midpoint, so the strategy should
     * generate a buy signal.
     */
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
        101);

    EXPECT_EQ(
        intent->quantity,
        OrderQuantity);
}


TEST(
    MicrostructureStrategyTest,
    GeneratesSellIntentForStrongAskImbalance)
{
    ArrayOrderBook marketBook;

    MicrostructureStrategy strategy(
        marketBook,
        1001,
        10,
        0.70,
        0.30);

    Order bidOrder{};
    bidOrder.id = 1;
    bidOrder.side = Side::Buy;
    bidOrder.price = 100;
    bidOrder.quantity = 300;

    bidOrder.level = nullptr;
    bidOrder.prev = nullptr;
    bidOrder.next = nullptr;

    Order askOrder{};
    askOrder.id = 2;
    askOrder.side = Side::Sell;
    askOrder.price = 101;
    askOrder.quantity = 900;

    askOrder.level = nullptr;
    askOrder.prev = nullptr;
    askOrder.next = nullptr;

    marketBook.addOrder(&bidOrder);
    marketBook.addOrder(&askOrder);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 2,
        .side = Side::Sell,
        .symbol = {
            'A', 'A', 'P', 'L',
            ' ', ' ', ' ', ' '
        },
        .price = 101,
        .quantity = 900
    };

    /*
     * Bid-side imbalance is 0.25 and the microprice
     * is below the midpoint, so the strategy should
     * generate a sell signal.
     */
    const auto intent =
        strategy.onMarketData(event);

    ASSERT_TRUE(intent.has_value());

    EXPECT_EQ(
        intent->side,
        Side::Sell);

    EXPECT_EQ(
        intent->price,
        100);
}


TEST(
    MicrostructureStrategyTest,
    ProducesNoIntentForBalancedBook)
{
    ArrayOrderBook marketBook;

    MicrostructureStrategy strategy(
        marketBook,
        1001,
        10,
        0.70,
        0.30);

    Order bidOrder{};
    bidOrder.id = 1;
    bidOrder.side = Side::Buy;
    bidOrder.price = 100;
    bidOrder.quantity = 500;

    bidOrder.level = nullptr;
    bidOrder.prev = nullptr;
    bidOrder.next = nullptr;

    Order askOrder{};
    askOrder.id = 2;
    askOrder.side = Side::Sell;
    askOrder.price = 101;
    askOrder.quantity = 500;

    askOrder.level = nullptr;
    askOrder.prev = nullptr;
    askOrder.next = nullptr;

    marketBook.addOrder(&bidOrder);
    marketBook.addOrder(&askOrder);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 2,
        .side = Side::Sell,
        .price = 101,
        .quantity = 500
    };

    const auto intent =
        strategy.onMarketData(event);

    EXPECT_FALSE(
        intent.has_value());
}