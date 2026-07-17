#include "order_executed_mapper.hpp"

#include "../messages/order_executed_parser.hpp"

OrderExecuted
OrderExecutedMapper::fromWire(
    const OrderExecutedWireMessage* message)
{
    OrderExecuted result;

    result.orderReferenceNumber =
        OrderExecutedParser::orderReferenceNumber(message);

    result.executedShares =
        OrderExecutedParser::executedShares(message);

    result.matchNumber =
        OrderExecutedParser::matchNumber(message);

    return result;
}