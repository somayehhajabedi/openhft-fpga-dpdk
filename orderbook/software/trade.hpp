#pragma once

#include "common/types.hpp"

struct Trade
{
    uint64_t sequence;

    OrderId buy_order_id;
    OrderId sell_order_id;
    
    AccountId buy_account_id;
    AccountId sell_account_id;

    Price price;
    Quantity quantity;
};