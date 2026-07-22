# OpenHFT-FPGA-DPDK

A modular low-latency trading platform implemented in modern C++17, focused on high-performance market data processing, DPDK-based packet processing, risk management, automated testing, and performance optimization.

**Status:** 🚧 Active Development

---

## Overview

OpenHFT-FPGA-DPDK is an ongoing engineering project that explores the design and implementation of production-style high-performance trading infrastructure.

The project focuses on building a modular and extensible trading platform while emphasizing:

- Low-latency software design
- Modern C++17
- High-performance packet processing
- Market data processing
- Performance profiling
- Automated testing
- Future FPGA acceleration

---

## Architecture

```
                +------------------+
                | Packet Replay    |
                +------------------+
                          |
                          v
                +------------------+
                | Ethernet Parser  |
                +------------------+
                          |
                          v
                +------------------+
                | IPv4 Parser      |
                +------------------+
                          |
                          v
                +------------------+
                | UDP Parser       |
                +------------------+
                          |
                          v
                +------------------+
                | ITCH Parser      |
                +------------------+
                          |
                          v
                +------------------+
                | Gateway          |
                +------------------+
                          |
                          v
                +------------------+
                | Risk Manager     |
                +------------------+
                          |
                          v
                +------------------+
                | Matching Engine  |
                +------------------+
                          |
                          v
                +------------------+
                | Order Book       |
                +------------------+
```

---

## Features

### Market Data

- Ethernet frame parsing
- IPv4 packet parsing
- UDP packet parsing
- NASDAQ ITCH message parsing
- Replay engine for deterministic testing

### Trading Engine

- Modular Gateway
- Matching Engine
- Order Book
- Risk Management

### Performance

- Google Benchmark
- Linux perf profiling
- Low-latency oriented architecture

### Testing

- GoogleTest
- Unit Tests
- Integration Tests

### Build System

- Modern CMake
- Modular library architecture

---

## Technologies

- C++17
- Linux
- CMake
- GoogleTest
- Google Benchmark
- DPDK
- Git

---

## Repository Structure

```
benchmark/
docs/
dpdk/
ethernet/
gateway/
ipv4/
itch/
orderbook/
replay/
risk/
tests/
udp/
```

---

## Build

```bash
cmake -S . -B build
cmake --build build
```

---

## Run Tests

```bash
ctest --test-dir build
```

or

```bash
./build/tests/parser_tests
```

---

## Run Benchmark

```bash
./build/benchmark/order_book_benchmark
```

---

## Roadmap

### Completed

- Modular CMake architecture
- Ethernet parser
- IPv4 parser
- UDP parser
- ITCH parser
- Replay engine
- Gateway
- Matching engine
- Order book
- Risk manager
- Unit tests
- Integration tests
- Performance benchmarks

### In Progress

- Trading strategy framework

### Planned

- Journaling
- Monitoring (Prometheus + Grafana)
- CI/CD pipeline
- DPDK optimization
- FPGA implementation

---

## Design Goals

The long-term objective of this project is to build a modular, production-inspired trading platform that explores:

- Low-latency software architecture
- High-performance networking
- Market data processing
- Order management
- Risk controls
- Performance engineering
- FPGA acceleration

