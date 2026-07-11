#pragma once

#include "../orderbook/software/trade.hpp"
#include "../events/i_trade_listener.hpp"

class TradePublisher : public ITradeListener
{
public:
    void publish(const Trade& trade);
    void onTrade(const Trade& trade) override;
};