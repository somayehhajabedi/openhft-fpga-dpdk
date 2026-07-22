#pragma once

#include "../messages/order_cancel.hpp"
#include "order_cancel.hpp"

class OrderCancelMapper
{
public:
    static OrderCancel fromWire(
        const OrderCancelWireMessage* message);
};