#pragma once

#include "../orderbook/software/trade.hpp"

class TradePublisher
{
public:
    void publish(const Trade& trade);
};