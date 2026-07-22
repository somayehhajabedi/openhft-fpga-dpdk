#include "dpdk/parser/ethernet/ethernet.hpp"
#include "dpdk/parser/ipv4/ipv4.hpp"
#include "dpdk/parser/udp/udp.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

// Verifies the complete Ethernet -> IPv4 -> UDP parsing pipeline
// and confirms that the original UDP payload is preserved.
TEST(ParserPipelineTest, ParsesCompleteUdpPacket)
{
    constexpr std::string_view expected_payload{"Hello DPDK"};

    constexpr std::array<std::uint8_t, 52> packet{
        // Ethernet header: 14 bytes
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, // Destination MAC
        0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, // Source MAC
        0x08, 0x00,                         // EtherType = IPv4

        // IPv4 header: 20 bytes
        0x45,       // Version = 4, IHL = 5
        0x00,       // DSCP / ECN
        0x00, 0x26, // Total length = 38 bytes
        0x00, 0x01, // Identification
        0x00, 0x00, // Flags / Fragment offset
        0x40,       // TTL = 64
        0x11,       // Protocol = UDP
        0x00, 0x00, // Header checksum
        0xC0, 0xA8, 0x01, 0x0A, // Source IP: 192.168.1.10
        0xC0, 0xA8, 0x01, 0x14, // Destination IP: 192.168.1.20

        // UDP header: 8 bytes
        0x30, 0x39, // Source port = 12345
        0x23, 0x28, // Destination port = 9000
        0x00, 0x12, // UDP length = 18 bytes
        0x00, 0x00, // Checksum

        // UDP payload: "Hello DPDK"
        'H', 'e', 'l', 'l', 'o', ' ',
        'D', 'P', 'D', 'K'
    };

    const EthernetHeader* ethernet =
        EthernetParser::parse(packet.data(), packet.size());

    ASSERT_NE(ethernet, nullptr);
    ASSERT_EQ(EthernetParser::etherType(ethernet), 0x0800);

    const std::uint8_t* ipv4_data =
        EthernetParser::payload(ethernet);

    const std::size_t ipv4_available_length =
        packet.size() - sizeof(EthernetHeader);

    const IPv4Header* ipv4 =
        IPv4Parser::parse(ipv4_data, ipv4_available_length);

    ASSERT_NE(ipv4, nullptr);
    ASSERT_EQ(ipv4->protocol, 17);

    const std::uint8_t* udp_data =
        IPv4Parser::payload(ipv4);

    const UDPHeader* udp =
        UDPParser::parse(
            udp_data,
            IPv4Parser::payloadLength(ipv4));

    ASSERT_NE(udp, nullptr);
    EXPECT_EQ(UDPParser::sourcePort(udp), 12345);
    EXPECT_EQ(UDPParser::destinationPort(udp), 9000);
    EXPECT_EQ(
        UDPParser::payloadLength(udp),
        expected_payload.size());

    const auto* payload =
        reinterpret_cast<const char*>(
            UDPParser::payload(udp));

    const std::string_view actual_payload{
        payload,
        UDPParser::payloadLength(udp)
    };

    EXPECT_EQ(actual_payload, expected_payload);
}