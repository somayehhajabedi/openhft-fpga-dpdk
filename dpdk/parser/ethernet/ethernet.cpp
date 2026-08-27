
#include "ethernet.hpp"

#include "common/endian.hpp"

const EthernetHeader*
EthernetParser::parse(
    std::span<const std::uint8_t> data)
{
    if (data.size() < sizeof(EthernetHeader))
    {
        return nullptr;
    }

    return reinterpret_cast<const EthernetHeader*>(
        data.data());
}

const EthernetHeader*
EthernetParser::parse(
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

std::uint16_t
EthernetParser::etherType(
    const EthernetHeader* header)
{
    if (!header)
    {
        return 0;
    }

    return fromBigEndian(
        header->ether_type);
}

const std::uint8_t*
EthernetParser::payload(
    const EthernetHeader* header)
{
    if (!header)
    {
        return nullptr;
    }

    return reinterpret_cast<const std::uint8_t*>(
        header) + sizeof(EthernetHeader);
}