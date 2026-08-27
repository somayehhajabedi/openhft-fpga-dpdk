
#include "ipv4.hpp"

#include "common/endian.hpp"

const IPv4Header*
IPv4Parser::parse(
    std::span<const std::uint8_t> data)
{
    if (data.size() < sizeof(IPv4Header))
    {
        return nullptr;
    }

    const auto* header =
        reinterpret_cast<const IPv4Header*>(
            data.data());

    if (version(header) != 4)
    {
        return nullptr;
    }

    const std::uint8_t headerLength =
        headerLengthBytes(header);

    if (headerLength < sizeof(IPv4Header))
    {
        return nullptr;
    }

    if (data.size() < headerLength)
    {
        return nullptr;
    }

    return header;
}

const IPv4Header*
IPv4Parser::parse(
    const std::uint8_t* data,
    std::size_t length)
{
    if (data == nullptr)
    {
        return nullptr;
    }

    return parse(
        std::span<const std::uint8_t>{
            data,
            length});
}

std::uint8_t
IPv4Parser::version(
    const IPv4Header* header)
{
    if (!header)
    {
        return 0;
    }

    return header->version_ihl >> 4;
}

std::uint8_t
IPv4Parser::headerLengthBytes(
    const IPv4Header* header)
{
    if (!header)
    {
        return 0;
    }

    const std::uint8_t ihl =
        header->version_ihl & 0x0F;

    return static_cast<std::uint8_t>(
        ihl * 4);
}

std::uint16_t
IPv4Parser::totalLength(
    const IPv4Header* header)
{
    if (!header)
    {
        return 0;
    }

    return fromBigEndian(
        header->total_length);
}

const std::uint8_t*
IPv4Parser::payload(
    const IPv4Header* header)
{
    if (!header)
    {
        return nullptr;
    }

    return reinterpret_cast<const std::uint8_t*>(
        header) + headerLengthBytes(header);
}

std::size_t
IPv4Parser::payloadLength(
    const IPv4Header* header)
{
    if (!header)
    {
        return 0;
    }

    const std::uint16_t totalLengthValue =
        totalLength(header);

    const std::uint8_t headerLength =
        headerLengthBytes(header);

    if (totalLengthValue < headerLength)
    {
        return 0;
    }

    return totalLengthValue - headerLength;
}