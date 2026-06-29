# OpenHFT FPGA DPDK

OpenHFT FPGA DPDK is a learning and research project for building a simplified low-latency market data pipeline using FPGA, DPDK, and C++.

The goal is to understand how high-performance trading infrastructure processes market data from the network layer up to an order book and trading strategy.

## Project Goals

- Build FPGA packet parsing modules
- Implement Ethernet, ARP, IPv4, UDP, and ITCH parsing
- Build a DPDK-based packet receiver
- Implement a software order book in C++
- Compare software and hardware processing paths
- Measure latency and throughput

## Architecture

```text
NIC
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
Order Book
 │
 ▼
Strategy



Repository Structure

openhft-fpga-dpdk/
├── docs/
├── fpga/
├── dpdk/
├── orderbook/
├── strategy/
├── benchmark/
├── scripts/
├── tests/
└── tools/





