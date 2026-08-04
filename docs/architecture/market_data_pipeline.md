# Market Data Pipeline

## Overview

The Market Data Pipeline is responsible for decoupling market data
ingestion from order book processing.

Instead of invoking the matching engine directly, parsed market data
is published into a lock-free queue and processed asynchronously by
a dedicated matching thread.

This architecture reduces coupling between components while improving
throughput and enabling future scalability.

---

# Motivation

Originally, market data flowed directly from the parser into the
matching engine.

Receiver
    ↓
Parser
    ↓
Matching Engine

Although simple, this design couples packet reception and order
processing into the same execution path.

A slow matching engine can therefore delay packet processing.

The new architecture introduces asynchronous communication using
a lock-free SPSC queue.

---

# Architecture

                Receiver Thread

                        │

                        ▼

                Ethernet Parser

                        │

                        ▼

                  IPv4 Parser

                        │

                        ▼

                   UDP Parser

                        │

                        ▼

                   ITCH Parser

                        │

                        ▼

                MarketDataPipeline

                        │

                        ▼

                 Lock-Free Queue

                        │

                        ▼

                Matching Thread

                        │

                        ▼

                Matching Engine

---

# Responsibilities

MarketDataPipeline is responsible for

- owning the communication queue

- managing the worker thread

- publishing parsed orders

- forwarding orders to the matching engine

The pipeline intentionally contains no matching logic.

---

# Thread Model

Producer Thread

- DPDK Receiver
- Packet Parsing
- ITCH Mapping
- Queue Push

Consumer Thread

- Queue Pop
- Matching Engine
- Risk Processing

This follows a Single Producer / Single Consumer model.

---

# Queue Selection

The pipeline currently uses

SPSCRingBuffer<Order>

because

- exactly one producer exists

- exactly one consumer exists

- minimal synchronization overhead

- predictable latency

---

# Future Extensions

The pipeline is designed to support additional asynchronous
consumers.

Examples include

- Async Logger

- Binary WAL

- Monitoring

- Strategy Engine

- FPGA Offload

without changing the packet parser.

---

# Benefits

Compared with the previous synchronous architecture

- lower latency

- improved throughput

- reduced coupling

- simpler scalability

- better observability

---

# Current Scope

Version 1 focuses only on

Parser

↓

Queue

↓

Matching Engine

Additional components will be integrated incrementally
in future milestones.

---

# Roadmap

P17

Lock-Free Queue

↓

P17.5

Pipeline Integration

↓

P18

SIMD Optimizations

↓

P19

Async Logger

↓

P20

Write-Ahead Logging

↓

P21

Monitoring

↓

P22

Strategy Framework

↓

P23

Execution Gateway

↓

P24

FPGA Integration





