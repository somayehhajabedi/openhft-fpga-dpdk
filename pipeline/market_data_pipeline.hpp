/*
 * Market Data Pipeline
 * --------------------
 * 
 * *
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
 *
 * Coordinates the complete market-data processing pipeline.
 *
 * Planned data flow:
 *
 *   DPDK Receiver
 *          │
 *          ▼
 *   Ethernet Parser
 *          ▼
 *      IPv4 Parser
 *          ▼
 *       UDP Parser
 *          ▼
 *      ITCH Parser
 *          ▼
 *      ITCH Mapper
 *          ▼
 *   MarketDataEvent
 *          ▼
 *    SPSC Ring Buffer
 *          ▼
 *      Dispatcher
 *          ▼
 *   Matching Engine
 *
 * The pipeline owns the communication queue between the producer
 * and consumer stages.
 */

#include "common/spsc_ring_buffer.hpp"
#include "pipeline/dispatcher.hpp"
#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/pipeline.hpp"

#include <atomic>
#include <cstddef>
#include <thread>

class MarketDataPipeline final : public Pipeline
{
public:

    explicit MarketDataPipeline(
        EventConsumer& consumer);

    ~MarketDataPipeline() override;

    void start() override;

    void stop() override;

    bool submit(
        const MarketDataEvent& event);

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
};
