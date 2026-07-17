#include <gtest/gtest.h>

#include <endian.h>
#include <arpa/inet.h>
#include <cstdint>

#include "gateway/market_data/itch_handler.hpp"
#include "orderbook/software/array_order_book.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_delete.hpp"

TEST(ITCHHandlerDeleteTest, RemovesOrderFromOrderBook)
{
    ArrayOrderBook orderBook;
    ITCHHandler handler(orderBook);

    constexpr std::uint64_t orderId = 2001;
    constexpr std::uint32_t quantity = 1000;
    constexpr std::uint32_t price = 12500;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(orderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(quantity);
    addMessage.price = htonl(price);

    ASSERT_TRUE(
        handler.onAddOrder(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    ASSERT_NE(orderBook.bestBid(), nullptr);

    OrderDeleteWireMessage deleteMessage{};

    deleteMessage.message_type = 'D';
    deleteMessage.order_reference_number = htobe64(orderId);

    ASSERT_TRUE(
        handler.onOrderDelete(
            reinterpret_cast<const std::uint8_t*>(&deleteMessage),
            sizeof(deleteMessage)));

    EXPECT_EQ(orderBook.bestBid(), nullptr);
}