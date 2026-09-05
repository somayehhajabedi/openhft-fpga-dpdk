#include "strategy/microstructure_strategy.hpp"

#include "strategy/market_feature_extractor.hpp"

/*
 * Evaluate the latest top-of-book state and generate an OrderIntent
 * when the microstructure model indicates directional pressure.
 *
 * Buy signal:
 *   - bid-side imbalance exceeds the configured threshold
 *   - microprice is above midpoint
 *
 * Sell signal:
 *   - bid-side imbalance falls below the configured threshold
 *   - microprice is below midpoint
 *
 * Otherwise no trading action is generated.
 */
std::optional<OrderIntent>
MicrostructureStrategy::onMarketData(
    const MarketDataEvent& event)
{
    const auto features =
        MarketFeatureExtractor::extract(
            marketView_);

    if (!features.has_value())
    {
        return std::nullopt;
    }

    const MarketFeatures& market =
        *features;

    if (
        market.imbalance >= buyImbalanceThreshold_ &&
        market.microPrice > market.midPrice)
    {
        return OrderIntent{
            .accountId = accountId_,
            .side = Side::Buy,
            .symbol = event.symbol,
            .price = market.bestAskPrice,
            .quantity = quantity_
        };
    }

    if (
        market.imbalance <= sellImbalanceThreshold_ &&
        market.microPrice < market.midPrice)
    {
        return OrderIntent{
            .accountId = accountId_,
            .side = Side::Sell,
            .symbol = event.symbol,
            .price = market.bestBidPrice,
            .quantity = quantity_
        };
    }

    return std::nullopt;
}


MicrostructureStrategy::MicrostructureStrategy(
    const MarketView& marketView,
    AccountId accountId,
    Quantity quantity,
    double buyImbalanceThreshold,
    double sellImbalanceThreshold)
    :
    marketView_(marketView),
    accountId_(accountId),
    quantity_(quantity),
    buyImbalanceThreshold_(
        buyImbalanceThreshold),
    sellImbalanceThreshold_(
        sellImbalanceThreshold)
{
}