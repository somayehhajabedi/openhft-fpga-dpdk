#pragma once

#include "../orderbook/software/trade.hpp"
class Journal;

class TradePublisher;

class EventDispatcher
{
public:
    EventDispatcher(TradePublisher& trade_publisher,
                Journal& journal);

    void publish(const Trade& trade);

private:
    TradePublisher& trade_publisher_;
    Journal& journal_;
};