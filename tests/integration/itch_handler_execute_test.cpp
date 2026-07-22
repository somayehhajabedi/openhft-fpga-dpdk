#include <gtest/gtest.h>

#include <endian.h>
#include <arpa/inet.h>
#include <cstdint>

#include "gateway/market_data/itch_handler.hpp"
#include "orderbook/software/array_order_book.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_executed.hpp"

TEST(ITCHHandlerExecuteTest, ReducesOrderQuantity)
{
    ArrayOrderBook orderBook;
    ITCHHandler handler(orderBook);

    constexpr std::uint64_t orderId = 3001;
    constexpr std::uint32_t initialQuantity = 1000;
    constexpr std::uint32_t executedQuantity = 250;
    constexpr std::uint32_t expectedQuantity = 750;
    constexpr std::uint32_t price = 12500;
    constexpr std::uint64_t matchNumber = 90001;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(orderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(initialQuantity);
    addMessage.price = htonl(price);

    ASSERT_TRUE(
        handler.onAddOrder(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    const PriceLevel* levelBefore = orderBook.bestBid();

    ASSERT_NE(levelBefore, nullptr);
    ASSERT_NE(levelBefore->head, nullptr);
    EXPECT_EQ(levelBefore->head->quantity, initialQuantity);

    OrderExecutedWireMessage executedMessage{};

    executedMessage.message_type = 'E';
    executedMessage.order_reference_number = htobe64(orderId);
    executedMessage.executed_shares = htobe32(executedQuantity);
    executedMessage.match_number = htobe64(matchNumber);

    ASSERT_TRUE(
        handler.onOrderExecuted(
            reinterpret_cast<const std::uint8_t*>(&executedMessage),
            sizeof(executedMessage)));

    const PriceLevel* levelAfter = orderBook.bestBid();

    ASSERT_NE(levelAfter, nullptr);
    ASSERT_NE(levelAfter->head, nullptr);

    EXPECT_EQ(levelAfter->head->id, orderId);
    EXPECT_EQ(levelAfter->head->quantity, expectedQuantity);
}