#pragma once

#include "strategy/i_strategy.hpp"
#include "strategy/linear_signal_model.hpp"
#include "strategy/market_view.hpp"

/*
 * MicrostructureStrategy
 *
 * Generates trading decisions from Level-1 market microstructure
 * features scored by an injected linear signal model.
 *
 * The strategy is responsible for:
 *   - extracting the latest market features
 *   - obtaining a directional model score
 *   - converting that score into Buy / Sell / NoAction
 *
 * Model construction and parameter calibration remain external,
 * keeping model inference separate from trading decision logic.
 */
class MicrostructureStrategy final : public IStrategy
{
public:
    MicrostructureStrategy(
        const MarketView& marketView,
        const LinearSignalModel& signalModel,
        AccountId accountId,
        Quantity quantity,
        double buyScoreThreshold,
        double sellScoreThreshold);

    [[nodiscard]]
    std::optional<OrderIntent> onMarketData(
        const MarketDataEvent& event) override;

private:
    const MarketView& marketView_;
    const LinearSignalModel& signalModel_;

    AccountId accountId_{};
    Quantity quantity_{};

    double buyScoreThreshold_{};
    double sellScoreThreshold_{};
};