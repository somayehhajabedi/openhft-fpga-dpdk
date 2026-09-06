/*
 * LinearSignalModel Tests
 *
 * Verifies that the linear strategy model converts normalized
 * market microstructure features into the expected directional
 * signal score.
 *
 * The tests cover bullish, bearish, and balanced market states
 * using deterministic model weights.
 */

#include <gtest/gtest.h>

#include "strategy/linear_signal_model.hpp"


TEST(
    LinearSignalModelTest,
    ProducesPositiveScoreForBullishMarket)
{
    const LinearSignalModel model(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MarketFeatures features{};

    features.imbalance = 0.75;
    features.midPrice = 100.5;
    features.microPrice = 100.75;
    features.spread = 1.0;

    /*
     * imbalanceSignal:
     *
     * (0.75 - 0.5) * 2 = 0.5
     *
     * microPriceSignal:
     *
     * (100.75 - 100.5) / 1 = 0.25
     *
     * score:
     *
     * 0.6 * 0.5 + 0.4 * 0.25 = 0.4
     */
    const double score =
        model.predict(features);

    EXPECT_DOUBLE_EQ(
        score,
        0.4);
}


TEST(
    LinearSignalModelTest,
    ProducesNegativeScoreForBearishMarket)
{
    const LinearSignalModel model(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MarketFeatures features{};

    features.imbalance = 0.25;
    features.midPrice = 100.5;
    features.microPrice = 100.25;
    features.spread = 1.0;

    /*
     * imbalanceSignal = -0.5
     * microPriceSignal = -0.25
     *
     * score:
     *
     * 0.6 * -0.5 + 0.4 * -0.25 = -0.4
     */
    const double score =
        model.predict(features);

    EXPECT_DOUBLE_EQ(
        score,
        -0.4);
}


TEST(
    LinearSignalModelTest,
    ProducesZeroScoreForBalancedMarket)
{
    const LinearSignalModel model(
        LinearSignalModel::ModelWeights{
            .imbalance = 0.6,
            .microPrice = 0.4,
            .spread = 0.0
        });

    MarketFeatures features{};

    features.imbalance = 0.5;
    features.midPrice = 100.5;
    features.microPrice = 100.5;
    features.spread = 1.0;

    const double score =
        model.predict(features);

    EXPECT_DOUBLE_EQ(
        score,
        0.0);
}