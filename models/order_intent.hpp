#pragma once

#include "orderbook/software/common/types.hpp"

struct OrderIntent
{
    AccountId accountId{};
    Side side{Side::Buy};
    Price price{};
    Quantity quantity{};
};
