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



## ITCH Order Delete (`D`)

The ITCH `Order Delete` message removes an existing order completely from the order book.

Unlike `Order Cancel (X)`, this message does not reduce the quantity partially. The entire remaining order is removed.

### Message fields

| Field | Description |
|---|---|
| Message Type | Always `D` |
| Order Reference Number | Identifies the order to remove |

### Processing flow

```text
OrderDeleteWireMessage
        ↓
OrderDeleteParser
        ↓
OrderDeleteMapper
        ↓
ITCHHandler::onOrderDelete()
        ↓
ArrayOrderBook::cancelOrder()
```

### Behavior

Given an existing order:

```text
Order ID: 2001
Side: Buy
Price: 12500
Quantity: 1000
```

after receiving:

```text
Message Type: D
Order ID: 2001
```

the order is removed completely from:

- its price level
- the order index
- the best bid or best ask calculation, when applicable

### Invalid operations

The operation returns `false` when:

- the order reference number does not exist
- the wire message is invalid
- the wire message is shorter than the required message size

No order-book state is modified when validation fails.

### Handler API

```cpp
bool onOrderDelete(
    const std::uint8_t* payload,
    std::size_t length);
```

### Order book API

```cpp
bool cancelOrder(OrderId id);
```

### Protocol mapping

```text
ITCH X → ArrayOrderBook::reduceOrder()
ITCH D → ArrayOrderBook::cancelOrder()
```

`Order Cancel (X)` supports partial quantity reduction.

`Order Delete (D)` removes the complete remaining order.

### Integration test scenario

```text
1. Add a buy order
2. Verify that bestBid() is not null
3. Send an Order Delete message
4. Verify that onOrderDelete() returns true
5. Verify that bestBid() becomes null
```
## ITCH Order Executed (`E`)

The ITCH `Order Executed` message reports that an existing order has been partially or fully executed.

### Message fields

| Field | Description |
|---|---|
| Message Type | Always `E` |
| Order Reference Number | Identifies the executed order |
| Executed Shares | Number of shares executed |
| Match Number | Identifies the execution event |

### Processing flow

```text
OrderExecutedWireMessage
        ↓
OrderExecutedParser
        ↓
OrderExecutedMapper
        ↓
ITCHHandler::onOrderExecuted()
        ↓
ArrayOrderBook::executeOrder()
        ↓
ArrayOrderBook::reduceOrder()
```

### Behavior

Given an existing order:

```text
Order ID: 3001
Side: Buy
Price: 12500
Quantity: 1000
```

and an execution message:

```text
Order ID: 3001
Executed Shares: 250
Match Number: 90001
```

the remaining order becomes:

```text
Order ID: 3001
Quantity: 750
```

The order keeps its price level and FIFO position when the execution is partial.

### Full execution

If the executed quantity equals the remaining order quantity, the order is removed completely from the order book.

### Invalid operations

The operation returns `false` when:

- the order does not exist
- the executed quantity is zero
- the executed quantity is greater than the remaining order quantity
- the wire message is invalid or too short

No order-book state is modified when validation fails.

### Order book API

```cpp
bool executeOrder(
    OrderId id,
    Quantity executedQuantity);
```

### Handler API

```cpp
bool onOrderExecuted(
    const std::uint8_t* payload,
    std::size_t length);
```

### Match number handling

The `matchNumber` is parsed and mapped, but it is not stored inside the order book.

It belongs to the execution event and will later be forwarded to:

```text
Trade Event
    ↓
Publisher
    ↓
Journal / Analytics
```

### Integration test scenario

```text
1. Add a buy order with quantity 1000
2. Verify the order exists in bestBid()
3. Send an Order Executed message for 250 shares
4. Verify that onOrderExecuted() returns true
5. Verify the remaining quantity is 750
6. Verify the order ID and price level remain unchanged
```


## ITCH Order Replace (`U`)

The ITCH `Order Replace` message replaces an existing order with a new order reference number, quantity, and price.

The original order is removed from the order book, and the replacement order is inserted as a new order.

### Message fields

| Field | Description |
|---|---|
| Message Type | Always `U` |
| Original Order Reference Number | Identifies the existing order |
| New Order Reference Number | Identifies the replacement order |
| Shares | New order quantity |
| Price | New order price |

### Processing flow

```text
OrderReplaceWireMessage
        ↓
OrderReplaceParser
        ↓
OrderReplaceMapper
        ↓
ITCHHandler::onOrderReplace()
        ↓
ArrayOrderBook::replaceOrder()
        ↓
Remove original order
        ↓
Insert replacement order
```

### Behavior

Given an existing order:

```text
Order ID: 4001
Side: Buy
Price: 12500
Quantity: 1000
```

and a replace message:

```text
Original Order ID: 4001
New Order ID: 4002
New Price: 12750
New Quantity: 800
```

the original order is removed and replaced with:

```text
Order ID: 4002
Side: Buy
Price: 12750
Quantity: 800
```

The side and account information are preserved from the original order.

### Queue priority

The replacement order does not keep the original order's FIFO priority.

It is inserted as a new order at the end of its target price level.

### Invalid operations

The operation returns `false` when:

- the original order does not exist
- the new order reference number is zero
- the new quantity is zero
- the new order reference number is equal to the original reference number
- the new order reference number already exists
- the wire message is invalid or too short

No order-book state is modified when validation fails.

### Order book API

```cpp
bool replaceOrder(
    OrderId originalOrderId,
    OrderId newOrderId,
    Quantity newQuantity,
    Price newPrice);
```

### Handler API

```cpp
bool onOrderReplace(
    const std::uint8_t* payload,
    std::size_t length);
```

### Integration test scenario

```text
1. Add an order with ID 4001, price 12500, and quantity 1000
2. Verify that it is the current best bid
3. Send an Order Replace message
4. Replace it with ID 4002, price 12750, and quantity 800
5. Verify that onOrderReplace() returns true
6. Verify that the new order is the best bid
7. Verify the new order ID, price, and quantity



```


### Responsibilities

#### ItchReplayReader

- Opens a binary ITCH replay file.
- Reads one length-prefixed ITCH message at a time.
- Validates message length.
- Detects truncated messages.
- Returns raw message bytes.

#### ItchReplayDispatcher

Routes replay messages to the appropriate ITCH handler.

Supported message types:

| Type | Message |
|------|----------|
| A | Add Order |
| X | Order Cancel |
| D | Order Delete |
| E | Order Executed |
| U | Order Replace |

Unknown message types are rejected.

### Tests

Replay functionality is validated by:

- ItchReplayReaderTest
- ItchReplayDispatcherTest
- ItchReplayPipelineTest

The pipeline test validates:

Replay File
→ Reader
→ Dispatcher
→ ITCHHandler
→ OrderBook

