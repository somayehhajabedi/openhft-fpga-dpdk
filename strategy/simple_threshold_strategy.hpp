#pragma once

#include "strategy/i_strategy.hpp"

class SimpleThresholdStrategy final : public IStrategy
{
public:
    SimpleThresholdStrategy(
        AccountId accountId,
        Price buyBelowPrice,
        Quantity quantity);

    [[nodiscard]]
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) override;

private:
    AccountId accountId_;
    Price buyBelowPrice_;
    Quantity quantity_;
};
