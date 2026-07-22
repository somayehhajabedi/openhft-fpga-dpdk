#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/mapper/order_executed_mapper.hpp"

TEST(OrderExecutedMapperTest, MapsFieldsCorrectly)
{
    constexpr std::uint64_t expectedOrderId = 3001;
    constexpr std::uint32_t expectedExecutedShares = 250;
    constexpr std::uint64_t expectedMatchNumber = 90001;

    OrderExecutedWireMessage message{};

    message.order_reference_number =
        htobe64(expectedOrderId);

    message.executed_shares =
        htobe32(expectedExecutedShares);

    message.match_number =
        htobe64(expectedMatchNumber);

    const OrderExecuted result =
        OrderExecutedMapper::fromWire(&message);

    EXPECT_EQ(
        result.orderReferenceNumber,
        expectedOrderId);

    EXPECT_EQ(
        result.executedShares,
        expectedExecutedShares);

    EXPECT_EQ(
        result.matchNumber,
        expectedMatchNumber);
}