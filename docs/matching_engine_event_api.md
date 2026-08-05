# P19 - Event-Based Matching Engine

## Overview

This milestone introduces an event-driven interface for the Matching Engine.

Instead of receiving only `Order*` objects, the Matching Engine now supports
processing normalized `MarketDataEvent` objects.

This change decouples market data parsing from order matching while preserving
the existing pointer-based interface for backward compatibility.

The migration is performed incrementally to minimize refactoring risk and keep
existing components operational throughout the transition.

---

## Motivation

Previously, the processing pipeline tightly coupled parsing with Order Book
operations.

```
ITCH Parser
      │
      ▼
ITCHHandler
      │
      ▼
ArrayOrderBook
```

As the platform evolves toward a modular low-latency trading system, multiple
components should be able to generate market events without depending on Order
Book internals.

Examples include:

- Replay Engine
- DPDK Receiver
- FPGA Feed Handler
- Exchange Simulator
- Market Data Recovery

Introducing a normalized event interface enables this separation.

---

## New Architecture

```
           ITCH Parser
                │
                ▼
        MarketDataEvent
                │
                ▼
        Event Dispatcher
                │
                ▼
    MatchingEngineConsumer
                │
                ▼
         MatchingEngine
                │
                ▼
         ArrayOrderBook
```

The consumer performs no business logic.

Its only responsibility is forwarding events to the Matching Engine.

All matching behavior remains centralized inside the Matching Engine.

---

## Implemented

### 1. MarketDataEvent

A normalized event model was introduced supporting:

- AddOrder
- CancelOrder
- DeleteOrder
- ExecuteOrder
- ReplaceOrder

Each event contains the information required by the Matching Engine without
exposing Order Book implementation details.

---

### 2. Event-Based Matching Engine API

A new overload was introduced:

```cpp
bool process(const MarketDataEvent& event);
```

while preserving the existing interface:

```cpp
void process(Order* order);
```

This allows gradual migration without breaking existing callers.

---

### 3. MatchingEngineConsumer

The consumer has been simplified.

Instead of dispatching individual event types itself, it simply forwards every
event directly to the Matching Engine.

```
Dispatcher
      │
      ▼
MatchingEngineConsumer
      │
      ▼
MatchingEngine::process(event)
```

This keeps business logic centralized in a single component.

---

### 4. Event Processing

The Matching Engine now handles all supported market-data events.

| Event | Status |
|-------|--------|
| AddOrder | ✅ |
| CancelOrder | ✅ |
| DeleteOrder | ✅ |
| ExecuteOrder | ✅ |
| ReplaceOrder | ✅ |

---

## Order Ownership

One important architectural improvement introduced during this milestone is
centralized order ownership.

Orders created through the Event API are allocated from the Matching Engine's
internal OrderPool.

```
MarketDataEvent
        │
        ▼
OrderPool::acquire()
        │
        ▼
Order
        │
        ▼
process(Order*)
```

This reuses the existing matching implementation instead of duplicating
matching logic.

---

## Safe Order Recycling

Orders removed after:

- Delete
- Full Cancel
- Full Execute
- Fully matched incoming orders

are safely recycled using:

```
releaseIfOwned()
```

The helper internally verifies ownership using:

```
OrderPool::owns()
```

Only orders allocated from the internal OrderPool are returned to the pool.

Orders originating from legacy callers remain untouched.

This guarantees safe ownership management during the migration.

---

## Memory Management

Before:

```
External Owner
      │
      ▼
 Order*
      │
      ▼
Matching Engine
```

After:

```
MarketDataEvent
        │
        ▼
 OrderPool
        │
        ▼
Matching Engine
        │
        ▼
releaseIfOwned()
```

Dynamic allocation is removed from the Event-based AddOrder hot path.

---

## Migration Strategy

The migration is intentionally incremental.

Both interfaces currently coexist.

```cpp
process(Order*)
process(const MarketDataEvent& event)
```

Existing components continue using the legacy interface while new components
use the Event API.

This minimizes migration risk and allows each subsystem to transition
independently.

Future milestones will gradually eliminate direct `Order*` usage.

---

## Benefits

- Decouples parsing from matching.
- Centralizes matching logic.
- Provides a unified Event interface.
- Preserves backward compatibility.
- Centralizes order ownership.
- Eliminates dynamic allocation for Event-based AddOrder.
- Safely recycles pooled orders.
- Enables Replay, DPDK, FPGA, and simulators to share the same processing path.
- Simplifies future performance optimization.

---

## Current Progress

Completed:

- ✅ Event infrastructure
- ✅ Dispatcher integration
- ✅ Matching Engine Event API
- ✅ MatchingEngineConsumer forwarding
- ✅ AddOrder implementation
- ✅ CancelOrder implementation
- ✅ DeleteOrder implementation
- ✅ ExecuteOrder implementation
- ✅ ReplaceOrder implementation
- ✅ OrderPool integration
- ✅ Order ownership tracking
- ✅ Safe pooled-order recycling
- ✅ Backward compatibility with the legacy API

Remaining:

- Gateway migration to the Event API
- Replay migration to the Event API
- Removal of the legacy `process(Order*)` interface
- End-to-end Event pipeline benchmarking

---

## Conclusion

This milestone establishes the Event-Based Matching Engine as the new entry
point for market-data processing.

The implementation preserves compatibility with existing components while
introducing a cleaner architecture based on normalized market events.

By combining Event-based processing with centralized OrderPool ownership, the
Matching Engine now provides deterministic memory management, reusable matching
logic, and a solid foundation for Replay, DPDK, FPGA integration, and future
low-latency optimizations.
