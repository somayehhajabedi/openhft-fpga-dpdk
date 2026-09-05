#pragma once

#include "strategy/market_features.hpp"
#include "strategy/market_view.hpp"

#include <optional>

/*
 * MarketFeatureExtractor
 *
 * Converts the current Level-1 order-book state exposed through MarketView
 * into a compact set of market microstructure features.
 *
 * The extractor is intentionally stateless. It reads the current best bid
 * and best ask and derives features such as spread, midpoint, imbalance,
 * and microprice.
 *
 * Feature extraction is kept outside the strategy so that multiple
 * strategies or models can reuse the same feature definitions.
 */
class MarketFeatureExtractor
{
public:
    [[nodiscard]]
    static std::optional<MarketFeatures> extract(
        const MarketView& marketView);
};