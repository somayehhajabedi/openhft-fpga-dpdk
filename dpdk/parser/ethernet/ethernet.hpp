#pragma once

#include <cstddef>
#include <cstdint>

struct EthernetHeader
{
    std::uint8_t destination[6];
    std::uint8_t source[6];
    std::uint16_t ether_type;
};

static_assert(sizeof(EthernetHeader) == 14);

class EthernetParser
{
public:
    static const EthernetHeader* parse(
        const std::uint8_t* data,
        std::size_t length);

    static std::uint16_t etherType(
        const EthernetHeader* header);

    static const std::uint8_t* payload(
        const EthernetHeader* header);

      
};