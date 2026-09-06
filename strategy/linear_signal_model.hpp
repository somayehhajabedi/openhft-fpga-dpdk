#pragma once

#include "strategy/market_features.hpp"

/*
 * LinearSignalModel
 *
 * Converts a set of normalized market microstructure features into
 * a single directional signal score.
 *
 * The model performs lightweight inference suitable for the trading
 * hot path. Model training and weight calibration are expected to
 * happen offline using historical market data.
 *
 * Positive scores represent bullish pressure, while negative scores
 * represent bearish pressure.
 */
class LinearSignalModel
{
public:
    /*
     * ModelWeights
     *
     * Contains the configurable contribution of each market feature
     * to the final signal score.
     */
    struct ModelWeights
    {
        double imbalance{};
        double microPrice{};
        double spread{};
    };

    explicit LinearSignalModel(
        ModelWeights weights);

    [[nodiscard]]
    double predict(
        const MarketFeatures& features) const noexcept;

private:
    ModelWeights weights_;
};