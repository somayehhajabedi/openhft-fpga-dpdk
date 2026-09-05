#pragma once

#include "strategy/i_strategy.hpp"
#include "strategy/market_view.hpp"

class MicrostructureStrategy final : public IStrategy
{
public:
    /*
     * MicrostructureStrategy
     *
     * Generates trading decisions from Level-1 market microstructure
     * features such as order-book imbalance and microprice.
     *
     * The strategy does not calculate features directly. It relies on
     * MarketFeatureExtractor so that feature construction remains
     * reusable and independent from trading decision logic.
     */
    MicrostructureStrategy(
        const MarketView& marketView,
        AccountId accountId,
        Quantity quantity,
        double buyImbalanceThreshold,
        double sellImbalanceThreshold);

    [[nodiscard]]
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) override;

private:
    const MarketView& marketView_;

    AccountId accountId_{};
    Quantity quantity_{};

    double buyImbalanceThreshold_{};
    double sellImbalanceThreshold_{};
};