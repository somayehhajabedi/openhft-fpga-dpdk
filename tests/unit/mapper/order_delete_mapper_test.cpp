#include <gtest/gtest.h>

#include <endian.h>
#include <cstdint>

#include "dpdk/parser/itch/mapper/order_delete_mapper.hpp"

TEST(OrderDeleteMapperTest, MapsOrderReferenceNumber)
{
    constexpr std::uint64_t expectedOrderId = 1001;

    OrderDeleteWireMessage message{};
    message.order_reference_number =
        htobe64(expectedOrderId);

    const OrderDelete result =
        OrderDeleteMapper::fromWire(&message);

    EXPECT_EQ(
        result.orderReferenceNumber,
        expectedOrderId);
}