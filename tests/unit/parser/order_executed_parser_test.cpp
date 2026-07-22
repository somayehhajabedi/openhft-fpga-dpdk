#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/messages/order_executed_parser.hpp"

TEST(OrderExecutedParserTest, ParsesValidMessage)
{
    OrderExecutedWireMessage message{};
    message.message_type = 'E';

    const auto* parsed =
        OrderExecutedParser::parse(
            reinterpret_cast<const std::uint8_t*>(&message),
            sizeof(message));

    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->message_type, 'E');
}

TEST(OrderExecutedParserTest, RejectsNullBuffer)
{
    const auto* parsed =
        OrderExecutedParser::parse(
            nullptr,
            sizeof(OrderExecutedWireMessage));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderExecutedParserTest, RejectsShortMessage)
{
    std::uint8_t buffer[
        sizeof(OrderExecutedWireMessage) - 1]{};

    const auto* parsed =
        OrderExecutedParser::parse(
            buffer,
            sizeof(buffer));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderExecutedParserTest, ConvertsOrderReferenceNumber)
{
    constexpr std::uint64_t expectedOrderId = 3001;

    OrderExecutedWireMessage message{};
    message.order_reference_number =
        htobe64(expectedOrderId);

    EXPECT_EQ(
        OrderExecutedParser::orderReferenceNumber(&message),
        expectedOrderId);
}

TEST(OrderExecutedParserTest, ConvertsExecutedShares)
{
    constexpr std::uint32_t expectedShares = 250;

    OrderExecutedWireMessage message{};
    message.executed_shares =
        htobe32(expectedShares);

    EXPECT_EQ(
        OrderExecutedParser::executedShares(&message),
        expectedShares);
}

TEST(OrderExecutedParserTest, ConvertsMatchNumber)
{
    constexpr std::uint64_t expectedMatchNumber = 90001;

    OrderExecutedWireMessage message{};
    message.match_number =
        htobe64(expectedMatchNumber);

    EXPECT_EQ(
        OrderExecutedParser::matchNumber(&message),
        expectedMatchNumber);
}