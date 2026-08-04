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
    using namespace std::chrono_literals;

    while (running_)
    {
        dispatcher_.dispatch();

        std::this_thread::sleep_for(1us);
    }

    // Drain remaining events before exiting.
    dispatcher_.dispatch();
}