#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/mapper/order_replace_mapper.hpp"

TEST(OrderReplaceMapperTest, MapsFieldsCorrectly)
{
    constexpr std::uint64_t originalOrderId = 4001;
    constexpr std::uint64_t newOrderId = 4002;
    constexpr std::uint32_t expectedShares = 800;
    constexpr std::uint32_t expectedPrice = 12750;

    OrderReplaceWireMessage message{};

    message.original_order_reference =
        htobe64(originalOrderId);

    message.new_order_reference =
        htobe64(newOrderId);

    message.shares =
        htobe32(expectedShares);

    message.price =
        htobe32(expectedPrice);

    const OrderReplace result =
        OrderReplaceMapper::fromWire(&message);

    EXPECT_EQ(
        result.originalOrderReferenceNumber,
        originalOrderId);

    EXPECT_EQ(
        result.newOrderReferenceNumber,
        newOrderId);

    EXPECT_EQ(
        result.shares,
        expectedShares);

    EXPECT_EQ(
        result.price,
        expectedPrice);
}