#pragma once

#include "array_order_book.hpp"
#include "../../sequencer/sequencer.hpp"
#include "../../dispatcher/event_dispatcher.hpp"
#include "order.hpp"
#include "trade.hpp"
#include "pipeline/market_data_event.hpp"

class MatchingEngine
{
public:
    public:
    explicit MatchingEngine(EventDispatcher& dispatcher);

    bool process(
        const MarketDataEvent& event);

    void process(
        Order* order);

private:
private:
 private:

    
    bool canCross(const Order* order) const;
    void executeTrade(Order* incoming);
    bool matchOne(Order* incoming);

    Trade createTrade(const Order* incoming,
                      const Order* resting,
                      Quantity traded_quantity);

    ArrayOrderBook book_;
    EventDispatcher& dispatcher_;
    Sequencer sequencer_;
};
