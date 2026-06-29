# Project Roadmap

This document describes the implementation roadmap for the OpenHFT FPGA DPDK project.

---

# Phase 1 — Documentation

## Goals

- [x] Project structure
- [x] README
- [x] Architecture
- [x] Data Flow
- [ ] Roadmap

---

# Phase 2 — FPGA Networking

Implement protocol parsers in FPGA.

## Ethernet

- [ ] Ethernet frame parser

## ARP

- [ ] ARP parser

## IPv4

- [ ] IPv4 parser

## UDP

- [ ] UDP parser

---

# Phase 3 — Market Data

Implement Nasdaq ITCH parser.

- [ ] Message decoder
- [ ] Add Order
- [ ] Delete Order
- [ ] Execute Order
- [ ] Replace Order

---

# Phase 4 — DPDK

Build the software packet processing pipeline.

- [ ] Packet receiver
- [ ] Packet parser
- [ ] ITCH decoder
- [ ] Performance benchmark

---

# Phase 5 — Order Book

Software implementation.

- [ ] Price levels
- [ ] Order storage
- [ ] Add Order
- [ ] Delete Order
- [ ] Execute Order
- [ ] Modify Order

---

# Phase 6 — Benchmark

Compare FPGA and DPDK implementations.

- [ ] Latency
- [ ] Throughput
- [ ] CPU utilization
- [ ] Resource utilization

---

# Phase 7 — FPGA Integration

- [ ] Hardware validation
- [ ] FPGA simulation
- [ ] Board testing

---

# Phase 8 — Future Work

- [ ] FPGA Order Book
- [ ] FPGA Strategy
- [ ] PCIe Integration
- [ ] 10G Ethernet
- [ ] 25G Ethernet
- [ ] Hardware Timestamping
