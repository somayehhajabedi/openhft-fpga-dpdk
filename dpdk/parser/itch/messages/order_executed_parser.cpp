#include "order_executed_parser.hpp"

#include <endian.h>

const OrderExecutedWireMessage*
OrderExecutedParser::parse(
    const std::uint8_t* buffer,
    std::size_t length)
{
    if (buffer == nullptr)
        return nullptr;

    if (length < sizeof(OrderExecutedWireMessage))
        return nullptr;

    return reinterpret_cast<const OrderExecutedWireMessage*>(buffer);
}

std::uint64_t
OrderExecutedParser::orderReferenceNumber(
    const OrderExecutedWireMessage* message)
{
    return message
        ? be64toh(message->order_reference_number)
        : 0;
}

std::uint32_t
OrderExecutedParser::executedShares(
    const OrderExecutedWireMessage* message)
{
    return message
        ? be32toh(message->executed_shares)
        : 0;
}

std::uint64_t
OrderExecutedParser::matchNumber(
    const OrderExecutedWireMessage* message)
{
    return message
        ? be64toh(message->match_number)
        : 0;
}