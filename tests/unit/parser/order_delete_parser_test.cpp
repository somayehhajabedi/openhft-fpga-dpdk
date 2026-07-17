#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/messages/order_delete_parser.hpp"

TEST(OrderDeleteParserTest, ParsesValidMessage)
{
    OrderDeleteWireMessage message{};
    message.message_type = 'D';

    const auto* parsed =
        OrderDeleteParser::parse(
            reinterpret_cast<const std::uint8_t*>(&message),
            sizeof(message));

    ASSERT_NE(parsed, nullptr);
    EXPECT_EQ(parsed->message_type, 'D');
}

TEST(OrderDeleteParserTest, RejectsNullBuffer)
{
    const auto* parsed =
        OrderDeleteParser::parse(
            nullptr,
            sizeof(OrderDeleteWireMessage));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderDeleteParserTest, RejectsShortMessage)
{
    std::uint8_t buffer[
        sizeof(OrderDeleteWireMessage) - 1]{};

    const auto* parsed =
        OrderDeleteParser::parse(
            buffer,
            sizeof(buffer));

    EXPECT_EQ(parsed, nullptr);
}

TEST(OrderDeleteParserTest, ConvertsOrderReferenceNumber)
{
    constexpr std::uint64_t expectedOrderId = 1001;

    OrderDeleteWireMessage message{};
    message.order_reference_number =
        htobe64(expectedOrderId);

    const auto actualOrderId =
        OrderDeleteParser::orderReferenceNumber(
            &message);

    EXPECT_EQ(actualOrderId, expectedOrderId);
}