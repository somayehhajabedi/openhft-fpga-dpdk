#include "udp.hpp"

#include "common/endian.hpp"

const UDPHeader*
UDPParser::parse(
    const std::uint8_t* data,
    std::size_t length)
{
    if (!data)
    {
        return nullptr;
    }

    if (length < sizeof(UDPHeader))
    {
        return nullptr;
    }

    return reinterpret_cast<const UDPHeader*>(
        data);
}

std::uint16_t
UDPParser::sourcePort(
    const UDPHeader* header)
{
    if (!header)
    {
        return 0;
    }

    return fromBigEndian(
        header->source_port);
}

std::uint16_t
UDPParser::destinationPort(
    const UDPHeader* header)
{
    if (!header)
    {
        return 0;
    }

    return fromBigEndian(
        header->destination_port);
}

std::uint16_t
UDPParser::payloadLength(
    const UDPHeader* header)
{
    if (!header)
    {
        return 0;
    }

    const std::uint16_t udpLength =
        fromBigEndian(
            header->length);

    if (udpLength < sizeof(UDPHeader))
    {
        return 0;
    }

    return static_cast<std::uint16_t>(
        udpLength - sizeof(UDPHeader));
}

const std::uint8_t*
UDPParser::payload(
    const UDPHeader* header)
{
    if (!header)
    {
        return nullptr;
    }

    return reinterpret_cast<const std::uint8_t*>(
        header) + sizeof(UDPHeader);
}
