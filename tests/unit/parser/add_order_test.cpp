#include "dpdk/parser/itch/messages/add_order.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

TEST(AddOrderParserTest, ParsesValidMessage)
{
    constexpr std::array<std::uint8_t, sizeof(AddOrderWireMessage)> message{
        'A'
    };

    const AddOrderWireMessage* add_order =
        AddOrderParser::parse(
            message.data(),
            message.size());

    ASSERT_NE(add_order, nullptr);

    EXPECT_EQ(add_order->message_type, 'A');
}

// Verifies that 16-bit and 32-bit fields encoded in network byte order
// are converted correctly by AddOrderParser.
TEST(AddOrderParserTest, ConvertsNetworkByteOrder)
{
    std::array<std::uint8_t, sizeof(AddOrderWireMessage)> message{};

    message[0] = 'A';

    // stock_locate = 0x1234
    message[1] = 0x12;
    message[2] = 0x34;

    // tracking_number = 0x5678
    message[3] = 0x56;
    message[4] = 0x78;

    // shares = 1000 = 0x000003E8
    message[20] = 0x00;
    message[21] = 0x00;
    message[22] = 0x03;
    message[23] = 0xE8;

    // price = 1234500 = 0x0012D644
    message[32] = 0x00;
    message[33] = 0x12;
    message[34] = 0xD6;
    message[35] = 0x44;

    const AddOrderWireMessage* add_order =
        AddOrderParser::parse(
            message.data(),
            message.size());

    ASSERT_NE(add_order, nullptr);

    EXPECT_EQ(AddOrderParser::messageType(add_order), 'A');
    EXPECT_EQ(AddOrderParser::stockLocate(add_order), 0x1234);
    EXPECT_EQ(AddOrderParser::trackingNumber(add_order), 0x5678);
    EXPECT_EQ(AddOrderParser::shares(add_order), 1000U);
    EXPECT_EQ(AddOrderParser::price(add_order), 1234500U);
}
