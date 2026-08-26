#include <gtest/gtest.h>

#include <cstdint>

#include "common/endian.hpp"

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"
#include "dpdk/parser/itch/messages/order_delete.hpp"
#include "dpdk/parser/itch/messages/order_executed.hpp"
#include "dpdk/parser/itch/messages/order_replace.hpp"

#include "market_data/replay/itch_replay_dispatcher.hpp"
#include "pipeline/market_data_event_sink.hpp"

namespace
{

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

} // namespace

TEST(
    ItchReplayDispatcherTest,
    RejectsNullMessage)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    EXPECT_FALSE(
        dispatcher.dispatch(nullptr, 0));

    EXPECT_FALSE(sink.submitted);
}

TEST(
    ItchReplayDispatcherTest,
    RejectsEmptyMessage)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    const std::uint8_t dummy = 0;

    EXPECT_FALSE(
        dispatcher.dispatch(
            &dummy,
            0));

    EXPECT_FALSE(sink.submitted);
}

TEST(
    ItchReplayDispatcherTest,
    RejectsUnknownMessageType)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    const std::uint8_t message[]{'?'};

    EXPECT_FALSE(
        dispatcher.dispatch(
            message,
            sizeof(message)));

    EXPECT_FALSE(sink.submitted);
}

TEST(
    ItchReplayDispatcherTest,
    DispatchesAddOrder)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t OrderId = 5001;
    constexpr std::uint32_t Quantity = 1000;
    constexpr std::uint32_t PriceValue = 12500;

    AddOrderWireMessage message{};

    message.message_type = 'A';

    message.order_reference_number =
        toBigEndian(OrderId);

    message.buy_sell_indicator = 'B';

    message.shares =
        toBigEndian(Quantity);

    message.price =
        toBigEndian(PriceValue);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<
                const std::uint8_t*>(&message),
            sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
        sink.lastEvent.type,
        MarketDataEventType::AddOrder);

    EXPECT_EQ(
        sink.lastEvent.orderId,
        OrderId);

    EXPECT_EQ(
        sink.lastEvent.side,
        Side::Buy);

    EXPECT_EQ(
        sink.lastEvent.quantity,
        Quantity);

    EXPECT_EQ(
        sink.lastEvent.price,
        PriceValue);
}

TEST(
    ItchReplayDispatcherTest,
    DispatchesOrderCancel)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t OrderId = 5002;
    constexpr std::uint32_t CancelledQuantity = 300;

    OrderCancelWireMessage message{};

    message.message_type = 'X';

    message.order_reference_number =
        toBigEndian(OrderId);

    message.cancelled_shares =
        toBigEndian(CancelledQuantity);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<
                const std::uint8_t*>(&message),
            sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
        sink.lastEvent.type,
        MarketDataEventType::CancelOrder);

    EXPECT_EQ(
        sink.lastEvent.orderId,
        OrderId);

    EXPECT_EQ(
        sink.lastEvent.quantity,
        CancelledQuantity);
}

TEST(
    ItchReplayDispatcherTest,
    DispatchesOrderDelete)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t OrderId = 5003;

    OrderDeleteWireMessage message{};

    message.message_type = 'D';

    message.order_reference_number =
        toBigEndian(OrderId);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<
                const std::uint8_t*>(&message),
            sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
        sink.lastEvent.type,
        MarketDataEventType::DeleteOrder);

    EXPECT_EQ(
        sink.lastEvent.orderId,
        OrderId);
}

TEST(
    ItchReplayDispatcherTest,
    DispatchesOrderExecuted)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t OrderId = 5004;
    constexpr std::uint32_t ExecutedQuantity = 250;
    constexpr std::uint64_t MatchNumber = 91001;

    OrderExecutedWireMessage message{};

    message.message_type = 'E';

    message.order_reference_number =
        toBigEndian(OrderId);

    message.executed_shares =
        toBigEndian(ExecutedQuantity);

    message.match_number =
        toBigEndian(MatchNumber);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<
                const std::uint8_t*>(&message),
            sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
        sink.lastEvent.type,
        MarketDataEventType::ExecuteOrder);

    EXPECT_EQ(
        sink.lastEvent.orderId,
        OrderId);

    EXPECT_EQ(
        sink.lastEvent.quantity,
        ExecutedQuantity);
}

TEST(
    ItchReplayDispatcherTest,
    DispatchesOrderReplace)
{
    RecordingMarketDataEventSink sink;

    ItchReplayDispatcher dispatcher(sink);

    constexpr std::uint64_t OriginalOrderId = 5005;
    constexpr std::uint64_t NewOrderId = 6005;
    constexpr std::uint32_t NewQuantity = 400;
    constexpr std::uint32_t NewPrice = 13000;

    OrderReplaceWireMessage message{};

    message.message_type = 'U';

    message.original_order_reference =
        toBigEndian(OriginalOrderId);

    message.new_order_reference =
        toBigEndian(NewOrderId);

    message.shares =
        toBigEndian(NewQuantity);

    message.price =
        toBigEndian(NewPrice);

    ASSERT_TRUE(
        dispatcher.dispatch(
            reinterpret_cast<
                const std::uint8_t*>(&message),
            sizeof(message)));

    ASSERT_TRUE(sink.submitted);

    EXPECT_EQ(
        sink.lastEvent.type,
        MarketDataEventType::ReplaceOrder);

    EXPECT_EQ(
        sink.lastEvent.orderId,
        OriginalOrderId);

    EXPECT_EQ(
        sink.lastEvent.newOrderId,
        NewOrderId);

    EXPECT_EQ(
        sink.lastEvent.quantity,
        NewQuantity);

    EXPECT_EQ(
        sink.lastEvent.price,
        NewPrice);
}
