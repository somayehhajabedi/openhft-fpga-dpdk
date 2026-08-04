#pragma once

#include <cstdint>


/*
 * Identifies the type of normalized market-data event
 * transported through the asynchronous pipeline.
 *
 * Payload fields will be added incrementally as the
 * pipeline integration evolves.
 */
enum class MarketDataEventType : std::uint8_t
{
    AddOrder,
    CancelOrder,
    DeleteOrder,
    ExecuteOrder,
    ReplaceOrder
};

/*
 * Normalized event transported between the market-data
 * producer thread and the matching-engine consumer thread.
 *
 * Version 1 contains only the event type. Concrete payloads
 * will be introduced after the queue and dispatcher skeletons
 * are integrated successfully.
 */
struct MarketDataEvent
{
    MarketDataEventType type{
        MarketDataEventType::AddOrder};
};
