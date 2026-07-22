/******************************************************************************
 *
 * File:
 *     itch_test.cpp
 *
 * Purpose:
 *     Unit tests for ITCHParser.
 *
 * Coverage:
 *     - Valid message type parsing
 *     - Null input rejection
 *     - Truncated input rejection
 *
 ******************************************************************************/

#include "dpdk/parser/itch/itch.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

// Verifies that the parser reads the first byte as the ITCH message type.
TEST(ITCHParserTest, ParsesMessageType)
{
    constexpr std::array<std::uint8_t, 1> message{
        static_cast<std::uint8_t>('A')
    };

    const ITCHHeader* header =
        ITCHParser::parse(message.data(), message.size());

    ASSERT_NE(header, nullptr);
    EXPECT_EQ(ITCHParser::messageType(header), 'A');
}

// Verifies that a null message buffer is rejected safely.
TEST(ITCHParserTest, RejectsNullBuffer)
{
    EXPECT_EQ(ITCHParser::parse(nullptr, 1), nullptr);
}

// Verifies that a zero-length message is rejected.
TEST(ITCHParserTest, RejectsEmptyMessage)
{
    constexpr std::array<std::uint8_t, 1> message{
        static_cast<std::uint8_t>('A')
    };

    EXPECT_EQ(ITCHParser::parse(message.data(), 0), nullptr);
}

// Verifies that an Add Order message is recognized correctly.
TEST(ITCHParserTest, DetectsAddOrder)
{
    constexpr std::array<std::uint8_t, 1> message{
        static_cast<std::uint8_t>('A')
    };

    const ITCHHeader* header =
        ITCHParser::parse(message.data(), message.size());

    ASSERT_NE(header, nullptr);

    EXPECT_TRUE(
        ITCHParser::isAddOrder(header));
}
// Verifies that payload() returns the byte immediately
// following the ITCH header.
TEST(ITCHParserTest, ReturnsPayloadPointer)
{
    constexpr std::array<std::uint8_t, 4> message{
        static_cast<std::uint8_t>('A'),
        0x11,
        0x22,
        0x33
    };

    const ITCHHeader* header =
        ITCHParser::parse(message.data(), message.size());

    ASSERT_NE(header, nullptr);

    const std::uint8_t* payload =
        ITCHParser::payload(header);

    ASSERT_NE(payload, nullptr);

    EXPECT_EQ(*payload, 0x11);
}