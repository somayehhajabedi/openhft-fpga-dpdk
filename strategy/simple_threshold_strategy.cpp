#include "strategy/simple_threshold_strategy.hpp"

SimpleThresholdStrategy::SimpleThresholdStrategy(
    const MarketView& marketView,
    AccountId accountId,
    Price buyBelowPrice,
    Quantity quantity)
    :
    marketView_(marketView),
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

    const PriceLevel* bestAsk =
        marketView_.bestAsk();

    if (bestAsk == nullptr)
    {
        return std::nullopt;
    }

    if (bestAsk->price > buyBelowPrice_)
    {
        return std::nullopt;
    }

    return OrderIntent{
        .accountId = accountId_,
        .side = Side::Buy,
        .symbol = event.symbol,
        .price = bestAsk->price,
        .quantity = quantity_
    };
}