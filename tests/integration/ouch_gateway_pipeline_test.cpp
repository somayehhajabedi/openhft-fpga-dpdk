#include <gtest/gtest.h>

#include "execution/ouch/ouch_execution_sink.hpp"
#include "execution/ouch/ouch_transport.hpp"
#include "gateway/gateway.hpp"
#include "models/order_intent.hpp"
#include "risk/risk_manager.hpp"

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
        return true;
    }

    std::array<std::uint8_t, 64> lastData{};
    std::size_t lastLength{0};
    bool sent{false};
};

} // namespace

TEST(
    OuchGatewayPipelineTest,
    ValidOrderPassesRiskAndIsEncodedToOuch)
{
    RecordingTransport transport;

    ouch::OuchExecutionSink executionSink(
        transport);

    RiskManager riskManager;

    Gateway gateway(
        riskManager,
        executionSink);

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .symbol = {
            'A', 'A', 'P', 'L',
            ' ', ' ', ' ', ' '
        },
        .price = 100,
        .quantity = 100
    };

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::Accepted);

    ASSERT_TRUE(
        transport.sent);

    EXPECT_EQ(
        transport.lastLength,
        47U);

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
    OuchGatewayPipelineTest,
    RiskRejectedOrderNeverReachesTransport)
{
    RecordingTransport transport;

    ouch::OuchExecutionSink executionSink(
        transport);

    RiskManager riskManager;

    Gateway gateway(
        riskManager,
        executionSink);

    const OrderIntent intent{
        .accountId = 1001,
        .side = Side::Buy,
        .symbol = {
            'A', 'A', 'P', 'L',
            ' ', ' ', ' ', ' '
        },
        .price = 0,
        .quantity = 100
    };

    EXPECT_EQ(
        gateway.submit(intent),
        RiskResult::InvalidPrice);

    EXPECT_FALSE(
        transport.sent);
}
