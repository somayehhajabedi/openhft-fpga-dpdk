# P18 - Event-Based Matching Engine

## Overview

This milestone introduces an event-driven interface for the Matching Engine.

Instead of receiving only `Order*` objects, the Matching Engine now supports
processing normalized `MarketDataEvent` objects.

This change decouples market data parsing from order matching and establishes a
common interface for all market data producers.

---

## Motivation

Previously, the processing pipeline tightly coupled parsing with order book
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

As the system evolves toward a modular low-latency architecture, multiple
components (Replay, DPDK Receiver, FPGA, Simulators) should be able to submit
market events without knowing Order Book internals.

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

The consumer simply forwards events.

All event-specific logic resides inside the Matching Engine.

---

## Implemented

### 1. MarketDataEvent

A normalized event model was introduced supporting:

- AddOrder
- CancelOrder
- DeleteOrder
- ExecuteOrder
- ReplaceOrder

---

### 2. Event-based Matching Engine API

A new overload was introduced:

```cpp
bool process(const MarketDataEvent& event);
```

while preserving the existing API:

```cpp
void process(Order* order);
```

This allows gradual migration without breaking existing callers.

---

### 3. Implemented Event Types

The following event handlers are fully implemented:

- CancelOrder
- DeleteOrder
- ExecuteOrder
- ReplaceOrder

Each event is translated into the corresponding Order Book operation.

---

### 4. MatchingEngineConsumer

The consumer no longer contains event-specific logic.

Its sole responsibility is forwarding events to the Matching Engine.

```
Dispatcher
      │
      ▼
MatchingEngineConsumer
      │
      ▼
MatchingEngine::process(event)
```

This keeps business logic centralized.

---

## Current Status

| Event | Status |
|-------|--------|
| AddOrder | 🚧 In Progress |
| CancelOrder | ✅ |
| DeleteOrder | ✅ |
| ExecuteOrder | ✅ |
| ReplaceOrder | ✅ |

---

## Next Step

The remaining work is integrating the existing OrderPool into the
Matching Engine.

The future AddOrder path will become:

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

Orders removed by:

- Delete
- Execute
- Full Cancel

will be recycled using:

```
OrderPool::release()
```

This preserves deterministic memory management while maintaining compatibility
with the existing Matching Engine implementation.

---

## Benefits

- Decouples parsing from matching.
- Provides a unified event interface.
- Enables Replay, DPDK, FPGA, and simulators to share the same processing path.
- Preserves backward compatibility during migration.
- Keeps event-processing logic centralized.
- Simplifies future performance optimization.

---

## Current Progress

-  Event infrastructure
-  Dispatcher integration
-  Matching Engine event API
-  Consumer forwarding
-  Order update events

Remaining:

- OrderPool migration
- AddOrder implementation
- Order ownership migration
