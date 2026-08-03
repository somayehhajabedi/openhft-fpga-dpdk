#include <gtest/gtest.h>

#include "orderbook/software/array_order_book.hpp"
#include "orderbook/software/order.hpp"

namespace
{

Order makeOrder(
    OrderId id,
    Side side,
    Price price,
    Quantity quantity = 100)
{
    return Order{
        .id = id,
        .account_id = 1,
        .side = side,
        .price = price,
        .quantity = quantity
    };
}

} // namespace

TEST(ArrayOrderBookTest, EmptyBookHasNoBestBidOrAsk)
{
    ArrayOrderBook book;

    EXPECT_EQ(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestAsk(), nullptr);
}


TEST(ArrayOrderBookTest, AddSingleBid)
{
    ArrayOrderBook book;

    Order order = makeOrder(1, Side::Buy, 100);

    book.addOrder(&order);

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price, 100);
}

TEST(ArrayOrderBookTest, AddSingleAsk)
{
    ArrayOrderBook book;

    Order order = makeOrder(1, Side::Sell, 100);

    book.addOrder(&order);

    ASSERT_NE(book.bestAsk(), nullptr);
    EXPECT_EQ(book.bestAsk()->price, 100);
}

TEST(ArrayOrderBookBitmapTest, FindsNextBestBidInSameBitmapWord)
{
    ArrayOrderBook book;

    Order lowerBid = makeOrder(1, Side::Buy, 100);
    Order bestBid = makeOrder(2, Side::Buy, 110);

    book.addOrder(&lowerBid);
    book.addOrder(&bestBid);

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price, 110);

    EXPECT_TRUE(book.cancelOrder(bestBid.id));

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price, 100);
}

TEST(ArrayOrderBookBitmapTest, FindsNextBestBidInPreviousBitmapWord)
{
    ArrayOrderBook book;

    Order lowerBid = makeOrder(1, Side::Buy, 63);
    Order bestBid = makeOrder(2, Side::Buy, 64);

    book.addOrder(&lowerBid);
    book.addOrder(&bestBid);

    EXPECT_TRUE(book.cancelOrder(bestBid.id));

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price, 63);
}

TEST(ArrayOrderBookBitmapTest, FindsNextBestAskInSameBitmapWord)
{
    ArrayOrderBook book;

    Order bestAsk = makeOrder(1, Side::Sell, 100);
    Order higherAsk = makeOrder(2, Side::Sell, 110);

    book.addOrder(&bestAsk);
    book.addOrder(&higherAsk);

    ASSERT_NE(book.bestAsk(), nullptr);
    EXPECT_EQ(book.bestAsk()->price, 100);

    EXPECT_TRUE(book.cancelOrder(bestAsk.id));

    ASSERT_NE(book.bestAsk(), nullptr);
    EXPECT_EQ(book.bestAsk()->price, 110);
}

TEST(ArrayOrderBookBitmapTest, FindsNextBestAskInNextBitmapWord)
{
    ArrayOrderBook book;

    Order bestAsk = makeOrder(1, Side::Sell, 63);
    Order higherAsk = makeOrder(2, Side::Sell, 64);

    book.addOrder(&bestAsk);
    book.addOrder(&higherAsk);

    EXPECT_TRUE(book.cancelOrder(bestAsk.id));

    ASSERT_NE(book.bestAsk(), nullptr);
    EXPECT_EQ(book.bestAsk()->price, 64);
}

TEST(ArrayOrderBookBitmapTest, KeepsLevelActiveUntilLastOrderIsCancelled)
{
    ArrayOrderBook book;

    Order firstBid = makeOrder(1, Side::Buy, 100);
    Order secondBid = makeOrder(2, Side::Buy, 100);

    book.addOrder(&firstBid);
    book.addOrder(&secondBid);

    EXPECT_TRUE(book.cancelOrder(firstBid.id));

    ASSERT_NE(book.bestBid(), nullptr);
    EXPECT_EQ(book.bestBid()->price, 100);

    EXPECT_TRUE(book.cancelOrder(secondBid.id));
    EXPECT_EQ(book.bestBid(), nullptr);
}




