#pragma once

#include "array_order_book.hpp"
#include "order.hpp"
#include "order_pool.hpp"
#include "trade.hpp"

#include "../../dispatcher/event_dispatcher.hpp"
#include "../../sequencer/sequencer.hpp"
#include "pipeline/market_data_event.hpp"

#include <cstddef>

class MatchingEngine
{
public:
    static constexpr std::size_t DefaultOrderCapacity = 4096;

    explicit MatchingEngine(
        EventDispatcher& dispatcher);

    bool process(
        const MarketDataEvent& event);

    // Legacy API used by Gateway, tests and benchmarks.
    void process(
        Order* order);

private:

    void releaseIfOwned(
        Order* order);

    bool canCross(
        const Order* order) const;

    void executeTrade(
        Order* incoming);

    bool matchOne(
        Order* incoming);

    Trade createTrade(
        const Order* incoming,
        const Order* resting,
        Quantity tradedQuantity);
   

    ArrayOrderBook book_;
    OrderPool orderPool_;
    EventDispatcher& dispatcher_;
    Sequencer sequencer_;
};