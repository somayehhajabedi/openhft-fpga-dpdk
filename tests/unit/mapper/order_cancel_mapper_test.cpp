#include <gtest/gtest.h>

#include "dpdk/parser/itch/mapper/order_cancel_mapper.hpp"
#include "dpdk/parser/itch/messages/order_cancel.hpp"

#include <bit>
#include <cstdint>

TEST(
    OrderCancelMapperTest,
    MapsFieldsCorrectly)
{
    constexpr std::uint64_t expectedOrderId =
        123456789ULL;

    constexpr std::uint32_t expectedCancelledShares =
        250U;

    OrderCancelWireMessage message{};

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

    const OrderCancel result =
        OrderCancelMapper::fromWire(
            &message);

    EXPECT_EQ(
        result.orderReferenceNumber,
        expectedOrderId);

    EXPECT_EQ(
        result.cancelledShares,
        expectedCancelledShares);
}
