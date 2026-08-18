#pragma once

#include "strategy/i_strategy.hpp"
#include "strategy/market_view.hpp"

class SimpleThresholdStrategy final : public IStrategy
{
public:
    SimpleThresholdStrategy(
        const MarketView& marketView,
        AccountId accountId,
        Price buyBelowPrice,
        Quantity quantity);

    [[nodiscard]]
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) override;

private:
    const MarketView& marketView_;

    AccountId accountId_;
    Price buyBelowPrice_;
    Quantity quantity_;
};
