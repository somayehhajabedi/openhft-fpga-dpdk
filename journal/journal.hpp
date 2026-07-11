#pragma once

#include "../orderbook/software/trade.hpp"
#include "../events/i_trade_listener.hpp"

class Journal : public ITradeListener
{
public:
    void write(const Trade& trade);
    void onTrade(const Trade& trade) override;
    
};