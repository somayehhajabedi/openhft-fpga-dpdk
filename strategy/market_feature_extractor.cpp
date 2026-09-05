#include "strategy/market_feature_extractor.hpp"

/*
 * Extract Level-1 microstructure features from the current market view.
 *
 * A valid feature snapshot requires both sides of the book and non-zero
 * displayed liquidity. If either condition is missing, no feature snapshot
 * is produced.
 */
std::optional<MarketFeatures>
MarketFeatureExtractor::extract(
    const MarketView& marketView)
{
    const PriceLevel* bestBid =
        marketView.bestBid();

    const PriceLevel* bestAsk =
        marketView.bestAsk();

    // A two-sided market is required for spread, midpoint,
    // imbalance, and microprice calculations.
    if (bestBid == nullptr ||
        bestAsk == nullptr)
    {
        return std::nullopt;
    }

    const double bidPrice =
        static_cast<double>(bestBid->price);

    const double askPrice =
        static_cast<double>(bestAsk->price);

    const double bidQuantity =
        static_cast<double>(
            bestBid->total_quantity);

    const double askQuantity =
        static_cast<double>(
            bestAsk->total_quantity);

    const double totalQuantity =
        bidQuantity + askQuantity;

    // Avoid division by zero when both top-of-book levels
    // contain no displayed quantity.
    if (totalQuantity == 0.0)
    {
        return std::nullopt;
    }

    MarketFeatures features{};

    features.bestBidPrice =
        bestBid->price;

    features.bestAskPrice =
        bestAsk->price;

    features.bestBidQuantity =
        bestBid->total_quantity;

    features.bestAskQuantity =
        bestAsk->total_quantity;

    features.spread =
        askPrice - bidPrice;

    features.midPrice =
        (bidPrice + askPrice) / 2.0;

    features.imbalance =
        bidQuantity / totalQuantity;

    // Microprice shifts the midpoint toward the side with
    // relatively lower available liquidity.
    features.microPrice =
        (
            askPrice * bidQuantity +
            bidPrice * askQuantity
        ) / totalQuantity;

    return features;
}