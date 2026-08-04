#pragma once

/*
 * Matching Engine Consumer
 * ========================
 *
 * Purpose
 * -------
 * Receives normalized MarketDataEvent objects from the Dispatcher
 * and forwards them to the Matching Engine.
 *
 * Responsibilities
 * ----------------
 * - Consume MarketDataEvent objects.
 * - Dispatch events based on their type.
 * - Preserve FIFO ordering.
 *
 * Current Implementation
 * ----------------------
 * Infrastructure only.
 * Event forwarding will be implemented in the next milestone.
 *
 * Future Work
 * -----------
 * - AddOrder
 * - CancelOrder
 * - ReplaceOrder
 * - ExecuteOrder
 * - DeleteOrder
 */

#include "pipeline/event_consumer.hpp"

class MatchingEngine;

class MatchingEngineConsumer final
    : public EventConsumer
{
public:

    explicit MatchingEngineConsumer(
        MatchingEngine& engine);

    void consume(
        const MarketDataEvent& event) override;

private:

    MatchingEngine& engine_;
};
