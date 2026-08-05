 
 
 # Market Data Pipeline

## Overview

The Market Data Pipeline is responsible for transporting normalized
market data events from the market-data processing thread to the
Matching Engine using a lock-free Single Producer Single Consumer
(SPSC) queue.

The primary goals are:

- Decouple producers from consumers
- Preserve FIFO ordering
- Eliminate lock contention
- Minimize latency
- Provide a modular architecture for future extensions

---

# Architecture

```
                 ITCH Parser
                      │
                      ▼
                 ITCH Mapper
                      │
                      ▼
              MarketDataEvent
                      │
                      ▼
           MarketDataPipeline
                      │
                      ▼
             SPSC Ring Buffer
                      │
                      ▼
                Dispatcher
                      │
                      ▼
               EventConsumer
                      │
         ┌────────────┴─────────────┐
         ▼                          ▼
 TestConsumer          MatchingEngineConsumer
```

---

# Components

## MarketDataEvent

Represents a normalized event transported through the pipeline.

Current implementation contains only the event type.

Future versions will include payloads such as:

- OrderId
- Price
- Quantity
- Side

---

## MarketDataPipeline

Owns the communication queue between producer and consumer threads.

Responsibilities:

- Accept incoming events
- Push events into the SPSC queue
- Start the dispatcher
- Hide queue implementation details

Public API:

```cpp
bool submit(const MarketDataEvent&);
```

---

## SPSC Ring Buffer

The pipeline uses a lock-free fixed-capacity SPSC ring buffer.

Characteristics:

- Wait-free producer
- Wait-free consumer
- No dynamic memory allocation
- Cache-friendly layout

---

## Dispatcher

Consumes events from the SPSC queue.

Responsibilities:

- Preserve FIFO ordering
- Read events from the queue
- Forward events to EventConsumer

Dispatcher does not know anything about the Matching Engine.

---

## EventConsumer

Abstract interface implemented by downstream components.

Current implementations:

- TestConsumer
- MatchingEngineConsumer

Future implementations may include:

- Recorder
- Metrics collector
- Logger
- Risk Engine

---

## MatchingEngineConsumer

Acts as the bridge between Dispatcher and Matching Engine.

Current implementation provides the infrastructure only.

Event translation will be implemented in the next milestone.

---

# Design Principles

The Market Data Pipeline follows several important design principles.

## Loose Coupling

Dispatcher depends only on the EventConsumer interface.

```
Dispatcher
      │
      ▼
EventConsumer
```

instead of

```
Dispatcher
      │
      ▼
MatchingEngine
```

This improves modularity and testability.

---

## Single Responsibility

Each component has one clearly defined responsibility.

| Component | Responsibility |
|----------|----------------|
| Pipeline | Owns queue |
| Queue | Transport |
| Dispatcher | Dispatch |
| Consumer | Process event |

---

## Lock-Free Communication

Communication between producer and consumer is performed without locks.

Benefits:

- Low latency
- No mutex contention
- Predictable performance

---

## Testing

Current tests verify:

- Event submission
- Queue transport
- Dispatcher execution
- Consumer invocation

Current result:

```
112 / 112 tests passed
```

---

# Future Work

The next milestone introduces real event processing.

Planned additions include:

- AddOrder payload
- CancelOrder payload
- ReplaceOrder payload
- ExecuteOrder payload
- DeleteOrder payload

Eventually the complete pipeline becomes:

```
ITCH Message
      │
      ▼
MarketDataEvent
      │
      ▼
Pipeline
      │
      ▼
Dispatcher
      │
      ▼
MatchingEngineConsumer
      │
      ▼
Matching Engine
      │
      ▼
OrderBook
```

---

# Summary

P17.5 establishes the event-driven infrastructure used by the OpenHFT
platform.

The implementation provides:

- Modular architecture
- Lock-free communication
- FIFO ordering
- Event abstraction
- Consumer abstraction
- End-to-end integration tests

This foundation enables future integration with the Matching Engine,
Execution Engine, Monitoring, FPGA components, and additional
low-latency processing stages.
 
 
 ////////////////////////////////////////////////////////////
 
 Step 1  MarketDataEvent
 Step 2  Pipeline owns Queue
 Step 3  Dispatcher
 Step 4  Connect Queue ↔ Dispatcher
 Step 5  Inject Events
 Step 6  Matching Engine Integration
 Step 7  Integration Tests
 Step 8  Pipeline Benchmark


                
               MarketDataEvent
                       │
                       ▼
            MarketDataPipeline
                       │
                       ▼
               SPSC Ring Buffer
                       │
                       ▼
                 Dispatcher
                       │
                EventConsumer
          ┌────────────┴────────────┐
          ▼                         ▼
   Matching Engine           TestConsumer



                 ITCH Parser
                      │
                      ▼
                 ITCH Mapper
                      │
                      ▼
              MarketDataEvent
                      │
                      ▼
           MarketDataPipeline
                      │
                      ▼
             SPSC Ring Buffer
                      │
                      ▼
                Dispatcher
                      │
                      ▼
               EventConsumer
                      │
         ┌────────────┴─────────────┐
         ▼                          ▼
 TestConsumer          MatchingEngineConsumer







