#pragma once

#include "orderbook/software/common/types.hpp"

/*
 * MarketFeatures
 *
 * Represents a snapshot of derived Level-1 market microstructure features.
 *
 * These features are calculated from the current best bid and best ask
 * and are consumed by trading strategies or predictive models.
 *
 * Keeping the derived features separate from the order book allows
 * strategies to operate on a compact and well-defined market representation.
 */
struct MarketFeatures
{
    Price bestBidPrice{};
    Price bestAskPrice{};

    Quantity bestBidQuantity{};
    Quantity bestAskQuantity{};

    // Distance between the best ask and best bid.
    double spread{};

    // Simple midpoint between the best bid and best ask.
    double midPrice{};

    // Relative bid-side liquidity:
    //
    //     bidQty
    // ----------------
    // bidQty + askQty
    //
    // Values closer to 1 indicate stronger bid-side pressure,
    // while values closer to 0 indicate stronger ask-side pressure.
    double imbalance{};

    // Quantity-weighted estimate of the short-term fair price.
    //
    // askPrice * bidQty + bidPrice * askQty
    // --------------------------------------
    //             bidQty + askQty
    double microPrice{};
};