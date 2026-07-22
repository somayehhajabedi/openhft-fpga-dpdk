
#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <endian.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"
#include "dpdk/parser/itch/messages/order_delete.hpp"
#include "dpdk/parser/itch/messages/order_executed.hpp"
#include "dpdk/parser/itch/messages/order_replace.hpp"

#include "gateway/market_data/itch_handler.hpp"
#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "market_data/replay/itch_replay_reader.hpp"
#include "orderbook/software/array_order_book.hpp"

namespace
{

template <typename Message>
void writeMessage(
    std::ofstream& stream,
    const Message& message)
{
    const auto messageLength =
        static_cast<std::uint16_t>(sizeof(Message));

    const std::uint16_t networkLength =
        htons(messageLength);

    stream.write(
        reinterpret_cast<const char*>(&networkLength),
        sizeof(networkLength));

    stream.write(
        reinterpret_cast<const char*>(&message),
        sizeof(message));
}

} // namespace

TEST(ItchReplayPipelineTest, ReplaysMessagesIntoOrderBook)
{
    const std::string path =
        "itch_replay_pipeline_test.bin";

    constexpr std::uint64_t originalOrderId = 7001;
    constexpr std::uint64_t replacementOrderId = 8001;

    constexpr std::uint32_t initialQuantity = 1000;
    constexpr std::uint32_t cancelledQuantity = 200;
    constexpr std::uint32_t executedQuantity = 300;

    constexpr std::uint32_t originalPrice = 12500;
    constexpr std::uint32_t replacementQuantity = 400;
    constexpr std::uint32_t replacementPrice = 12600;

    constexpr std::uint64_t matchNumber = 99001;

    AddOrderWireMessage addMessage{};
    addMessage.message_type = 'A';
    addMessage.order_reference_number =
        htobe64(originalOrderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares =
        htonl(initialQuantity);
    addMessage.price =
        htonl(originalPrice);

    OrderCancelWireMessage cancelMessage{};
    cancelMessage.message_type = 'X';
    cancelMessage.order_reference_number =
        htobe64(originalOrderId);
    cancelMessage.cancelled_shares =
        htonl(cancelledQuantity);

    OrderExecutedWireMessage executedMessage{};
    executedMessage.message_type = 'E';
    executedMessage.order_reference_number =
        htobe64(originalOrderId);
    executedMessage.executed_shares =
        htonl(executedQuantity);
    executedMessage.match_number =
        htobe64(matchNumber);

    OrderReplaceWireMessage replaceMessage{};
    replaceMessage.message_type = 'U';

    replaceMessage.original_order_reference =
    htobe64(originalOrderId);
    replaceMessage.new_order_reference =
    htobe64(replacementOrderId);
    replaceMessage.shares =
        htonl(replacementQuantity);
    replaceMessage.price =
        htonl(replacementPrice);

    OrderDeleteWireMessage deleteMessage{};
    deleteMessage.message_type = 'D';
    deleteMessage.order_reference_number =
        htobe64(replacementOrderId);

    {
        std::ofstream stream(path, std::ios::binary);
        ASSERT_TRUE(stream.is_open());

        writeMessage(stream, addMessage);
        writeMessage(stream, cancelMessage);
        writeMessage(stream, executedMessage);
        writeMessage(stream, replaceMessage);
        writeMessage(stream, deleteMessage);
    }

    ArrayOrderBook orderBook;
    ITCHHandler handler(orderBook);
    ItchReplayDispatcher dispatcher(handler);
    ItchReplayReader reader(path);

    ASSERT_TRUE(reader.isOpen());

    std::vector<std::uint8_t> message;
    std::size_t dispatchedMessages = 0;

    while (reader.readNext(message))
    {
        ASSERT_TRUE(
            dispatcher.dispatch(
                message.data(),
                message.size()));

        ++dispatchedMessages;
    }

    EXPECT_EQ(dispatchedMessages, 5U);
    EXPECT_EQ(orderBook.bestBid(), nullptr);
    EXPECT_EQ(orderBook.bestAsk(), nullptr);

    std::remove(path.c_str());
}