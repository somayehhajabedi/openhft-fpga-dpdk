#pragma once

#include "array_order_book.hpp"
#include "../../sequencer/sequencer.hpp"
#include "../../dispatcher/event_dispatcher.hpp"
#include "order.hpp"
#include "trade.hpp"

class MatchingEngine
{
public:
    explicit MatchingEngine(EventDispatcher& dispatcher);

    void process(Order* order);

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
