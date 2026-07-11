#pragma once

#include "../orderbook/software/trade.hpp"

class ITradeListener
{
public:
    virtual ~ITradeListener() = default;

    virtual void onTrade(const Trade& trade) = 0;
};