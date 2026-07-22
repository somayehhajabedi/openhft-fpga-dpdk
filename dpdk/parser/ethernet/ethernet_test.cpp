#include "ethernet.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <arpa/inet.h>

int main()
{
    const std::uint8_t packet[] =
    {
        // Destination MAC
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,

        // Source MAC
        0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,

        // EtherType = IPv4 (0x0800), network byte order
        0x08, 0x00
    };

    const EthernetHeader* header =
        EthernetParser::parse(packet, sizeof(packet));

    assert(header != nullptr);

    const std::uint16_t type = EthernetParser::etherType(header);

    assert(type == 0x0800);

    std::cout << "EtherType: 0x"
          << std::hex
          << type
          << '\n';

    std::cout << "Ethernet header parsed successfully\n";

    return 0;
}