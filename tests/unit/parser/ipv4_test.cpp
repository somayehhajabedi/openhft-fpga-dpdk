
/******************************************************************************
 *
 * File:
 *     ipv4_test.cpp
 *
 * Purpose:
 *     Unit tests for IPv4Parser.
 *
 * Coverage:
 *     - Valid IPv4 header parsing
 *     - Null input rejection
 *     - Truncated header rejection
 *     - Version validation
 *     - Header-length validation
 *     - Network-byte-order conversion
 *     - Payload pointer and payload length
 *
 ******************************************************************************/


#include "dpdk/parser/ipv4/ipv4.hpp"

#include <gtest/gtest.h>

#include <array>



// Verifies that a valid IPv4 header is parsed and its main fields
// are converted and exposed correctly.
TEST(IPv4ParserTest, ParsesValidIPv4Header)
{
    constexpr std::array<std::uint8_t,20> packet
    {
        0x45,
        0x00,

        0x00,0x1C,

        0x00,0x01,

        0x00,0x00,

        64,

        17,

        0x00,0x00,

        192,168,1,1,

        192,168,1,2
    };

    auto* ip =
        IPv4Parser::parse(
            packet.data(),
            packet.size());

    ASSERT_NE(ip,nullptr);

    EXPECT_EQ(
        IPv4Parser::version(ip),
        4);

    EXPECT_EQ(
        IPv4Parser::headerLengthBytes(ip),
        20);

    EXPECT_EQ(
        IPv4Parser::totalLength(ip),
        28);
}


// Verifies that the parser safely rejects a null packet buffer.
TEST(IPv4ParserTest, RejectNullPointer)
{
    EXPECT_EQ(
        IPv4Parser::parse(nullptr,20),
        nullptr);
}


TEST(IPv4ParserTest, RejectShortPacket)
{
    constexpr std::array<std::uint8_t,10> packet{};

    EXPECT_EQ(
        IPv4Parser::parse(
            packet.data(),
            packet.size()),
        nullptr);
}
TEST(IPv4ParserTest, RejectNonIPv4Packet)
{
    std::array<std::uint8_t,20> packet{};

    packet[0]=0x65;

    EXPECT_EQ(
        IPv4Parser::parse(
            packet.data(),
            packet.size()),
        nullptr);
}
TEST(IPv4ParserTest, PayloadPointer)
{
    std::array<std::uint8_t,24> packet{};

    packet[0]=0x45;

    packet[2]=0x00;
    packet[3]=0x18;

    auto* ip=
        IPv4Parser::parse(
            packet.data(),
            packet.size());

    ASSERT_NE(ip,nullptr);

    auto* payload=
        IPv4Parser::payload(ip);

    EXPECT_EQ(
        payload,
        packet.data()+20);
}


