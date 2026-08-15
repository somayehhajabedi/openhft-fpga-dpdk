#pragma once

#include "orderbook/software/common/types.hpp"

struct OrderIntent
{
    AccountId accountId{};
    Side side{Side::Buy};
    Symbol symbol{};
    Price price{};
    Quantity quantity{};
};
