#include "add_order_mapper.hpp"

AddOrder AddOrderMapper::fromWire(
    const AddOrderWireMessage* message)
{
    AddOrder order{};

    order.stockLocate =
        AddOrderParser::stockLocate(message);

    order.trackingNumber =
        AddOrderParser::trackingNumber(message);

    order.shares =
        AddOrderParser::shares(message);

    order.price =
        AddOrderParser::price(message);

    order.orderReferenceNumber =
        AddOrderParser::orderReferenceNumber(message);

    order.isBuy =
        AddOrderParser::buySellIndicator(message) == 'B';

    return order;
}
