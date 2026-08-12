#pragma once

/*
 * Market Data Pipeline
 * --------------------
 *
 * Owns the asynchronous market-data pipeline connecting the
 * producer thread to the Dispatcher through a lock-free
 * Single Producer Single Consumer (SPSC) queue.
 *
 * Responsibilities
 * ----------------
 * - Accept MarketDataEvent objects.
 * - Own the SPSC queue.
 * - Manage the worker thread lifecycle.
 * - Forward queued events to the Dispatcher.
 * - Optionally pin the worker thread to a CPU.
 *
 * Data flow:
 *
 *   Producer
 *      |
 *      v
 *   MarketDataEvent
 *      |
 *      v
 *   SPSC Queue
 *      |
 *      v
 *   Dispatcher
 *      |
 *      v
 *   EventConsumer
 */

#pragma once

#include "common/spsc_ring_buffer.hpp"
#include "pipeline/dispatcher.hpp"
#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/pipeline.hpp"
#include "pipeline/market_data_event_sink.hpp"

#include <atomic>
#include <cstddef>
#include <optional>
#include <thread>

class MarketDataPipeline final
    : public Pipeline,
      public MarketDataEventSink
{
public:
    explicit MarketDataPipeline(
        EventConsumer& consumer,
        std::optional<std::size_t> workerCpu = std::nullopt);

    ~MarketDataPipeline() override;

    void start() override;

    void stop() override;

    bool submit(
         const MarketDataEvent& event) override;


    [[nodiscard]]
    std::size_t processedCount() const noexcept;

private:
    static constexpr std::size_t QueueCapacity = 4096;

    using EventQueue =
        SPSCRingBuffer<
            MarketDataEvent,
            QueueCapacity>;

    void processingLoop();

    EventQueue queue_;

    Dispatcher dispatcher_;

    std::thread worker_;

    std::atomic<bool> running_{false};

    std::atomic<std::size_t> processedCount_{0};

    std::optional<std::size_t> workerCpu_;
};
