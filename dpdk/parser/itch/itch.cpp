#include "itch.hpp"

const ITCHHeader* ITCHParser::parse(
    const std::uint8_t* data,
    std::size_t length)
{
    if (data == nullptr)
        return nullptr;

    if (length < sizeof(ITCHHeader))
        return nullptr;

    return reinterpret_cast<const ITCHHeader*>(data);
}

char ITCHParser::messageType(
    const ITCHHeader* header)
{
    return header->message_type;
}

bool ITCHParser::isAddOrder(
    const ITCHHeader* header)
{
    return header &&
           header->message_type == 'A';
}

const std::uint8_t*
ITCHParser::payload(
    const ITCHHeader* header)
{
    if (!header)
        return nullptr;

    return reinterpret_cast<const std::uint8_t*>(header)
           + sizeof(ITCHHeader);
}