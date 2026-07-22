# AddOrder Mapper

## Overview

The mapper layer converts protocol-specific messages into protocol-independent domain models.

This separation keeps the Order Book independent from the NASDAQ ITCH binary format.

---

## Data Flow

```text
UDP Payload
      │
      ▼
AddOrderWireMessage
      │
      ▼
AddOrderParser
      │
      ▼
AddOrderMapper
      │
      ▼
AddOrder
      │
      ▼
Order Book
```

---

## Responsibilities

### AddOrderWireMessage

Represents the binary layout defined by the NASDAQ ITCH specification.

### AddOrderParser

- Validates packet size
- Parses the wire format
- Converts network byte order to host byte order

### AddOrderMapper

Converts protocol-specific structures into domain models.

### AddOrder

Represents the business object used by the Order Book.

---

## Design Goals

- Separation of concerns
- Protocol independence
- Easier testing
- Maintainability
- Extensibility
