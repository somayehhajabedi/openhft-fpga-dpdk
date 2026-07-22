#pragma once

#include "dpdk/parser/itch/messages/add_order.hpp"
#include "models/add_order.hpp"

class AddOrderMapper
{
public:

    static AddOrder fromWire(
        const AddOrderWireMessage* message);

};