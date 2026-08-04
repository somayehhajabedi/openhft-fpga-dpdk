/*
 * Market Data Pipeline
 * --------------------
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

#pragma once

#include "common/spsc_ring_buffer.hpp"
#include "pipeline/dispatcher.hpp"
#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"
#include "pipeline/pipeline.hpp"

#include <cstddef>

class MarketDataPipeline final : public Pipeline
{
public:
    explicit MarketDataPipeline(
        EventConsumer& consumer);

    ~MarketDataPipeline() override = default;

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

    EventQueue queue_;

    Dispatcher dispatcher_;
};