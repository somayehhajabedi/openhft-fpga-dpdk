/*
 * MicrostructureStrategy Tests
 *
 * Verifies that model-derived directional scores are converted
 * into trading decisions by MicrostructureStrategy.
 *
 * The tests cover bullish conditions producing a buy intent,
 * bearish conditions producing a sell intent, and balanced
 * conditions producing no trading intent.
 */

#include <gtest/gtest.h>

#include "orderbook/software/array_order_book.hpp"
#include "strategy/linear_signal_model.hpp"
#include "strategy/microstructure_strategy.hpp"


TEST(
    MicrostructureStrategyTest,
    GeneratesBuyIntentForPositiveModelScore)
{
    constexpr AccountId Account = 1001;
    constexpr Quantity OrderQuantity = 10;

    ArrayOrderBook marketBook;

    const LinearSignalModel signalModel(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MicrostructureStrategy strategy(
        marketBook,
        signalModel,
        Account,
        OrderQuantity,
        0.20,
        -0.20);

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
     * Features:
     *
     * imbalance = 0.75
     * imbalanceSignal = 0.5
     *
     * microPrice = 100.75
     * midPrice = 100.5
     * microPriceSignal = 0.25
     *
     * score:
     *
     * 0.6 * 0.5 + 0.4 * 0.25 = 0.4
     *
     * 0.4 >= buy threshold 0.20
     * therefore a Buy intent is expected.
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
    GeneratesSellIntentForNegativeModelScore)
{
    ArrayOrderBook marketBook;

    const LinearSignalModel signalModel(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MicrostructureStrategy strategy(
        marketBook,
        signalModel,
        1001,
        10,
        0.20,
        -0.20);

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
     * Features:
     *
     * imbalance = 0.25
     * imbalanceSignal = -0.5
     *
     * microPrice = 100.25
     * midPrice = 100.5
     * microPriceSignal = -0.25
     *
     * score:
     *
     * 0.6 * -0.5 + 0.4 * -0.25 = -0.4
     *
     * -0.4 <= sell threshold -0.20
     * therefore a Sell intent is expected.
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

    const LinearSignalModel signalModel(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MicrostructureStrategy strategy(
        marketBook,
        signalModel,
        1001,
        10,
        0.20,
        -0.20);

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

    /*
     * Balanced quantities produce:
     *
     * imbalanceSignal = 0
     * microPriceSignal = 0
     * score = 0
     *
     * The score remains inside the no-trade region.
     */
    const auto intent =
        strategy.onMarketData(event);

    EXPECT_FALSE(
        intent.has_value());
}