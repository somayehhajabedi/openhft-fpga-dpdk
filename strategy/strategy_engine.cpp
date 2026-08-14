#include "strategy/strategy_engine.hpp"

StrategyEngine::StrategyEngine(
    IStrategy& strategy)
    :
    strategy_(strategy)
{
}

std::optional<OrderIntent>
StrategyEngine::onMarketData(
    const MarketDataEvent& event)
{
    return strategy_.onMarketData(event);
}
