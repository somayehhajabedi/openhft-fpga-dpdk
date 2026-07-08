#pragma once

#include "common/types.hpp"

struct Trade
{
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
};
