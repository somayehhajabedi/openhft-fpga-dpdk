# OpenHFT FPGA DPDK Architecture

## Overview

This project implements a simplified low-latency market data pipeline inspired by modern High-Frequency Trading (HFT) systems.

The goal is to process market data from the network interface all the way to an order book while comparing software (DPDK) and hardware (FPGA) implementations.

## Design Philosophy

The software pipeline is intentionally implemented as a
layered protocol stack.

Each parser owns a single protocol layer and exposes the
payload of the next protocol.

This separation allows the same parsers to be reused with:

- DPDK
- PCAP files
- FPGA
- Unit tests

without modification.

## High-Level Architecture

```text
                  +----------------------+
                  |    Exchange Feed     |
                  +----------+-----------+
                             |
                             |
                     Ethernet Frames
                             |
                             ▼
                     +---------------+
                     |      NIC      |
                     +-------+-------+
                             |
          +------------------+------------------+
          |                                     |
          ▼                                     ▼
+----------------------+            +----------------------+
|     FPGA Pipeline    |            |     DPDK Pipeline    |
+----------------------+            +----------------------+
| Ethernet Parser      |            | Packet Receiver      |
| ARP Parser           |            | Ethernet Parser      |
| IPv4 Parser          |            | IPv4 Parser          |
| UDP Parser           |            | UDP parser           |
| ITCH Parser          |            | ITCH parser          |
+----------+-----------+            +----------+-----------+
           |                                   |
           +---------------+-------------------+
                           |
                           ▼
                  Software Order Book
                           |
                           ▼
                       Strategy
                           |
                           ▼
                    Performance Metrics
```

---

## FPGA Data Path

```
Ethernet
    │
    ▼
ARP
    │
    ▼
IPv4
    │
    ▼
UDP
    │
    ▼
ITCH
```

Each module removes one protocol header and forwards the remaining payload to the next stage.

---

## Software Data Path

```
NIC
 │
 ▼
DPDK RX
 │
 ▼
Receiver
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

---

## Project Components

### FPGA

    - Ethernet Parser
    - ARP Parser
    - IPv4 Parser
    - UDP Parser
    - ITCH Parser

### DPDK

    - Receiver

    - Ethernet Parser

    - IPv4 Parser

    - UDP Parser

    - ITCH Parser

    - Benchmarks

### Order Book

- Software implementation
- Future hardware implementation

---

## Design Goals

- Low latency
- High throughput
- Modular architecture
- Easy simulation
- Incremental development
- Software and hardware comparison

---

## Future Work

- FPGA Order Book
- Hardware Strategy
- PCIe Integration
- 10G/25G Ethernet
- Hardware Benchmarking


## Current Status

✅ DPDK Receiver

✅ Ethernet Parser

✅ IPv4 Parser

✅ UDP Parser

⬜ ITCH Parser

⬜ Order Book

⬜ Matching Engine

⬜ FPGA Pipeline
