/******************************************************************************
 *
 * File:
 *     add_order_mapper_test.cpp
 *
 * Purpose:
 *     Unit tests for AddOrderMapper.
 *
 * Coverage:
 *     - Mapping wire-format values to the internal AddOrder model
 *     - Network-byte-order conversion through AddOrderParser
 *
 ******************************************************************************/

#include "dpdk/parser/itch/mapper/add_order_mapper.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

// Verifies that the mapper converts parsed wire fields into
// the protocol-independent AddOrder domain model.
TEST(AddOrderMapperTest, MapsWireMessageToDomainModel)
{
    std::array<std::uint8_t, sizeof(AddOrderWireMessage)> message{};

    message[0] = 'A';

    // stockLocate = 0x1234
    message[1] = 0x12;
    message[2] = 0x34;

    // trackingNumber = 0x5678
    message[3] = 0x56;
    message[4] = 0x78;

    // shares = 1000
    message[20] = 0x00;
    message[21] = 0x00;
    message[22] = 0x03;
    message[23] = 0xE8;

    // price = 1234500
    message[32] = 0x00;
    message[33] = 0x12;
    message[34] = 0xD6;
    message[35] = 0x44;

    const AddOrderWireMessage* wire_message =
        AddOrderParser::parse(message.data(), message.size());

    ASSERT_NE(wire_message, nullptr);

    const AddOrder order =
        AddOrderMapper::fromWire(wire_message);

    EXPECT_EQ(order.stockLocate, 0x1234);
    EXPECT_EQ(order.trackingNumber, 0x5678);
    EXPECT_EQ(order.shares, 1000U);
    EXPECT_EQ(order.price, 1234500U);
}
