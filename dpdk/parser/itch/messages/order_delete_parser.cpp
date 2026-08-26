#include "order_delete_parser.hpp"
#include "common/endian.hpp"

const OrderDeleteWireMessage*
OrderDeleteParser::parse(
    const std::uint8_t* buffer,
    std::size_t length)
{
    if (buffer == nullptr)
        return nullptr;

    if (length < sizeof(OrderDeleteWireMessage))
        return nullptr;

    return reinterpret_cast<const OrderDeleteWireMessage*>(buffer);
}

std::uint64_t
OrderDeleteParser::orderReferenceNumber(
    const OrderDeleteWireMessage* message)
{
    return message
        ? fromBigEndian(
              message->order_reference_number)
        : 0;
}
