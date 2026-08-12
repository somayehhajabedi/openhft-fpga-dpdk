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
