#pragma once

#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_book_consumer.hpp"
#include "strategy/order_intent_sink.hpp"
#include "strategy/strategy_engine.hpp"

class MarketDataStrategyConsumer final : public EventConsumer
{
public:
    MarketDataStrategyConsumer(
        MarketDataBookConsumer& bookConsumer,
        StrategyEngine& strategyEngine,
        OrderIntentSink& intentSink);

    void consume(
        const MarketDataEvent& event) override;

private:
    MarketDataBookConsumer& bookConsumer_;
    StrategyEngine& strategyEngine_;
    OrderIntentSink& intentSink_;
};
