#pragma once

/*
 * Market Data Event
 * =================
 *
 * Purpose
 * -------
 * Represents a normalized market-data operation transported from
 * the market-data producer thread to the matching-engine consumer
 * through the lock-free SPSC pipeline.
 *
 * The event uses the same domain types as the Order Book to avoid
 * unnecessary conversion inside the hot path.
 * 
 * 
Event          Fields
AddOrder	   orderId, accountId, side, price, quantity
CancelOrder	   orderId, quantity
DeleteOrder	   orderId
ExecuteOrder   orderId, quantity
ReplaceOrder   orderId, newOrderId, price, quantity

 */

#include "orderbook/software/common/types.hpp"
#include <cstdint>

enum class MarketDataEventType : std::uint8_t
{
    AddOrder,
    CancelOrder,
    DeleteOrder,
    ExecuteOrder,
    ReplaceOrder
};

struct MarketDataEvent
{
    MarketDataEventType type{
        MarketDataEventType::AddOrder};

    OrderId orderId{0};

    OrderId newOrderId{0};

    AccountId accountId{0};

    Side side{Side::Buy};

    Price price{0};

    Quantity quantity{0};
};
