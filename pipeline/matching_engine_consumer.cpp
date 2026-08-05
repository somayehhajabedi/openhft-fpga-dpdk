
#include "pipeline/matching_engine_consumer.hpp"

#include "orderbook/software/matching_engine.hpp"

MatchingEngineConsumer::MatchingEngineConsumer(
    MatchingEngine& engine)
    :
    engine_(engine)
{
}

void MatchingEngineConsumer::consume(
    const MarketDataEvent& event)
{
    static_cast<void>(
        engine_.process(event));
}