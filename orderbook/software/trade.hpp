#pragma once

#include "common/types.hpp"

struct Trade
{
    uint64_t sequence;

    OrderId buy_order_id;
    OrderId sell_order_id;

    Price price;
    Quantity quantity;
};