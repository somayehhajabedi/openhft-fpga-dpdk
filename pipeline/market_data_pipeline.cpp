#include "pipeline/market_data_pipeline.hpp"

#include "common/thread_affinity.hpp"

#include <atomic>

MarketDataPipeline::MarketDataPipeline(
    EventConsumer& consumer,
    std::optional<std::size_t> workerCpu)
    :
    queue_{},
    dispatcher_(queue_, consumer),
    workerCpu_(workerCpu)
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

    processedCount_.store(
        0,
        std::memory_order_relaxed);

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

std::size_t MarketDataPipeline::processedCount() const noexcept
{
    return processedCount_.load(
        std::memory_order_acquire);
}

void MarketDataPipeline::processingLoop()
{
    if (workerCpu_.has_value())
    {
        static_cast<void>(
            pinCurrentThreadToCpu(
                workerCpu_.value()));
    }

    while (running_.load(
        std::memory_order_acquire))
    {
        const std::size_t processed =
            dispatcher_.dispatch();

        if (processed > 0)
        {
            processedCount_.fetch_add(
                processed,
                std::memory_order_release);
        }
    }

    // Drain events that were queued before shutdown.
    const std::size_t processed =
        dispatcher_.dispatch();

    if (processed > 0)
    {
        processedCount_.fetch_add(
            processed,
            std::memory_order_release);
    }
}