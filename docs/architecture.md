# OpenHFT FPGA DPDK Architecture

## Overview

This project implements a simplified low-latency market data pipeline inspired by modern High-Frequency Trading (HFT) systems.

The goal is to process market data from the network interface all the way to an order book while comparing software (DPDK) and hardware (FPGA) implementations.

---

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
| ARP Parser           |            | Packet Parser        |
| IPv4 Parser          |            | ITCH Parser          |
| UDP Parser           |            |                      |
| ITCH Parser          |            |                      |
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
Packet Parser
    │
    ▼
ITCH Parser
    │
    ▼
Software Order Book
```

---

## Project Components

### FPGA

- Ethernet Parser
- ARP Parser
- IPv4 Parser
- UDP Parser
- ITCH Parser

### DPDK

- Packet Receiver
- Packet Parsing
- Benchmarking

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
