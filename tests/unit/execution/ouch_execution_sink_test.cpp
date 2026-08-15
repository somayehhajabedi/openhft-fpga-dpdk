#include <gtest/gtest.h>

#include "execution/ouch/ouch_execution_sink.hpp"
#include "execution/ouch/ouch_encoder.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace
{

class RecordingTransport final : public ouch::OuchTransport
{
public:
    bool send(
        const std::uint8_t* data,
        std::size_t length) override
    {
        lastLength = length;

        if (length > lastData.size())
        {
            return false;
        }

        for (std::size_t i = 0;
             i < length;
             ++i)
        {
            lastData[i] = data[i];
        }

        sent = true;
        return acceptSend;
    }

    std::array<std::uint8_t, 64> lastData{};
    std::size_t lastLength{0};
    bool sent{false};
    bool acceptSend{true};
};

} // namespace

TEST(
    OuchExecutionSinkTest,
    EncodesAndSendsOrderIntent)
{
    RecordingTransport transport;

    ouch::OuchExecutionSink sink(
        transport);

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .symbol = {
            'A', 'A', 'P', 'L',
            ' ', ' ', ' ', ' '
        },
        .price = 12345,
        .quantity = 100
    };

    ASSERT_TRUE(
        sink.submit(intent));

    ASSERT_TRUE(
        transport.sent);

    ASSERT_EQ(
        transport.lastLength,
        ouch::OuchEncoder::EnterOrderSize);

    EXPECT_EQ(
        transport.lastData[0],
        static_cast<std::uint8_t>('O'));

    EXPECT_EQ(
        transport.lastData[5],
        static_cast<std::uint8_t>('B'));

    EXPECT_EQ(
        transport.lastData[10],
        static_cast<std::uint8_t>('A'));

    EXPECT_EQ(
        transport.lastData[11],
        static_cast<std::uint8_t>('A'));

    EXPECT_EQ(
        transport.lastData[12],
        static_cast<std::uint8_t>('P'));

    EXPECT_EQ(
        transport.lastData[13],
        static_cast<std::uint8_t>('L'));
}

TEST(
    OuchExecutionSinkTest,
    ReturnsFalseWhenTransportRejectsSend)
{
    RecordingTransport transport;
    transport.acceptSend = false;

    ouch::OuchExecutionSink sink(
        transport);

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Sell,
        .symbol = {
            'M', 'S', 'F', 'T',
            ' ', ' ', ' ', ' '
        },
        .price = 20000,
        .quantity = 50
    };

    EXPECT_FALSE(
        sink.submit(intent));
}
