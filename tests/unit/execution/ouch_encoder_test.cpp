#include <gtest/gtest.h>

#include "execution/ouch/ouch_encoder.hpp"

TEST(
    OuchEncoderTest,
    EncodesEnterOrderWithExpectedWireLayout)
{
    ouch::EnterOrder order{};

    order.userRefNum = 0x01020304;
    order.side = Side::Buy;
    order.quantity = 100;

    order.symbol = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    order.price = 12345;

    order.timeInForce =
        ouch::TimeInForce::Day;

    order.display =
        ouch::Display::Visible;

    order.capacity =
        ouch::Capacity::Agency;

    order.isoEligibility =
        ouch::IsoEligibility::NotEligible;

    order.crossType =
        ouch::CrossType::ContinuousMarket;

    order.clOrdId = {
        'C', 'L', 'O', 'R', 'D',
        '0', '0', '0', '0', '0',
        '0', '0', '0', '1'
    };

    const auto buffer =
        ouch::OuchEncoder::encode(order);

    ASSERT_EQ(
        buffer.size(),
        ouch::OuchEncoder::EnterOrderSize);

    // Message type
    EXPECT_EQ(buffer[0], static_cast<std::uint8_t>('O'));

    // UserRefNum = 0x01020304, big endian
    EXPECT_EQ(buffer[1], 0x01);
    EXPECT_EQ(buffer[2], 0x02);
    EXPECT_EQ(buffer[3], 0x03);
    EXPECT_EQ(buffer[4], 0x04);

    // Side
    EXPECT_EQ(buffer[5], static_cast<std::uint8_t>('B'));

    // Quantity = 100 = 0x00000064
    EXPECT_EQ(buffer[6], 0x00);
    EXPECT_EQ(buffer[7], 0x00);
    EXPECT_EQ(buffer[8], 0x00);
    EXPECT_EQ(buffer[9], 0x64);

    // Symbol
    EXPECT_EQ(buffer[10], static_cast<std::uint8_t>('A'));
    EXPECT_EQ(buffer[11], static_cast<std::uint8_t>('A'));
    EXPECT_EQ(buffer[12], static_cast<std::uint8_t>('P'));
    EXPECT_EQ(buffer[13], static_cast<std::uint8_t>('L'));

    // Fixed alpha fields
    EXPECT_EQ(
        buffer[26],
        static_cast<std::uint8_t>('0'));

    EXPECT_EQ(
        buffer[27],
        static_cast<std::uint8_t>('Y'));

    EXPECT_EQ(
        buffer[28],
        static_cast<std::uint8_t>('A'));

    EXPECT_EQ(
        buffer[29],
        static_cast<std::uint8_t>('N'));

    EXPECT_EQ(
        buffer[30],
        static_cast<std::uint8_t>('N'));

    // ClOrdID starts at offset 31
    EXPECT_EQ(
        buffer[31],
        static_cast<std::uint8_t>('C'));

    EXPECT_EQ(
        buffer[44],
        static_cast<std::uint8_t>('1'));

    // No optional appendages
    EXPECT_EQ(buffer[45], 0x00);
    EXPECT_EQ(buffer[46], 0x00);
}
