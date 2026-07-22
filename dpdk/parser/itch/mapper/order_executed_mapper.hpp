#pragma once

#include "../messages/order_executed.hpp"
#include "order_executed.hpp"

class OrderExecutedMapper
{
public:
    static OrderExecuted fromWire(
        const OrderExecutedWireMessage* message);
};