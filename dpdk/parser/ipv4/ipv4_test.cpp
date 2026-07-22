#include "ipv4.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    const std::uint8_t packet[] =
    {
        0x45,       // Version = 4, IHL = 5
        0x00,       // DSCP / ECN
        0x00, 0x1c, // Total length = 28 bytes
        0x00, 0x01, // Identification
        0x00, 0x00, // Flags / Fragment offset
        0x40,       // TTL = 64
        0x11,       // Protocol = UDP
        0x00, 0x00, // Header checksum

        // Source IP: 192.168.1.10
        0xc0, 0xa8, 0x01, 0x0a,

        // Destination IP: 192.168.1.20
        0xc0, 0xa8, 0x01, 0x14
    };

    const IPv4Header* header =
        IPv4Parser::parse(packet, sizeof(packet));

    assert(header != nullptr);
    assert(IPv4Parser::version(header) == 4);
    assert(IPv4Parser::headerLengthBytes(header) == 20);
    assert(IPv4Parser::totalLength(header) == 28);
    assert(header->protocol == 17);

    std::cout << "IPv4 header parsed successfully\n";
    std::cout << "Version: "
              << static_cast<int>(IPv4Parser::version(header))
              << '\n';
    std::cout << "Header length: "
              << static_cast<int>(
                     IPv4Parser::headerLengthBytes(header))
              << '\n';
    std::cout << "Total length: "
              << IPv4Parser::totalLength(header)
              << '\n';

    return 0;
}