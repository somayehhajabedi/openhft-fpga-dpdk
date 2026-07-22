#pragma once

#include "../messages/order_replace.hpp"
#include "order_replace.hpp"

class OrderReplaceMapper
{
public:
    static OrderReplace fromWire(
        const OrderReplaceWireMessage* message);
};