#pragma once

#include "orderbook/software/price_level.hpp"

class MarketView
{
public:
    virtual ~MarketView() = default;

    [[nodiscard]]
    virtual const PriceLevel* bestBid() const = 0;

    [[nodiscard]]
    virtual const PriceLevel* bestAsk() const = 0;
};
