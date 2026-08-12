#include <gtest/gtest.h>

#include <cstdint>

#include <endian.h>
#include <arpa/inet.h>

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"
#include "dpdk/parser/itch/messages/order_delete.hpp"
#include "dpdk/parser/itch/messages/order_executed.hpp"
#include "dpdk/parser/itch/messages/order_replace.hpp"


class RecordingMarketDataEventSink final
    : public MarketDataEventSink
{
public:
    bool submit(
        const MarketDataEvent& event) override
    {
        lastEvent = event;
        submitted = true;
        return true;
    }

    MarketDataEvent lastEvent{};
    bool submitted{false};
};


TEST(ItchReplayDispatcherTest, RejectsNullMessage)
{
	
    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    EXPECT_FALSE(dispatcher.dispatch(nullptr, 0));
}

TEST(ItchReplayDispatcherTest, RejectsEmptyMessage)
{

    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    const std::uint8_t dummy = 0;

    EXPECT_FALSE(dispatcher.dispatch(&dummy, 0));
}

TEST(ItchReplayDispatcherTest, RejectsUnknownMessageType)
{
 
    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    const std::uint8_t message[]{'?'};

    EXPECT_FALSE(
        dispatcher.dispatch(
            message,
            sizeof(message)));
}


TEST(ItchReplayDispatcherTest, DispatchesAddOrder)
{

    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);
    
    constexpr std::uint64_t orderId = 5001;
    constexpr std::uint32_t quantity = 1000;
    constexpr std::uint32_t price = 12500;

    AddOrderWireMessage message{};

    message.message_type = 'A';
    message.order_reference_number = htobe64(orderId);
    message.buy_sell_indicator = 'B';
    message.shares = htonl(quantity);
    message.price = htonl(price);
    
    ASSERT_TRUE(
    dispatcher.dispatch(
        reinterpret_cast<const std::uint8_t*>(&message),
        sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
       sink.lastEvent.type,
       MarketDataEventType::AddOrder);

    EXPECT_EQ(
       sink.lastEvent.orderId,
       orderId);

    EXPECT_EQ(
       sink.lastEvent.side,
       Side::Buy);

    EXPECT_EQ(
       sink.lastEvent.quantity,
       quantity);

    EXPECT_EQ(
       sink.lastEvent.price,
       price);
}



TEST(ItchReplayDispatcherTest, DispatchesOrderCancel)
{

    
    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t orderId = 5002;
    constexpr std::uint32_t initialQuantity = 1000;
    constexpr std::uint32_t cancelledQuantity = 300;
    constexpr std::uint32_t expectedQuantity = 700;
    constexpr std::uint32_t price = 12600;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(orderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(initialQuantity);
    addMessage.price = htonl(price);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    OrderCancelWireMessage cancelMessage{};

    cancelMessage.message_type = 'X';
    cancelMessage.order_reference_number = htobe64(orderId);
    cancelMessage.cancelled_shares = htonl(cancelledQuantity);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&cancelMessage),
            sizeof(cancelMessage)));

    const PriceLevel* bestBid = orderBook.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(bestBid->head->id, orderId);
    EXPECT_EQ(bestBid->head->quantity, expectedQuantity);
}


TEST(ItchReplayDispatcherTest, DispatchesOrderDelete)
{

    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t orderId = 5003;
    constexpr std::uint32_t quantity = 1000;
    constexpr std::uint32_t price = 12700;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(orderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(quantity);
    addMessage.price = htonl(price);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    ASSERT_NE(orderBook.bestBid(), nullptr);

    OrderDeleteWireMessage deleteMessage{};

    deleteMessage.message_type = 'D';
    deleteMessage.order_reference_number = htobe64(orderId);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&deleteMessage),
            sizeof(deleteMessage)));

    EXPECT_EQ(orderBook.bestBid(), nullptr);
}
TEST(ItchReplayDispatcherTest, DispatchesOrderExecuted)
{

    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t orderId = 5004;
    constexpr std::uint32_t initialQuantity = 1000;
    constexpr std::uint32_t executedQuantity = 250;
    constexpr std::uint32_t expectedQuantity = 750;
    constexpr std::uint32_t price = 12800;
    constexpr std::uint64_t matchNumber = 91001;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(orderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(initialQuantity);
    addMessage.price = htonl(price);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    OrderExecutedWireMessage executedMessage{};

    executedMessage.message_type = 'E';
    executedMessage.order_reference_number = htobe64(orderId);
    executedMessage.executed_shares = htonl(executedQuantity);
    executedMessage.match_number = htobe64(matchNumber);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&executedMessage),
            sizeof(executedMessage)));

    const PriceLevel* bestBid = orderBook.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(bestBid->head->id, orderId);
    EXPECT_EQ(bestBid->head->quantity, expectedQuantity);
}

TEST(ItchReplayDispatcherTest, DispatchesOrderReplace)
{

    RecordingMarketDataEventSink sink;
    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t originalOrderId = 5005;
    constexpr std::uint64_t newOrderId = 6005;
    constexpr std::uint32_t originalQuantity = 1000;
    constexpr std::uint32_t newQuantity = 400;
    constexpr std::uint32_t originalPrice = 12900;
    constexpr std::uint32_t newPrice = 13000;

    AddOrderWireMessage addMessage{};

    addMessage.message_type = 'A';
    addMessage.order_reference_number = htobe64(originalOrderId);
    addMessage.buy_sell_indicator = 'B';
    addMessage.shares = htonl(originalQuantity);
    addMessage.price = htonl(originalPrice);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&addMessage),
            sizeof(addMessage)));

    OrderReplaceWireMessage replaceMessage{};

    replaceMessage.message_type = 'U';
    replaceMessage.original_order_reference_number =
        htobe64(originalOrderId);
    replaceMessage.new_order_reference_number =
        htobe64(newOrderId);
    replaceMessage.shares =
        htonl(newQuantity);
    replaceMessage.price =
        htonl(newPrice);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<const std::uint8_t*>(&replaceMessage),
            sizeof(replaceMessage)));

    const PriceLevel* bestBid = orderBook.bestBid();

    ASSERT_NE(bestBid, nullptr);
    ASSERT_NE(bestBid->head, nullptr);

    EXPECT_EQ(bestBid->head->id, newOrderId);
    EXPECT_EQ(bestBid->head->quantity, newQuantity);
    EXPECT_EQ(bestBid->head->price, newPrice);
}
