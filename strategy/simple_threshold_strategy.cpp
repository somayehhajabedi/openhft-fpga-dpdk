#include "strategy/simple_threshold_strategy.hpp"

SimpleThresholdStrategy::SimpleThresholdStrategy(
    AccountId accountId,
    Price buyBelowPrice,
    Quantity quantity)
    :
    accountId_(accountId),
    buyBelowPrice_(buyBelowPrice),
    quantity_(quantity)
{
}

std::optional<OrderIntent>
SimpleThresholdStrategy::onMarketData(
    const MarketDataEvent& event)
{
    if (event.type != MarketDataEventType::AddOrder)
    {
        return std::nullopt;
    }

    if (event.side != Side::Sell)
    {
        return std::nullopt;
    }

    if (event.price > buyBelowPrice_)
    {
        return std::nullopt;
    }

    return OrderIntent{
        .accountId = accountId_,
        .side = Side::Buy,
        .symbol = event.symbol,
        .price = event.price,
        .quantity = quantity_
    };


}
