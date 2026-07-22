## ITCH Order Cancel (`X`)

The ITCH `Order Cancel` message reduces the remaining quantity of an existing order.

Unlike `Order Delete (D)`, this message may partially reduce an order without removing it completely from the order book.

### Message fields

| Field | Description |
|---|---|
| Message Type | Always `X` |
| Order Reference Number | Identifies the existing order |
| Cancelled Shares | Number of shares to remove from the order |

### Processing flow

```text
OrderCancelWireMessage
        ↓
OrderCancelParser
        ↓
OrderCancelMapper
        ↓
ITCHHandler::onOrderCancel()
        ↓
ArrayOrderBook::reduceOrder()
```

### Behavior

Given an existing order:

```text
Order ID: 1001
Side: Buy
Price: 12500
Quantity: 1000
```

and an incoming cancel message:

```text
Order ID: 1001
Cancelled Shares: 300
```

the remaining order becomes:

```text
Order ID: 1001
Quantity: 700
```

The order keeps its original price level and FIFO position because only its quantity changes.

### Full quantity cancellation

If the cancelled quantity is equal to the remaining quantity:

```text
Current Quantity: 1000
Cancelled Shares: 1000
```

the order is removed completely from:

- the price level
- the order index
- the best bid or best ask calculation, when applicable

Internally, `reduceOrder()` delegates this case to `cancelOrder()`.

### Invalid operations

The operation fails and returns `false` when:

- the order reference number does not exist
- the cancelled quantity is zero
- the cancelled quantity is greater than the remaining order quantity
- the wire message is invalid or shorter than the required message size

No order-book state is modified when validation fails.

### Order book API

```cpp
bool reduceOrder(
    OrderId id,
    Quantity cancelledQuantity);
```

### Handler API

```cpp
bool onOrderCancel(
    const std::uint8_t* payload,
    std::size_t length);
```

### Protocol mapping

```text
ITCH X → ArrayOrderBook::reduceOrder()
ITCH D → ArrayOrderBook::cancelOrder()
```

`Order Cancel (X)` supports partial quantity reduction.

`Order Delete (D)` removes the complete order regardless of its remaining quantity.

### Integration test scenario

The integration test performs the following sequence:

```text
1. Add a buy order with quantity 1000
2. Verify that the order exists in bestBid()
3. Send an Order Cancel message for 300 shares
4. Verify that onOrderCancel() returns true
5. Verify that the remaining quantity is 700
6. Verify that the order ID and price level remain unchanged
```

Expected result:

```text
Initial Quantity: 1000
Cancelled Shares: 300
Remaining Quantity: 700
```

