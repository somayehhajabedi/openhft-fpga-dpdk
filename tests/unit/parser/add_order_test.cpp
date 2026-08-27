#include "dpdk/parser/itch/messages/add_order.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string_view>

TEST(AddOrderParserTest, ParsesValidMessage)
{
    constexpr std::array<
        std::uint8_t,
        sizeof(AddOrderWireMessage)> message{
            'A'
        };

    const AddOrderWireMessage* addOrder =
        AddOrderParser::parse(
            message.data(),
            message.size());

    ASSERT_NE(
        addOrder,
        nullptr);

    EXPECT_EQ(
        addOrder->message_type,
        'A');
}


// Verifies that 16-bit and 32-bit fields encoded in network byte order
// are converted correctly by AddOrderParser.
TEST(
    AddOrderParserTest,
    ConvertsNetworkByteOrder)
{
    std::array<
        std::uint8_t,
        sizeof(AddOrderWireMessage)> message{};

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

    const AddOrderWireMessage* addOrder =
        AddOrderParser::parse(
            message.data(),
            message.size());

    ASSERT_NE(
        addOrder,
        nullptr);

    EXPECT_EQ(
        AddOrderParser::messageType(
            addOrder),
        'A');

    EXPECT_EQ(
        AddOrderParser::stockLocate(
            addOrder),
        0x1234);

    EXPECT_EQ(
        AddOrderParser::trackingNumber(
            addOrder),
        0x5678);

    EXPECT_EQ(
        AddOrderParser::shares(
            addOrder),
        1000U);

    EXPECT_EQ(
        AddOrderParser::price(
            addOrder),
        1234500U);
}


TEST(
    AddOrderParserTest,
    ReturnsStockAsStringView)
{
    AddOrderWireMessage message{};

    constexpr char expectedStock[8] = {
        'A', 'A', 'P', 'L',
        ' ', ' ', ' ', ' '
    };

    for (std::size_t index = 0;
         index < sizeof(message.stock);
         ++index)
    {
        message.stock[index] =
            expectedStock[index];
    }

    const std::string_view stock =
        AddOrderParser::stockView(
            &message);

    const std::string_view expectedView{
        expectedStock,
        sizeof(expectedStock)
    };

    EXPECT_EQ(
        stock.size(),
        8U);

    EXPECT_EQ(
        stock,
        expectedView);
}


TEST(
    AddOrderParserTest,
    StockViewIsZeroCopy)
{
    AddOrderWireMessage message{};

    message.stock[0] = 'I';
    message.stock[1] = 'B';
    message.stock[2] = 'M';

    const std::string_view stock =
        AddOrderParser::stockView(
            &message);

    EXPECT_EQ(
        stock.data(),
        message.stock);
}


TEST(
    AddOrderParserTest,
    StockViewHandlesNullMessage)
{
    const std::string_view stock =
        AddOrderParser::stockView(
            nullptr);

    EXPECT_TRUE(
        stock.empty());
}