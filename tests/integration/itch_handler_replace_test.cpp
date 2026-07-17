#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <endian.h>
#include <cstdint>

#include "gateway/market_data/itch_handler.hpp"
#include "orderbook/software/array_order_book.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_replace.hpp"

TEST(ITCHHandlerReplaceTest, ReplacesOrderWithNewIdPriceAndQuantity)
{
    ArrayOrderBook orderBook;
    ITCHHandler handler(orderBook);

    constexpr std::uint64_t originalOrderId = 4001;
    constexpr std::uint64_t newOrderId = 4002;

    constexpr std::uint32_t originalQuantity = 1000;
    constexpr std::uint32_t newQuantity = 800;

    constexpr std::uint32_t originalPrice = 12500;
    constexpr std::uint32_t newPrice = 12750;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number =
        htobe64(originalOrderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares =
        htonl(originalQuantity);
    addMessage.price =
        htonl(originalPrice);

    ASSERT_TRUE(
        handler.onAddOrder(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    const PriceLevel* levelBefore =
        orderBook.bestBid();

    ASSERT_NE(levelBefore, nullptr);
    ASSERT_NE(levelBefore->head, nullptr);

    EXPECT_EQ(
        levelBefore->head->id,
        originalOrderId);

    EXPECT_EQ(
        levelBefore->head->price,
        originalPrice);

    EXPECT_EQ(
        levelBefore->head->quantity,
        originalQuantity);

    OrderReplaceWireMessage replaceMessage{};

    replaceMessage.message_type = 'U';
    replaceMessage.original_order_reference =
        htobe64(originalOrderId);
    replaceMessage.new_order_reference =
        htobe64(newOrderId);
    replaceMessage.shares =
        htonl(newQuantity);
    replaceMessage.price =
        htonl(newPrice);

    ASSERT_TRUE(
        handler.onOrderReplace(
            reinterpret_cast<const std::uint8_t*>(&replaceMessage),
            sizeof(replaceMessage)));

    const PriceLevel* levelAfter =
        orderBook.bestBid();

    ASSERT_NE(levelAfter, nullptr);
    ASSERT_NE(levelAfter->head, nullptr);

    EXPECT_EQ(
        levelAfter->head->id,
        newOrderId);

    EXPECT_EQ(
        levelAfter->head->price,
        newPrice);

    EXPECT_EQ(
        levelAfter->head->quantity,
        newQuantity);
}