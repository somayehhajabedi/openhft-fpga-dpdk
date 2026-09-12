# OpenHFT-FPGA-DPDK

⚠️ This repository is publicly visible for portfolio, educational, and interview
demonstration purposes only.

Copyright © 2026 Somayeh Hajabedi.
All Rights Reserved.

No permission is granted to use this code in commercial or non-commercial
projects without explicit written permission.

---

## Overview

**OpenHFT-FPGA-DPDK** is a modular low-latency electronic trading platform
implemented in **Modern C++20** on Linux.

The project explores the architecture of latency-sensitive trading systems,
including:

- DPDK-based packet processing
- Ethernet / IPv4 / UDP protocol parsing
- NASDAQ ITCH market-data decoding
- Lock-free SPSC communication
- Cache-conscious limit order books
- Strategy and signal modeling
- Pre-trade risk management
- OUCH order entry
- TLS 1.3 encrypted execution transport
- Exchange simulation
- Matching-engine design
- Application-level latency benchmarking
- Future FPGA acceleration

**Status:** 🚧 Active Development

The project is designed as a production-inspired engineering environment rather
than a simplified trading application. The primary focus is deterministic
behavior, explicit ownership, predictable memory usage, modularity, and
measurable latency.

---

# System Architecture

The system contains two major flows:

1. **Market Data / Strategy Path**
2. **Order Entry / Exchange Path**

```text
                         MARKET DATA PATH

 NIC / PCAP Replay
        |
        v
      DPDK
        |
        v
    Ethernet
        |
        v
      IPv4
        |
        v
       UDP
        |
        v
   ITCH Parser
        |
        v
 MarketDataEvent
        |
        v
   Lock-Free SPSC
        |
        v
 MarketDataBookConsumer
        |
        v
 Local Order Book
        |
        v
 MarketFeatureExtractor
        |
        v
 LinearSignalModel
        |
        v
 MicrostructureStrategy
        |
        v
    OrderIntent


                         ORDER ENTRY PATH

    OrderIntent
        |
        v
      Gateway
        |
        v
   RiskManager
        |
        v
 OuchExecutionSink
        |
        v
   OuchEncoder
        |
        v
 TcpOuchTransport / TlsOuchTransport
        |
        |   OUCH over TCP / TLS 1.3
        v
+-----------------------------------+
|        Exchange Simulator         |
|                                   |
| ExchangeTcpServer                 |
|        |                          |
|      epoll                        |
|        |                          |
| ExchangeOuchHandler               |
|        |                          |
| MatchingEngine                    |
|        |                          |
| Exchange Order Book               |
+-----------------------------------+
        |
        v
  OUCH Accepted
        |
        v
OuchResponseDispatcher
```

This separation keeps market-data processing independent from the exchange
execution protocol and allows components to be benchmarked independently.

---

# Market Data Pipeline

The market-data path converts raw network packets into normalized events used by
the trading system.

```text
NIC
 |
 v
DPDK RX
 |
 v
Ethernet Parser
 |
 v
IPv4 Parser
 |
 v
UDP Parser
 |
 v
ITCH Parser
 |
 v
MarketDataEvent
 |
 v
SPSC Queue
 |
 v
Local Order Book
```

## Protocol Layers

Implemented protocol processing includes:

- Ethernet II
- IPv4
- UDP
- NASDAQ ITCH-style market-data messages

Raw ITCH messages are converted into a normalized `MarketDataEvent`.

This prevents the order book and strategy layers from depending directly on the
wire protocol.

---

# Lock-Free Market Data Handoff

The market-data pipeline uses a **Single Producer / Single Consumer (SPSC)**
ring buffer between producer and consumer execution contexts.

```text
Market Data Producer
        |
        v
   SPSC Ring Buffer
        |
        v
Market Data Consumer
```

The design avoids mutex contention on the hot path and provides predictable
communication between the network/replay side and the order-book processing
thread.

The project also explores:

- acquire/release memory ordering
- false sharing
- cache-line separation
- bounded queues
- deterministic ownership

Handling stale market state after queue overflow or sequence gaps is part of
the continuing recovery work.

---

# Local Order Book

The software order book is designed around predictable memory access and
single-writer ownership.

Key design choices include:

- Fixed-size price-level arrays
- O(1) price-to-index mapping
- Intrusive linked lists inside price levels
- FIFO price-time priority
- Fixed-capacity order lookup
- Object pooling
- Avoids dynamic allocation on latency-sensitive paths where practical
- Single-writer mutation model

Simplified structure:

```text
Price Level
    |
    +--> Order --> Order --> Order
          FIFO price-time priority
```

A custom `FixedHashMap` is used for order lookup to avoid behavior associated
with general-purpose `std::unordered_map`, such as rehashing and unpredictable
dynamic allocation.

---

# Strategy Framework

The strategy path converts current market state into lightweight features and
an order decision.

```text
Local Order Book
       |
       v
   MarketView
       |
       v
MarketFeatureExtractor
       |
       v
 MarketFeatures
       |
       v
LinearSignalModel
       |
       v
MicrostructureStrategy
       |
       v
   OrderIntent
```

Extracted features include:

- Best bid price
- Best ask price
- Best bid quantity
- Best ask quantity
- Spread
- Mid-price
- Order-book imbalance
- Microprice

Example imbalance:

```text
imbalance = bidQty / (bidQty + askQty)
```

Example microprice:

```text
microPrice =
    (askPrice * bidQty + bidPrice * askQty)
    / (bidQty + askQty)
```

The live path performs only lightweight inference.

Model fitting, parameter calibration, and historical analysis are intended to
run offline rather than on the latency-sensitive execution path.

---

# Gateway and Risk Management

Strategy output is represented as an `OrderIntent`.

Before reaching the execution transport, orders pass through:

```text
Strategy
   |
   v
OrderIntent
   |
   v
Gateway
   |
   v
RiskManager
   |
   v
Execution
```

This keeps trading decisions separated from execution and pre-trade controls.

---

# OUCH Order Entry

The execution layer implements an OUCH-style binary order-entry protocol.

Current order-entry path:

```text
OrderIntent
    |
    v
OuchExecutionSink
    |
    v
OuchEncoder
    |
    v
47-byte EnterOrder
    |
    v
TCP / TLS
```

The transport layer is abstracted through `OuchTransport`.

Available implementations include:

- TCP transport
- TLS transport
- Recording transport for testing/runtime verification

This allows the execution layer to remain independent of the selected
transport mechanism.

---

# TLS 1.3 Execution Transport

OUCH order entry can operate over TLS.

The current TLS implementation uses OpenSSL and performs:

- Server certificate validation
- CA verification
- IP identity verification
- Encrypted OUCH transport
- TLS client/server handshake

A verified runtime session negotiated:

```text
TLS version: TLSv1.3
TLS cipher: TLS_AES_256_GCM_SHA384
```

The TLS handshake occurs when the execution connection is established and is
kept outside the steady-state order latency measurement.

The current implementation authenticates the exchange server. Mutual TLS
(mTLS) is not currently implemented.

---

# Exchange Simulator

The repository contains a standalone exchange-side runtime.

```text
Linux TCP
    |
    v
ExchangeTcpServer
    |
    v
epoll Reactor
    |
    v
Optional TLS
    |
    v
ExchangeOuchHandler
    |
    v
MatchingEngine
    |
    v
Exchange Order Book
```

The exchange server supports:

- TCP connections
- TLS connections
- non-blocking sockets
- `epoll`
- incremental reads
- OUCH EnterOrder processing
- OUCH Accepted responses

The exchange runtime can be started independently from the trading runtime.

---

# OUCH TLS Round-Trip Benchmark

A dedicated benchmark measures steady-state application-level OUCH round-trip
latency over TLS.

Measured path:

```text
Client
  |
  | EnterOrder
  v
TLS / TCP loopback
  |
  v
ExchangeTcpServer
  |
  v
ExchangeOuchHandler
  |
  v
MatchingEngine
  |
  v
Accepted
  |
  v
TLS / TCP loopback
  |
  v
Client
  |
  v
OuchResponseDispatcher
```

The TLS handshake is intentionally excluded from the timed region.

## Baseline

Release build, 100,000 measured round trips:

| Metric | Latency |
|---|---:|
| Minimum | 16.015 µs |
| Mean | 27.507 µs |
| p50 | 25.077 µs |
| p95 | 37.381 µs |
| p99 | 44.838 µs |
| p99.9 | 97.058 µs |
| Maximum | 1.691 ms |

Measured throughput:

```text
36.319K round-trips/sec
```

These numbers represent an **application-level TLS loopback benchmark**, not
physical network latency.

The path includes OUCH encoding, TLS, TCP/kernel networking, exchange request
handling, matching-engine processing, response generation, TLS receive, and
response dispatch.

Detailed methodology:

```text
docs/performance/ouch_tls_round_trip_benchmark.md
```

---

# Performance Engineering

The repository contains benchmarks and experiments for several latency-sensitive
components.

Areas explored include:

- Market-data pipeline latency
- Matching-engine performance
- Order-book performance
- Fixed hash maps
- Lock-free queues
- False sharing
- Memory allocation
- Object pools
- Huge pages
- NUMA
- TLS round-trip latency

Linux profiling and debugging tools used during development include:

- `perf`
- Google Benchmark
- Valgrind
- Cachegrind / KCachegrind
- AddressSanitizer
- UndefinedBehaviorSanitizer
- ThreadSanitizer
- GDB

Latency analysis focuses on distributions such as:

```text
p50
p95
p99
p99.9
```

rather than averages alone.

---

# DPDK

The networking layer includes DPDK experimentation for kernel-bypass packet
processing.

The software market-data pipeline is designed around:

```text
NIC
 |
 v
RX Descriptor Ring
 |
 v
DMA
 |
 v
DPDK mbuf
 |
 v
Packet Parser
```

Development has included:

- EAL initialization
- RX queue configuration
- mbuf pools
- huge pages
- VFIO
- IOMMU
- NIC binding
- packet receive testing

The architecture allows packet replay to be used when DPDK hardware is not
available.

---

# FPGA Direction

The long-term architecture includes FPGA acceleration for selected packet
processing stages.

Target areas include:

```text
Ethernet
   |
IPv4
   |
UDP
   |
ITCH
```

The objective is not to move the entire trading system into hardware, but to
experiment with moving deterministic parsing and feed-processing stages closer
to the NIC while retaining strategy, risk, and orchestration logic in software.

FPGA integration remains active roadmap work.

---

# Testing

The project uses automated unit and integration testing.

Testing includes:

- Ethernet parsing
- IPv4 parsing
- UDP parsing
- ITCH parsing
- Replay processing
- Order-book operations
- FixedHashMap
- Risk management
- Strategy components
- OUCH encoding/decoding
- TCP execution
- TLS execution
- Exchange integration
- End-to-end runtime paths

GoogleTest is used for the test suite.

Sanitizer configurations are also used to detect memory, undefined-behavior,
and concurrency issues.

---

# Build

Requirements include:

- Linux
- C++20-capable compiler
- CMake
- Ninja or Make
- OpenSSL development libraries
- GoogleTest
- Google Benchmark
- DPDK for DPDK-specific components

Example Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

# Run Tests

```bash
ctest --test-dir build --output-on-failure
```

---

# Generate Sample ITCH Replay

A sample replay file can be generated using the replay-generation tool.

```bash
./build/tools/generate_sample_itch_replay
```

The generated market state can be used to exercise the complete
market-data → strategy → execution pipeline.

---

# Run Trading Runtime

Recording transport:

```bash
./build/apps/trading_runtime \
    --mode replay sample_itch_replay.bin \
    --execution recording
```

TLS execution:

```bash
./build/apps/trading_runtime \
    --mode replay sample_itch_replay.bin \
    --execution tls \
    --host 127.0.0.1 \
    --port 9000 \
    --ca certs/server.crt
```

---

# Run Exchange Simulator

```bash
./build/apps/exchange_runtime \
    --port 9000 \
    --cert certs/server.crt \
    --key certs/server.key
```

The private key is intentionally excluded from version control.

---

# Run TLS Round-Trip Benchmark

Build:

```bash
cmake --build build \
    --target ouch_tls_round_trip_benchmark \
    -j4
```

Run:

```bash
./build/benchmarks/ouch_tls_round_trip_benchmark \
    certs/server.crt \
    certs/server.key
```

---

# Repository Structure

```text
apps/                 Runtime composition roots
benchmarks/           Performance and latency benchmarks
common/               Shared low-level utilities
dispatcher/           Event dispatching
docs/                 Architecture and performance documentation
dpdk/                 DPDK networking
ethernet/             Ethernet parsing
exchange/             Exchange simulator and OUCH handling
execution/            Execution protocols and transports
fpga/                 FPGA-oriented protocol components
gateway/              Order gateway
ipv4/                 IPv4 parsing
itch/                 ITCH protocol processing
market_data/          Replay and market-data components
orderbook/            Software order book and matching engine
pipeline/             Market-data processing pipeline
risk/                 Pre-trade risk management
strategy/             Features, models, and trading strategies
tests/                Tests and integration tests
tools/                Development/replay tools
udp/                  UDP parsing
```

---

# Design Principles

The project intentionally emphasizes several principles common in
latency-sensitive systems:

- Single-writer state mutation
- Bounded data structures
- Explicit ownership
- Dependency injection
- Composition over unnecessary inheritance
- No allocation on critical paths where practical
- Cache-conscious data layout
- Lock-free communication at thread boundaries
- Separation of wire protocols from business logic
- Lightweight live inference
- Measured optimization instead of assumption-driven optimization

---

# Current Development Direction

Current engineering work focuses on:

- Execution response processing
- Latency and tail-latency optimization
- CPU affinity and scheduling
- Busy polling
- Socket tuning
- Market-data stale-state detection and recovery
- Sequence-gap handling
- Expanded strategy modeling
- Performance regression tracking

---

# Roadmap

## Completed / Implemented

- Modern C++20 modular architecture
- Ethernet / IPv4 / UDP parsing
- ITCH market-data parsing
- Replay infrastructure
- Normalized MarketDataEvent
- Lock-free SPSC handoff
- Local order book
- Fixed-capacity hash map
- Matching engine
- Gateway
- Risk manager
- Strategy framework
- Market feature extraction
- Linear signal model
- Microstructure strategy
- OUCH encoding
- TCP OUCH execution
- TLS 1.3 OUCH execution
- Exchange simulator
- epoll-based exchange server
- OUCH Accepted response path
- TLS round-trip latency benchmark
- Unit and integration testing
- CI infrastructure

## In Progress / Planned

- Continuous OUCH execution-response receiver
- Full execution lifecycle handling
- Market-data recovery and stale-state management
- CPU affinity / busy-poll optimization
- Network and TLS latency optimization
- Performance regression automation
- Monitoring and observability
- FIX integration
- FPGA integration
- Cross-machine latency experiments

---

# Project Goal

The long-term goal of OpenHFT-FPGA-DPDK is to provide a practical engineering
environment for studying how the major components of an electronic trading
system interact:

```text
Market Data
    ↓
Order Book
    ↓
Features / Model
    ↓
Strategy
    ↓
Risk
    ↓
Order Entry
    ↓
Exchange
    ↓
Execution Responses
```

The project emphasizes understanding the entire path—from packets arriving at
the network layer to an order reaching an exchange simulator—and measuring the
performance implications of architectural decisions along that path.
