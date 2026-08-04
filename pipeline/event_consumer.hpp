#pragma once

#include "pipeline/market_data_event.hpp"

/*
 * Event Consumer
 * ==============
 *
 * Purpose
 * -------
 * Defines the interface used by Dispatcher to forward normalized
 * MarketDataEvent objects to downstream processing components.
 *
 * Implementations may include:
 *
 * - Matching Engine consumer
 * - Test consumer
 * - Recorder
 * - Monitoring adapter
 */
class EventConsumer
{
public:
    virtual ~EventConsumer() = default;

    virtual void consume(
        const MarketDataEvent& event) = 0;
};
