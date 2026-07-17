#include "order_replace_mapper.hpp"

#include "../messages/order_replace_parser.hpp"

OrderReplace
OrderReplaceMapper::fromWire(
    const OrderReplaceWireMessage* message)
{
    OrderReplace result;

    result.originalOrderReferenceNumber =
        OrderReplaceParser::originalOrderReferenceNumber(message);

    result.newOrderReferenceNumber =
        OrderReplaceParser::newOrderReferenceNumber(message);

    result.shares =
        OrderReplaceParser::shares(message);

    result.price =
        OrderReplaceParser::price(message);

    return result;
}