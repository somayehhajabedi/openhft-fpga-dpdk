
/*
 * MarketFeatureExtractor Tests
 *
 * Verifies the calculation of Level-1 market microstructure features
 * derived from the local order book.
 *
 * The tests cover best bid/ask prices and quantities, spread,
 * midpoint, order-book imbalance, microprice, and invalid
 * one-sided market states.
 */


#include <gtest/gtest.h>

#include "orderbook/software/array_order_book.hpp"
#include "strategy/market_feature_extractor.hpp"


TEST(
    MarketFeatureExtractorTest,
    ExtractsLevelOneFeatures)
{
    ArrayOrderBook marketBook;

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

    /*
     * Expected Level-1 features:
     *
     * spread:
     *     101 - 100 = 1
     *
     * midPrice:
     *     (100 + 101) / 2 = 100.5
     *
     * imbalance:
     *     900 / (900 + 300) = 0.75
     *
     * microPrice:
     *     (101 * 900 + 100 * 300)
     *     -------------------------
     *              1200
     *
     *     = 100.75
     */
    const auto features =
        MarketFeatureExtractor::extract(
            marketBook);

    ASSERT_TRUE(features.has_value());

    EXPECT_EQ(
        features->bestBidPrice,
        100);

    EXPECT_EQ(
        features->bestAskPrice,
        101);

    EXPECT_EQ(
        features->bestBidQuantity,
        900);

    EXPECT_EQ(
        features->bestAskQuantity,
        300);

    EXPECT_DOUBLE_EQ(
        features->spread,
        1.0);

    EXPECT_DOUBLE_EQ(
        features->midPrice,
        100.5);

    EXPECT_DOUBLE_EQ(
        features->imbalance,
        0.75);

    EXPECT_DOUBLE_EQ(
        features->microPrice,
        100.75);
}


TEST(
    MarketFeatureExtractorTest,
    ReturnsNoFeaturesForOneSidedMarket)
{
    ArrayOrderBook marketBook;

    Order bidOrder{};
    bidOrder.id = 1;
    bidOrder.side = Side::Buy;
    bidOrder.price = 100;
    bidOrder.quantity = 500;

    bidOrder.level = nullptr;
    bidOrder.prev = nullptr;
    bidOrder.next = nullptr;

    marketBook.addOrder(&bidOrder);

    const auto features =
        MarketFeatureExtractor::extract(
            marketBook);

    EXPECT_FALSE(
        features.has_value());
}