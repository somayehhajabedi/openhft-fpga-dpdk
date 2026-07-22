#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <endian.h>
#include <cstdint>

#include "gateway/market_data/itch_handler.hpp"
#include "orderbook/software/array_order_book.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"

TEST(ITCHHandlerCancelTest, ReducesOrderQuantity)
{
    ArrayOrderBook orderBook;
    ITCHHandler handler(orderBook);

    constexpr std::uint64_t orderId = 1001;
    constexpr std::uint32_t initialQuantity = 1000;
    constexpr std::uint32_t cancelledQuantity = 300;
    constexpr std::uint32_t expectedQuantity = 700;
    constexpr std::uint32_t price = 12500;

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

    OrderCancelWireMessage cancelMessage{};

    cancelMessage.message_type = 'X';
    cancelMessage.order_reference_number = htobe64(orderId);
    cancelMessage.cancelled_shares = htonl(cancelledQuantity);

    ASSERT_TRUE(
        handler.onOrderCancel(
            reinterpret_cast<const std::uint8_t*>(&cancelMessage),
            sizeof(cancelMessage)));

    const PriceLevel* levelAfter = orderBook.bestBid();

    ASSERT_NE(levelAfter, nullptr);
    ASSERT_NE(levelAfter->head, nullptr);

    EXPECT_EQ(levelAfter->head->id, orderId);
    EXPECT_EQ(levelAfter->head->quantity, expectedQuantity);
}