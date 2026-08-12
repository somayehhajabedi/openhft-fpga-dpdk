#pragma once

#include "pipeline/market_data_event.hpp"

class MarketDataEventSink
{
public:
    virtual ~MarketDataEventSink() = default;

    virtual bool submit(
        const MarketDataEvent& event) = 0;
};
