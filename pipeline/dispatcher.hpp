#pragma once

/*
 * Dispatcher
 * ==========
 *
 * Purpose
 * -------
 * Consumes normalized MarketDataEvent objects from the lock-free
 * SPSC queue and forwards them to downstream processing stages.
 *
 * Responsibilities
 * ----------------
 * - Consume events from the SPSC queue.
 * - Preserve FIFO ordering.
 * - Dispatch events according to their type.
 * - Decouple producer and consumer threads.
 *
 * Design
 * ------
 *               MarketDataEvent
 *                       │
 *                       ▼
 *              SPSC Ring Buffer
 *                       │
 *                       ▼
 *                  Dispatcher
 *                       │
 *                       ▼
 *                Matching Engine
 *
 * Current Implementation
 * ----------------------
 * The initial implementation validates queue integration by
 * consuming events from the SPSC queue.
 *
 * Future Work
 * -----------
 * - Forward events to the Matching Engine.
 * - Collect dispatch metrics.
 * - Measure dispatch latency.
 * - Integrate monitoring.
 */

#pragma once

#include "common/spsc_ring_buffer.hpp"
#include "pipeline/event_consumer.hpp"
#include "pipeline/market_data_event.hpp"

#include <cstddef>

class Dispatcher
{
public:

    static constexpr std::size_t QueueCapacity = 4096;

    using EventQueue =
        SPSCRingBuffer<
            MarketDataEvent,
            QueueCapacity>;

    explicit Dispatcher(
        EventQueue& queue,
        EventConsumer& consumer);

    [[nodiscard]]
    bool dispatch();

private:

    EventQueue& queue_;

    EventConsumer& consumer_;
};
