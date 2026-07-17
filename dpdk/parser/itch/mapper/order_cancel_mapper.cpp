#include "order_cancel_mapper.hpp"

#include "../messages/order_cancel_parser.hpp"

OrderCancel
OrderCancelMapper::fromWire(
    const OrderCancelWireMessage* message)
{
    OrderCancel orderCancel;

    orderCancel.orderReferenceNumber =
        OrderCancelParser::orderReferenceNumber(message);

    orderCancel.cancelledShares =
        OrderCancelParser::cancelledShares(message);

    return orderCancel;
}