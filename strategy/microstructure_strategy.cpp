#include "strategy/microstructure_strategy.hpp"

#include "strategy/market_feature_extractor.hpp"

/*
 * MicrostructureStrategy
 *
 * Evaluates the latest top-of-book state, obtains a directional
 * score from the injected signal model, and converts that score
 * into a trading decision.
 *
 * The strategy owns only decision logic. Feature extraction and
 * model inference remain separate so they can be reused and tested
 * independently.
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

    const double score =
        signalModel_.predict(market);

    if (score >= buyScoreThreshold_)
    {
        return OrderIntent{
            .accountId = accountId_,
            .side = Side::Buy,
            .symbol = event.symbol,
            .price = market.bestAskPrice,
            .quantity = quantity_
        };
    }

    if (score <= sellScoreThreshold_)
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
    const LinearSignalModel& signalModel,
    AccountId accountId,
    Quantity quantity,
    double buyScoreThreshold,
    double sellScoreThreshold)
    :
    marketView_(marketView),
    signalModel_(signalModel),
    accountId_(accountId),
    quantity_(quantity),
    buyScoreThreshold_(buyScoreThreshold),
    sellScoreThreshold_(sellScoreThreshold)
{
}