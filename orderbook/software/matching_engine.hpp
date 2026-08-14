#pragma once

#include <cstddef>

#include "array_order_book.hpp"
#include "order.hpp"
#include "order_pool.hpp"
#include "trade.hpp"

#include "../../dispatcher/event_dispatcher.hpp"
#include "../../sequencer/sequencer.hpp"

class MatchingEngine
{
public:
    static constexpr std::size_t DefaultOrderCapacity = 4096;

    explicit MatchingEngine(
        EventDispatcher& dispatcher);

    // Low-level API used by tests and benchmarks.
    void process(
        Order* order);

    [[nodiscard]]
    bool submitOrder(
        AccountId accountId,
        Side side,
        Price price,
        Quantity quantity);

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

    OrderId nextOrderId_{1};
};
