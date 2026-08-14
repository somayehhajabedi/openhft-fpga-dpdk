#include "strategy/market_data_strategy_consumer.hpp"

MarketDataStrategyConsumer::MarketDataStrategyConsumer(
    MarketDataBookConsumer& bookConsumer,
    StrategyEngine& strategyEngine,
    OrderIntentSink& intentSink)
    :
    bookConsumer_(bookConsumer),
    strategyEngine_(strategyEngine),
    intentSink_(intentSink)
{
}

void MarketDataStrategyConsumer::consume(
    const MarketDataEvent& event)
{
    // First update our local view of the market.
    bookConsumer_.consume(event);

    // Then let the strategy observe the new market event.
    const auto intent =
        strategyEngine_.onMarketData(event);

    if (!intent.has_value())
    {
        return;
    }

    static_cast<void>(
        intentSink_.submit(*intent));
}
