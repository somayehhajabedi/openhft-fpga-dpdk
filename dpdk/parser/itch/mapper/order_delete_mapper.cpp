#include "order_delete_mapper.hpp"

#include "../messages/order_delete_parser.hpp"

OrderDelete
OrderDeleteMapper::fromWire(
    const OrderDeleteWireMessage* message)
{
    OrderDelete result;

    result.orderReferenceNumber =
        OrderDeleteParser::orderReferenceNumber(message);

    return result;
}