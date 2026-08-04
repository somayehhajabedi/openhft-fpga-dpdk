#include "pipeline/market_data_pipeline.hpp"

MarketDataPipeline::MarketDataPipeline(
    EventConsumer& consumer)
    :
    queue_{},
    dispatcher_(queue_, consumer)
{
}

void MarketDataPipeline::start()
{
    dispatcher_.dispatch();
}

void MarketDataPipeline::stop()
{
}

bool MarketDataPipeline::submit(
    const MarketDataEvent& event)
{
    return queue_.tryPush(event);
}