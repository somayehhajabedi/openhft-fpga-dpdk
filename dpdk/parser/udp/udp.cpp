#include "udp.hpp"

#include <arpa/inet.h>

const UDPHeader*
UDPParser::parse(const std::uint8_t* data,
                 std::size_t length)
{
    if (!data)
        return nullptr;

    if (length < sizeof(UDPHeader))
        return nullptr;

    return reinterpret_cast<const UDPHeader*>(data);
}

std::uint16_t
UDPParser::sourcePort(const UDPHeader* header)
{
    if (!header)
        return 0;

    return ntohs(header->source_port);
}

std::uint16_t
UDPParser::destinationPort(const UDPHeader* header)
{
    if (!header)
        return 0;

    return ntohs(header->destination_port);
}

std::uint16_t
UDPParser::payloadLength(const UDPHeader* header)
{
    if (!header)
        return 0;

    const std::uint16_t udp_length =
        ntohs(header->length);

    if (udp_length < sizeof(UDPHeader))
        return 0;

    return udp_length - sizeof(UDPHeader);
}

const std::uint8_t*
UDPParser::payload(const UDPHeader* header)
{
    if (!header)
        return nullptr;

    return reinterpret_cast<const std::uint8_t*>(header)
           + sizeof(UDPHeader);
}