#include <gtest/gtest.h>

#include "pipeline/market_data_book_consumer.hpp"

TEST(
    MarketDataBookConsumerTest,
    AddsOrderToBook)
{
    ArrayOrderBook book;

    MarketDataBookConsumer consumer(book);

    const MarketDataEvent event{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    consumer.consume(event);

    const PriceLevel* bestBid =
        book.bestBid();

    ASSERT_NE(bestBid, nullptr);

    EXPECT_EQ(
        bestBid->price,
        100);

    ASSERT_NE(
        bestBid->head,
        nullptr);

    EXPECT_EQ(
        bestBid->head->id,
        1U);

    EXPECT_EQ(
        bestBid->head->quantity,
        50U);
}

TEST(
    MarketDataBookConsumerTest,
    CancelReducesOrderQuantity)
{
    ArrayOrderBook book;
    MarketDataBookConsumer consumer(book);

    const MarketDataEvent add{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    consumer.consume(add);

    const MarketDataEvent cancel{
        .type = MarketDataEventType::CancelOrder,
        .orderId = 1,
        .quantity = 20
    };

    consumer.consume(cancel);

    const PriceLevel* bestBid =
        book.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(
        bestBid->head->id,
        1U);

    EXPECT_EQ(
        bestBid->head->quantity,
        30U);
}


TEST(
    MarketDataBookConsumerTest,
    DeleteRemovesOrder)
{
    ArrayOrderBook book;
    MarketDataBookConsumer consumer(book);

    const MarketDataEvent add{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    consumer.consume(add);

    ASSERT_NE(
        book.bestBid(),
        nullptr);

    const MarketDataEvent remove{
        .type = MarketDataEventType::DeleteOrder,
        .orderId = 1
    };

    consumer.consume(remove);

    EXPECT_EQ(
        book.bestBid(),
        nullptr);
}


TEST(
    MarketDataBookConsumerTest,
    ExecuteReducesOrderQuantity)
{
    ArrayOrderBook book;
    MarketDataBookConsumer consumer(book);

    const MarketDataEvent add{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    consumer.consume(add);

    const MarketDataEvent execution{
        .type = MarketDataEventType::ExecuteOrder,
        .orderId = 1,
        .quantity = 15
    };

    consumer.consume(execution);

    const PriceLevel* bestBid =
        book.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(
        bestBid->head->id,
        1U);

    EXPECT_EQ(
        bestBid->head->quantity,
        35U);
}


TEST(
    MarketDataBookConsumerTest,
    ReplaceUpdatesIdPriceAndQuantity)
{
    ArrayOrderBook book;
    MarketDataBookConsumer consumer(book);

    const MarketDataEvent add{
        .type = MarketDataEventType::AddOrder,
        .orderId = 1,
        .accountId = 1001,
        .side = Side::Buy,
        .price = 100,
        .quantity = 50
    };

    consumer.consume(add);

    const MarketDataEvent replace{
        .type = MarketDataEventType::ReplaceOrder,
        .orderId = 1,
        .newOrderId = 2,
        .price = 110,
        .quantity = 75
    };

    consumer.consume(replace);

    const PriceLevel* bestBid =
        book.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(
        bestBid->price,
        110);

    EXPECT_EQ(
        bestBid->head->id,
        2U);

    EXPECT_EQ(
        bestBid->head->quantity,
        75U);

    EXPECT_EQ(
        bestBid->head->account_id,
        1001U);

    EXPECT_EQ(
        bestBid->head->side,
        Side::Buy);
}

