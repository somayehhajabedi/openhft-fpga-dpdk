#pragma once

#include "../messages/order_delete.hpp"
#include "order_delete.hpp"

class OrderDeleteMapper
{
public:
    static OrderDelete fromWire(
        const OrderDeleteWireMessage* message);
};