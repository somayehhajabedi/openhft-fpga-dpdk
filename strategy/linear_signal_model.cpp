#include "strategy/linear_signal_model.hpp"

/*
 * Construct a linear signal model using externally supplied weights.
 *
 * Keeping weights configurable allows model parameters to be calibrated
 * offline without changing the live inference implementation.
 */
LinearSignalModel::LinearSignalModel(
    ModelWeights weights)
    :
    weights_(weights)
{
}


/*
 * Produce a directional signal score from Level-1 market features.
 *
 * Raw market values use different numerical scales, so each feature is
 * converted into a dimensionless directional signal before applying
 * the configured model weights.
 */
double LinearSignalModel::predict(
    const MarketFeatures& features) const noexcept
{
    /*
     * Convert imbalance from [0, 1] into approximately [-1, 1].
     *
     * 0.5 ->  0.0 : balanced book
     * 1.0 -> +1.0 : maximum bid-side imbalance
     * 0.0 -> -1.0 : maximum ask-side imbalance
     */
    const double imbalanceSignal =
        (features.imbalance - 0.5) * 2.0;

    /*
     * Express microprice displacement relative to the current spread.
     *
     * This makes the feature dimensionless and comparable across
     * instruments with different absolute price levels.
     */
    double microPriceSignal = 0.0;

    if (features.spread > 0.0)
    {
        microPriceSignal =
            (features.microPrice -
             features.midPrice) /
            features.spread;
    }

    /*
     * Wider spreads generally make immediate trading less attractive.
     *
     * This initial model uses the raw spread as a penalty input.
     * A production model would typically normalize this feature using
     * historical statistics or instrument-specific scaling.
     */
    const double spreadSignal =
        features.spread;

    return
        weights_.imbalance *
            imbalanceSignal
        +
        weights_.microPrice *
            microPriceSignal
        +
        weights_.spread *
            spreadSignal;
}