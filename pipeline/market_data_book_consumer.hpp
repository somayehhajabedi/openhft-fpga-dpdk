#pragma once

#include "pipeline/event_consumer.hpp"
#include "orderbook/software/array_order_book.hpp"
#include "orderbook/software/order_pool.hpp"

#include <cstddef>

class MarketDataBookConsumer final : public EventConsumer
{
public:
    static constexpr std::size_t DefaultOrderCapacity = 4096;

    explicit MarketDataBookConsumer(
        ArrayOrderBook& book);

    void consume(
        const MarketDataEvent& event) override;

private:
    void releaseIfOwned(
        Order* order);

    ArrayOrderBook& book_;

    OrderPool orderPool_;
};
