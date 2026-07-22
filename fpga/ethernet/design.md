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


## Design Decision

The initial implementation uses a 64-bit streaming data interface.

This keeps the first version simple and easy to verify. The design should be structured so that future versions can support wider data paths such as 128-bit or 256-bit streams.







## Design Decision: Parser vs Dispatcher

The Ethernet Parser is responsible only for extracting Ethernet header fields.

It does not decide whether the payload should go to the IPv4 parser, ARP parser, or any future protocol parser.

Protocol routing will be handled by a separate Ethernet Dispatcher module.

This keeps the parser simple, testable, and focused on one responsibility.



## Design Decision: Streaming Interface

The Ethernet Parser uses an AXI4-Stream inspired interface.

The implementation is not tied to any FPGA vendor, but follows the same streaming philosophy:

- data
- valid
- ready
- last

This makes future integration with FPGA IP cores significantly easier.


## Design Decision: Shared Stream Interface

All FPGA packet-processing modules will use a shared AXI4-Stream inspired interface.

The common interface is defined in:

```text
fpga/interfaces/axis_stream_if.sv



