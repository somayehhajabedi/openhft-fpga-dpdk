
#include "pipeline/matching_engine_consumer.hpp"

#include "orderbook/software/matching_engine.hpp"

/*
 * Matching Engine Consumer
 *
 * Receives normalized MarketDataEvent objects from the
 * Dispatcher.
 *
 * Event translation into Matching Engine operations will
 * be implemented incrementally.
 */

MatchingEngineConsumer::MatchingEngineConsumer(
    MatchingEngine& engine)
    :
    engine_(engine)
{
}

void MatchingEngineConsumer::consume(
    const MarketDataEvent& event)
{
    switch (event.type)
    {
        case MarketDataEventType::AddOrder:
            break;

        case MarketDataEventType::CancelOrder:
            break;

        case MarketDataEventType::DeleteOrder:
            break;

        case MarketDataEventType::ExecuteOrder:
            break;

        case MarketDataEventType::ReplaceOrder:
            break;
    }

    (void)engine_;
}