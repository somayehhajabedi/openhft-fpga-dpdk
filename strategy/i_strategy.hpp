#pragma once

#include "pipeline/market_data_event.hpp"
#include "models/order_intent.hpp"

#include <optional>

class IStrategy
{
public:
    virtual ~IStrategy() = default;

    [[nodiscard]]
    virtual std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) = 0;
};
