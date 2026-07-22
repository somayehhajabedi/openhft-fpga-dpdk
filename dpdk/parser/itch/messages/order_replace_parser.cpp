#include "order_replace_parser.hpp"

#include <endian.h>

const OrderReplaceWireMessage*
OrderReplaceParser::parse(
    const std::uint8_t* buffer,
    std::size_t length)
{
    if (buffer == nullptr)
        return nullptr;

    if (length < sizeof(OrderReplaceWireMessage))
        return nullptr;

    return reinterpret_cast<const OrderReplaceWireMessage*>(buffer);
}

std::uint64_t
OrderReplaceParser::originalOrderReferenceNumber(
    const OrderReplaceWireMessage* message)
{
    return message
        ? be64toh(message->original_order_reference)
        : 0;
}

std::uint64_t
OrderReplaceParser::newOrderReferenceNumber(
    const OrderReplaceWireMessage* message)
{
    return message
        ? be64toh(message->new_order_reference)
        : 0;
}

std::uint32_t
OrderReplaceParser::shares(
    const OrderReplaceWireMessage* message)
{
    return message
        ? be32toh(message->shares)
        : 0;
}

std::uint32_t
OrderReplaceParser::price(
    const OrderReplaceWireMessage* message)
{
    return message
        ? be32toh(message->price)
        : 0;
}