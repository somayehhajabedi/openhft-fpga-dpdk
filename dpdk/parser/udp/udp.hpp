#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#pragma pack(push, 1)

struct UDPHeader
{
    std::uint16_t source_port;
    std::uint16_t destination_port;
    std::uint16_t length;
    std::uint16_t checksum;
};

#pragma pack(pop)

static_assert(sizeof(UDPHeader) == 8);

class UDPParser
{
public:
    static const UDPHeader* parse(
        std::span<const std::uint8_t> data);

    static const UDPHeader* parse(
        const std::uint8_t* data,
        std::size_t length);

    static std::uint16_t sourcePort(
        const UDPHeader* header);

    static std::uint16_t destinationPort(
        const UDPHeader* header);

    static std::uint16_t payloadLength(
        const UDPHeader* header);

    static const std::uint8_t* payload(
        const UDPHeader* header);
};