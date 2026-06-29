# Data Flow

## Overview

This document describes how market data flows through the OpenHFT FPGA DPDK system.

The project contains two processing paths:

- FPGA Pipeline
- DPDK Software Pipeline

Both pipelines eventually update the same software order book.

---

# FPGA Pipeline

```
Exchange
    │
    ▼
Ethernet Frame
    │
    ▼
Ethernet Parser
    │
    ▼
ARP / IPv4
    │
    ▼
UDP Parser
    │
    ▼
ITCH Parser
    │
    ▼
Decoded Market Message
    │
    ▼
Software Order Book
```

---

## Ethernet Parser

Input

```
Raw Ethernet Frame
```

Output

```
Payload
EtherType
Source MAC
Destination MAC
```

---

## IPv4 Parser

Input

```
IPv4 Packet
```

Output

```
Protocol
Source IP
Destination IP
Payload
```

---

## UDP Parser

Input

```
UDP Packet
```

Output

```
Source Port
Destination Port
Payload
```

---

## ITCH Parser

Input

```
ITCH Message Stream
```

Output

```
Decoded Market Messages

Add Order
Modify Order
Delete Order
Execute Order
Trade
```

---

## Order Book

Receives decoded ITCH messages.

Updates

- Bid Levels
- Ask Levels
- Individual Orders

---

# DPDK Pipeline

```
NIC

↓

DPDK RX Queue

↓

Packet Parsing

↓

ITCH Parser

↓

Software Order Book
```

---

# Benchmark Flow

```
Packet

↓

Timestamp

↓

Parser

↓

Timestamp

↓

Order Book

↓

Timestamp

↓

Latency Report
```

---

# Future Hardware Flow

```
NIC

↓

FPGA

↓

Hardware Order Book

↓

Hardware Strategy
```
