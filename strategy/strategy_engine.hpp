#pragma once

#include "strategy/i_strategy.hpp"

#include <optional>

class StrategyEngine
{
public:
    explicit StrategyEngine(
        IStrategy& strategy);

    [[nodiscard]]
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event);

private:
    IStrategy& strategy_;
};
