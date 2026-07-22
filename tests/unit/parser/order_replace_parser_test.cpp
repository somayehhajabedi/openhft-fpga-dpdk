#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/messages/order_replace_parser.hpp"

TEST(OrderReplaceParserTest, ParsesValidMessage)
{
    OrderReplaceWireMessage message{};
    message.message_type = 'U';

    const auto* parsed =
        OrderReplaceParser::parse(
            reinterpret_cast<const std::uint8_t*>(&message),
            sizeof(message));

    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->message_type, 'U');
}

TEST(OrderReplaceParserTest, RejectsNullBuffer)
{
    const auto* parsed =
        OrderReplaceParser::parse(
            nullptr,
            sizeof(OrderReplaceWireMessage));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderReplaceParserTest, RejectsShortMessage)
{
    std::uint8_t buffer[
        sizeof(OrderReplaceWireMessage) - 1]{};

    const auto* parsed =
        OrderReplaceParser::parse(
            buffer,
            sizeof(buffer));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderReplaceParserTest, ConvertsOriginalOrderReferenceNumber)
{
    constexpr std::uint64_t expectedOrderId = 4001;

    OrderReplaceWireMessage message{};
    message.original_order_reference =
        htobe64(expectedOrderId);

    EXPECT_EQ(
        OrderReplaceParser::originalOrderReferenceNumber(&message),
        expectedOrderId);
}

TEST(OrderReplaceParserTest, ConvertsNewOrderReferenceNumber)
{
    constexpr std::uint64_t expectedOrderId = 4002;

    OrderReplaceWireMessage message{};
    message.new_order_reference =
        htobe64(expectedOrderId);

    EXPECT_EQ(
        OrderReplaceParser::newOrderReferenceNumber(&message),
        expectedOrderId);
}

TEST(OrderReplaceParserTest, ConvertsShares)
{
    constexpr std::uint32_t expectedShares = 800;

    OrderReplaceWireMessage message{};
    message.shares = htobe32(expectedShares);

    EXPECT_EQ(
        OrderReplaceParser::shares(&message),
        expectedShares);
}

TEST(OrderReplaceParserTest, ConvertsPrice)
{
    constexpr std::uint32_t expectedPrice = 12750;

    OrderReplaceWireMessage message{};
    message.price = htobe32(expectedPrice);

    EXPECT_EQ(
        OrderReplaceParser::price(&message),
        expectedPrice);
}