# End-to-End Trading Flow

## Overview

OpenHFT-FPGA-DPDK is a low-latency trading system implemented in modern C++.

The current architecture separates market-data ingestion, order-book maintenance,
strategy execution, risk validation, and exchange connectivity.

The end-to-end flow is:

    NIC / Replay
        |
        v
    Ethernet
        |
        v
    IPv4
        |
        v
    UDP
        |
        v
    ITCH
        |
        v
    MarketDataEvent
        |
        v
    SPSC Ring Buffer
        |
        v
    MarketDataPipeline
        |
        v
    MarketDataStrategyConsumer
        |
        +--------------------+
        |                    |
        v                    v
    Order Book          StrategyEngine
                             |
                             v
                           Strategy
                             |
                             v
                         OrderIntent
                             |
                             v
                           Gateway
                             |
                             v
                        RiskManager
                             |
                             v
                    OuchExecutionSink
                             |
                             v
                         EnterOrder
                             |
                             v
                        OuchEncoder
                             |
                             v
                    TcpOuchTransport
                             |
                             v
                          Exchange
                             |
                             v
                       OUCH Response
                             |
                             v
                 OuchResponseDispatcher
                    /     /     \     \
                   v     v       v     v
             Accepted Rejected Executed Canceled


## 1. Market Data Ingestion

Market data enters the system either from a network interface or from the replay
infrastructure used for testing and deterministic reproduction.

The network protocol stack is processed in stages:

    Ethernet
       |
       v
    IPv4
       |
       v
    UDP
       |
       v
    ITCH

Each layer is handled by a dedicated parser.

Keeping the protocol layers separated makes the packet-processing pipeline easier
to test and allows individual components to be optimized independently.


## 2. ITCH Parsing and Normalization

ITCH messages represent market activity such as:

- Add Order
- Cancel Order
- Delete Order
- Execute Order
- Replace Order

Wire-format structures are decoded by the ITCH parsers and converted into domain
models.

The protocol-specific representation is then normalized into:

    MarketDataEvent

MarketDataEvent provides a common internal representation containing fields such
as:

    type
    orderId
    newOrderId
    accountId
    side
    symbol
    price
    quantity

This prevents downstream components from depending directly on the ITCH wire
format.


## 3. SPSC Thread Boundary

MarketDataEvent objects are transferred between the producer and consumer threads
using a Single-Producer Single-Consumer ring buffer.

    Market Data Producer
            |
            v
    MarketDataEvent
            |
            v
       SPSC Queue
            |
            v
    Consumer Thread

The SPSC queue provides a lock-free communication boundary between the two
threads.

This avoids mutex contention on the market-data hot path and provides predictable
memory usage.


## 4. Market Data Pipeline

The MarketDataPipeline owns the queue used to transport normalized events.

The producer submits MarketDataEvent objects to the queue.

A consumer worker processes events from the opposite side and forwards them to
the configured market-data consumer.

Queue metrics track behavior such as:

- successful pushes
- failed pushes
- successful pops
- failed pops
- high-water mark


## 5. MarketDataStrategyConsumer

MarketDataStrategyConsumer coordinates two operations for every market-data event.

First:

    MarketDataEvent
          |
          v
    MarketDataBookConsumer
          |
          v
       Order Book

The local order book is updated.

Then:

    MarketDataEvent
          |
          v
     StrategyEngine
          |
          v
        Strategy

This ordering ensures the local market state is updated before strategy
processing continues.


## 6. Order Book

The software order book maintains local market state.

The implementation includes:

- preallocated OrderPool
- price-level storage
- intrusive FIFO order lists
- order lookup through FixedHashMap
- bitmap-assisted best bid / best ask lookup

The design avoids unnecessary dynamic allocation in latency-sensitive paths.

The order book supports operations including:

- add
- cancel
- delete
- execute
- replace


## 7. Strategy

Strategy logic is isolated from protocol and execution infrastructure.

The current SimpleThresholdStrategy is intentionally simple and primarily
demonstrates the strategy framework.

For example, a sell-side AddOrder event at or below a configured threshold may
produce a buy decision.

The strategy produces:

    OrderIntent

rather than directly sending an exchange order.

An OrderIntent describes the desired action using fields such as:

    accountId
    side
    symbol
    price
    quantity

This separation keeps trading decisions independent from exchange connectivity.


## 8. Gateway and Risk Management

OrderIntent objects are submitted to the Gateway.

The Gateway invokes the RiskManager before execution.

    OrderIntent
        |
        v
      Gateway
        |
        v
    RiskManager

Risk checks include validation of constraints such as:

- non-zero price
- non-zero quantity
- maximum order quantity
- maximum order value
- cumulative position limits

Rejected orders stop at the risk boundary and are never submitted to the
execution transport.

Orders that pass risk validation are forwarded through the OrderExecutionSink
abstraction.


## 9. OUCH Execution

OuchExecutionSink implements the OrderExecutionSink interface.

It converts an internal OrderIntent into an OUCH EnterOrder message.

    OrderIntent
        |
        v
    OuchExecutionSink
        |
        v
      EnterOrder

The OUCH representation contains exchange-facing fields including:

- UserRefNum
- Side
- Quantity
- Symbol
- Price
- TimeInForce
- Display
- Capacity
- ISO eligibility
- CrossType
- ClOrdID


## 10. OUCH Encoding

OuchEncoder converts the EnterOrder structure into its binary wire
representation.

Numeric fields are explicitly encoded in network byte order.

Fixed-size buffers are used so encoding does not require dynamic allocation on
the execution path.

    EnterOrder
        |
        v
    OuchEncoder
        |
        v
    Wire Bytes


## 11. TCP Transport

TcpOuchTransport sends the encoded OUCH message to the exchange over TCP.

The transport layer is deliberately separated from the encoder.

    OUCH Wire Message
            |
            v
    TcpOuchTransport
            |
            v
         Exchange

This separation allows transport behavior to be tested independently from
protocol encoding.


## 12. OUCH Response Processing

Exchange responses are received as binary OUCH messages.

OuchResponseDispatcher examines the message type and routes the message to the
appropriate decoder.

Supported response types currently include:

    A -> Accepted
    J -> Rejected
    E -> Executed
    C -> Canceled

Each decoder validates the message and converts the wire representation into a
typed domain structure.

The response path is therefore:

    Exchange
        |
        v
    Raw OUCH Bytes
        |
        v
    OuchResponseDispatcher
        |
        +---- Accepted
        |
        +---- Rejected
        |
        +---- Executed
        |
        +---- Canceled


## 13. Threading Model

The market-data path separates packet/event production from downstream processing
using an SPSC queue.

The intended high-level model is:

    Producer Core
        |
        | MarketDataEvent
        v
    SPSC Ring Buffer
        |
        v
    Consumer / Strategy Core

This allows producer and consumer execution to progress independently while
avoiding a mutex-protected shared queue.


## 14. Low-Latency Design Principles

The current implementation follows several low-latency design principles:

- lock-free SPSC communication
- preallocated order storage
- fixed-size protocol buffers
- explicit binary protocol parsing
- no unnecessary heap allocation on critical paths
- single-writer order-book design
- cache-conscious data structures
- bitmap-assisted price-level lookup
- separation of protocol, strategy, risk, and transport layers
- independent latency and throughput benchmarks
- NUMA and CPU-affinity utilities


## 15. Testing

The project contains unit and integration coverage across the major components.

Current tested areas include:

- Ethernet parsing
- IPv4 parsing
- ITCH parsing
- ITCH mapping
- ITCH replay
- FixedHashMap
- OrderPool
- ArrayOrderBook
- MatchingEngine
- SPSC ring buffer
- queue metrics
- strategy
- risk management
- gateway
- OUCH encoding
- OUCH execution
- OUCH decoding
- OUCH response dispatch
- TCP OUCH transport
- end-to-end gateway-to-OUCH integration

At this architecture checkpoint:

    130 / 130 tests pass.


## 16. Current Boundary and Next Steps

The current implementation establishes the core end-to-end architecture from
market-data ingestion through strategy decisions and outbound OUCH execution.

Future work can extend the system with:

- complete OUCH session handling
- TCP receive loop and message framing
- order lifecycle/state management
- outstanding-order correlation
- position and PnL updates from executions
- reconnect and recovery handling
- backpressure policies
- additional production strategies
- detailed end-to-end latency instrumentation
- DPDK production receive path integration
- FPGA acceleration of selected packet-processing stages
