#include "ethernet.hpp"
#include <arpa/inet.h>

const EthernetHeader*
EthernetParser::parse(const std::uint8_t* data,
                      std::size_t length)
{
    if (!data || length < sizeof(EthernetHeader))
        return nullptr;

    return reinterpret_cast<const EthernetHeader*>(data);
}

std::uint16_t EthernetParser::etherType(const EthernetHeader* header)
{
    if (!header)
        return 0;

    return ntohs(header->ether_type);
}


const std::uint8_t*
EthernetParser::payload(const EthernetHeader* header)
{
    if (!header)
        return nullptr;

    return reinterpret_cast<const std::uint8_t*>(header)
           + sizeof(EthernetHeader);
}


