# Ethernet Parser

## Overview

The Ethernet Parser is the first stage of the FPGA processing pipeline.

Its responsibility is to receive an Ethernet frame and extract the information required by the next protocol layer.

---

## Input

Raw Ethernet Frame

---

## Output

- Destination MAC
- Source MAC
- EtherType
- Payload

---

## Supported EtherTypes

| EtherType | Protocol |
|-----------|----------|
| 0x0800 | IPv4 |
| 0x0806 | ARP |

---

## Pipeline

```
Ethernet Frame
        │
        ▼
Ethernet Parser
        │
        ▼
IPv4 / ARP
```

---

## Future Improvements

- CRC Verification
- VLAN Support
- Jumbo Frames
