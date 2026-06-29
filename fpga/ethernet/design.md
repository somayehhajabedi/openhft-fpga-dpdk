## Purpose

The Ethernet Parser receives Ethernet frames from the MAC interface and extracts the Ethernet header fields required by the next protocol layer.

It forwards IPv4 or ARP packets to the corresponding parser.

## Inputs

- Clock
- Reset
- Data Stream
- Valid


## Outputs

- Destination MAC
- Source MAC
- EtherType
- Payload
- Header Valid


## Outputs

- Destination MAC
- Source MAC
- EtherType
- Payload
- Header Valid


## Assumptions

- Ethernet CRC has already been verified.
- Preamble and SFD are removed.
- Data arrives as a continuous stream.
- Stream width is 64 bits.

## Supported EtherTypes

| EtherType | Protocol |
|-----------|----------|
| 0x0800 | IPv4 |
| 0x0806 | ARP |




FSM:

IDLE

↓

READ_HEADER_2

↓

IDLE




| Clock | Data          | State         | Action                                             |
| ----- | ------------- | ------------- | -------------------------------------------------- |
| 1     | First 8 bytes | IDLE          | Read Destination MAC + first 2 bytes of Source MAC |
| 2     | Next 8 bytes  | READ_HEADER_2 | Complete Source MAC + EtherType                    |

