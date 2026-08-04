#include "pipeline/market_data_pipeline.hpp"

#include <chrono>

MarketDataPipeline::MarketDataPipeline(
    EventConsumer& consumer)
    :
    queue_{},
    dispatcher_(queue_, consumer)
{
}

MarketDataPipeline::~MarketDataPipeline()
{
    stop();
}

void MarketDataPipeline::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    worker_ =
        std::thread(
            &MarketDataPipeline::processingLoop,
            this);
}

void MarketDataPipeline::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    if (worker_.joinable())
    {
        worker_.join();
    }
}

bool MarketDataPipeline::submit(
    const MarketDataEvent& event)
{
    return queue_.tryPush(event);
}

void MarketDataPipeline::processingLoop()
{
    while (running_.load(
        std::memory_order_acquire))
    {
        static_cast<void>(
            dispatcher_.dispatch());
    }

    // Drain events that were queued before shutdown.
    static_cast<void>(
        dispatcher_.dispatch());
}