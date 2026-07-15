# ITCH Parser

## Overview

The ITCH parser processes the application payload extracted from UDP packets.

The current implementation supports:

- Reading the ITCH message type
- Detecting Add Order messages (`A`)
- Returning a pointer to the bytes following the ITCH header
- Rejecting null or empty input safely

---

## Current Header

```cpp
struct ITCHHeader
{
    char message_type;
};
```

The first byte identifies the ITCH message type.

---

## Public API

```cpp
class ITCHParser
{
public:
    static const ITCHHeader* parse(
        const std::uint8_t* data,
        std::size_t length);

    static char messageType(
        const ITCHHeader* header);

    static bool isAddOrder(
        const ITCHHeader* header);

    static const std::uint8_t* payload(
        const ITCHHeader* header);
};
```

---

## Parsing Strategy

The parser uses zero-copy parsing.

It does not copy the UDP payload. Instead, it returns a pointer into the original packet buffer.

Before parsing, it checks:

- The input pointer is not null
- The input length is at least the size of `ITCHHeader`

---

## Unit Tests

- Parses the message type
- Rejects a null buffer
- Rejects an empty message
- Detects an Add Order message
- Returns the payload pointer

---

## Next Step

Implement the complete Add Order message parser.
