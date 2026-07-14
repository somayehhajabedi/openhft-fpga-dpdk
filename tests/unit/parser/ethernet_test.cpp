#include "dpdk/parser/ethernet/ethernet.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

TEST(EthernetParserTest, ParsesValidIPv4Frame)
{
    constexpr std::array<std::uint8_t, 14> packet{
        // Destination MAC
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,

        // Source MAC
        0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,

        // EtherType: IPv4
        0x08, 0x00
    };

    const EthernetHeader* header =
        EthernetParser::parse(packet.data(), packet.size());

    ASSERT_NE(header, nullptr);
    EXPECT_EQ(EthernetParser::etherType(header), 0x0800);
    EXPECT_EQ(header->destination[0], 0x00);
    EXPECT_EQ(header->source[0], 0x66);
}

TEST(EthernetParserTest, RejectsNullBuffer)
{
    EXPECT_EQ(EthernetParser::parse(nullptr, 14), nullptr);
}

TEST(EthernetParserTest, RejectsTruncatedHeader)
{
    constexpr std::array<std::uint8_t, 10> packet{};

    EXPECT_EQ(
        EthernetParser::parse(packet.data(), packet.size()),
        nullptr);
}

TEST(EthernetParserTest, ReturnsPayloadAfterHeader)
{
    constexpr std::array<std::uint8_t, 15> packet{
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB,
        0x08, 0x00,
        0x45
    };

    const EthernetHeader* header =
        EthernetParser::parse(packet.data(), packet.size());

    ASSERT_NE(header, nullptr);

    const std::uint8_t* payload =
        EthernetParser::payload(header);

    ASSERT_NE(payload, nullptr);
    EXPECT_EQ(*payload, 0x45);
}
