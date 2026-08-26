#include <gtest/gtest.h>

#include "dpdk/parser/itch/messages/order_cancel_parser.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"

#include <bit>
#include <cstdint>

TEST(
    OrderCancelParserTest,
    ParsesValidMessage)
{
    OrderCancelWireMessage message{};

    constexpr std::uint64_t expectedOrderId =
        123456789ULL;

    constexpr std::uint32_t expectedCancelledShares =
        250U;

    if constexpr (
        std::endian::native ==
        std::endian::little)
    {
        message.order_reference_number =
            std::byteswap(expectedOrderId);

        message.cancelled_shares =
            std::byteswap(expectedCancelledShares);
    }
    else
    {
        message.order_reference_number =
            expectedOrderId;

        message.cancelled_shares =
            expectedCancelledShares;
    }

    const auto* parsed =
        OrderCancelParser::parse(
            reinterpret_cast<const std::uint8_t*>(
                &message),
            sizeof(message));

    ASSERT_NE(parsed, nullptr);

    EXPECT_EQ(
        OrderCancelParser::orderReferenceNumber(
            parsed),
        expectedOrderId);

    EXPECT_EQ(
        OrderCancelParser::cancelledShares(
            parsed),
        expectedCancelledShares);
}


TEST(
    OrderCancelParserTest,
    RejectsNullBuffer)
{
    EXPECT_EQ(
        OrderCancelParser::parse(
            nullptr,
            sizeof(OrderCancelWireMessage)),
        nullptr);
}


TEST(
    OrderCancelParserTest,
    RejectsShortMessage)
{
    OrderCancelWireMessage message{};

    const auto* parsed =
        OrderCancelParser::parse(
            reinterpret_cast<const std::uint8_t*>(
                &message),
            sizeof(message) - 1);

    EXPECT_EQ(parsed, nullptr);
}
