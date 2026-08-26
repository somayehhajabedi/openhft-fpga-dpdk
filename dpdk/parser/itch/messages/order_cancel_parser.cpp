#include "order_cancel_parser.hpp"
#include "common/endian.hpp"

const OrderCancelWireMessage*
OrderCancelParser::parse(
    const std::uint8_t* buffer,
    std::size_t length)
{
    if (buffer == nullptr)
        return nullptr;

    if (length < sizeof(OrderCancelWireMessage))
        return nullptr;

    return reinterpret_cast<const OrderCancelWireMessage*>(buffer);
}

std::uint64_t
OrderCancelParser::orderReferenceNumber(
    const OrderCancelWireMessage* message)
{
    return message
        ? fromBigEndian(
              message->order_reference_number)
        : 0;
}

std::uint32_t
OrderCancelParser::cancelledShares(
    const OrderCancelWireMessage* message)
{
    return message
        ? fromBigEndian(
              message->cancelled_shares)
        : 0;
}
