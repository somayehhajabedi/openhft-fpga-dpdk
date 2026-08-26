#include <gtest/gtest.h>

#include "dpdk/parser/udp/udp.hpp"

#include <bit>
#include <cstdint>

TEST(
    UDPParserTest,
    ParsesValidHeader)
{
    UDPHeader header{};

    constexpr std::uint16_t sourcePort = 1234;
    constexpr std::uint16_t destinationPort = 5678;
    constexpr std::uint16_t totalLength =
        static_cast<std::uint16_t>(
            sizeof(UDPHeader) + 20);

    if constexpr (
        std::endian::native ==
        std::endian::little)
    {
        header.source_port =
            std::byteswap(sourcePort);

        header.destination_port =
            std::byteswap(destinationPort);

        header.length =
            std::byteswap(totalLength);
    }
    else
    {
        header.source_port =
            sourcePort;

        header.destination_port =
            destinationPort;

        header.length =
            totalLength;
    }

    const auto* parsed =
        UDPParser::parse(
            reinterpret_cast<const std::uint8_t*>(
                &header),
            sizeof(header));

    ASSERT_NE(parsed, nullptr);

    EXPECT_EQ(
        UDPParser::sourcePort(parsed),
        sourcePort);

    EXPECT_EQ(
        UDPParser::destinationPort(parsed),
        destinationPort);

    EXPECT_EQ(
        UDPParser::payloadLength(parsed),
        20U);
}


TEST(
    UDPParserTest,
    RejectsNullBuffer)
{
    EXPECT_EQ(
        UDPParser::parse(
            nullptr,
            sizeof(UDPHeader)),
        nullptr);
}


TEST(
    UDPParserTest,
    RejectsShortHeader)
{
    UDPHeader header{};

    EXPECT_EQ(
        UDPParser::parse(
            reinterpret_cast<const std::uint8_t*>(
                &header),
            sizeof(UDPHeader) - 1),
        nullptr);
}


TEST(
    UDPParserTest,
    ReturnsPayloadPointer)
{
    struct Packet
    {
        UDPHeader header{};
        std::uint8_t payload[4]{};
    };

    Packet packet{};

    const auto* payload =
        UDPParser::payload(
            &packet.header);

    EXPECT_EQ(
        payload,
        reinterpret_cast<const std::uint8_t*>(
            &packet.header) + sizeof(UDPHeader));
}
