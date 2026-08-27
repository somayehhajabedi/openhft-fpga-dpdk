# Full End-to-End Trading Flow Benchmark

## Overview

This benchmark measures the current end-to-end trading flow from a normalized
market-data event through the strategy and order-entry path, across a real
loopback TCP connection to the exchange simulator, and back through OUCH
Accepted response decoding.

The benchmark is implemented in:

`benchmarks/full_flow_benchmark.cpp`

## Measured Flow

The measured path is:

MarketDataEvent
→ MarketDataPipeline (SPSC)
→ MarketDataBookConsumer
→ Local ArrayOrderBook
→ SimpleThresholdStrategy
→ StrategyEngine
→ OrderIntent
→ Gateway
→ RiskManager
→ OuchExecutionSink
→ OUCH EnterOrder encoding
→ TcpOuchTransport
→ Linux TCP loopback
→ ExchangeTcpServer
→ epoll
→ ExchangeOuchHandler
→ OUCH EnterOrder decoding
→ MatchingEngine
→ Exchange ArrayOrderBook
→ OUCH Accepted encoding
→ TCP response
→ TcpOuchTransport::receive()
→ OuchResponseDispatcher
→ Accepted decoding

This benchmark therefore exercises both the trading-system side and the
exchange-simulator side of the current architecture.

## Benchmark Workload

Each benchmark iteration processes:

- 500 market-data events
- Each event is an AddOrder
- Side: Sell
- Price: 10000
- Strategy threshold: 10000
- Strategy order quantity: 1
- Account ID: 1001

Every input event is intentionally constructed to trigger
`SimpleThresholdStrategy`.

Therefore, each benchmark iteration is expected to generate:

- 500 OrderIntent objects
- 500 successful risk checks
- 500 OUCH EnterOrder messages
- 500 exchange-side orders
- 500 OUCH Accepted responses
- 500 successfully decoded Accepted messages

The benchmark validates that all 500 orders complete the expected round trip.

## Benchmark Command

```bash
./build-release/benchmarks/full_flow_benchmark \
  --benchmark_min_time=1s \
  --benchmark_repetitions=1

Initial Result

Environment:

CPU reported by Google Benchmark: 8 × 4900 MHz
L1 Data: 32 KiB × 4
L1 Instruction: 32 KiB × 4
L2 Unified: 256 KiB × 4
L3 Unified: 8192 KiB
CPU scaling enabled

Observed result:

Benchmark                           Time             CPU   Iterations UserCounters
fullEndToEndFlow/real_time   65211409 ns     24460190 ns           23 items_per_second=7.66737k/s

The measured real time was approximately:

65.21 ms per 500-event benchmark iteration
7.67K events/orders per second

The corresponding amortized wall-clock cost is approximately:

65.21 ms / 500
≈ 130.4 us per event
Important Interpretation

The approximately 130 us/event value is NOT a direct single-order latency
measurement.

The benchmark currently submits a batch of 500 market-data events, waits for
the market-data pipeline to process the batch, and then receives and decodes
500 Accepted responses.

The benchmark iteration also includes setup and teardown work such as:

creation of trading-system components
creation of exchange components
TCP server startup
TCP connection establishment
exchange polling thread creation
market-data pipeline thread startup
thread shutdown and join
socket shutdown

Therefore, the current result should be interpreted primarily as an
end-to-end system throughput benchmark with an amortized per-event cost.

A separate latency benchmark is required for true per-order latency
measurements such as:

p50
p95
p99
p99.9
maximum latency
Comparison With In-Process Business Flow

An earlier benchmark measured the business flow without the real TCP exchange
round trip:

MarketDataEvent
→ SPSC
→ Local Order Book
→ Strategy
→ Gateway
→ RiskManager
→ OUCH encoding
→ in-memory benchmark transport

Observed median:

315518 ns / 500 events
≈ 631 ns/event

Observed throughput:

≈ 1.60 million events/second

The full TCP/exchange benchmark produced approximately:

≈ 7.67 thousand events/second
≈ 130.4 us/event amortized

These measurements are not directly equivalent latency measurements.

The difference includes TCP loopback, kernel networking, epoll processing,
additional thread scheduling, exchange-side OUCH decoding, matching-engine
processing, Accepted encoding, TCP response delivery, and client-side response
decoding.

Stack Overflow Issue Found During Benchmark Development

The first version of the full end-to-end benchmark crashed with SIGSEGV.

The benchmark originally created both:

ArrayOrderBook marketBook;
MatchingEngine matchingEngine(exchangeDispatcher);

on the benchmark thread's stack.

ArrayOrderBook contains two large fixed-size price-level arrays:

std::array<PriceLevel, 100000> bid_levels_;
std::array<PriceLevel, 100000> ask_levels_;

MatchingEngine also contains its own ArrayOrderBook.

The system stack limit was:

8192 KiB

The benchmark therefore placed two large order-book instances on an 8 MiB
stack, causing a stack overflow and SIGSEGV.

The benchmark harness was corrected by allocating the large objects on the
heap:

auto matchingEngine =
    std::make_unique<MatchingEngine>(
        exchangeDispatcher);

auto marketBook =
    std::make_unique<ArrayOrderBook>();

No production order-book or matching-engine architecture was changed.

After this change, the end-to-end benchmark completed successfully.

Current Benchmark Boundary

The benchmark currently begins at MarketDataEvent.

It does NOT yet include:

physical NIC receive
DPDK RX burst
Ethernet parsing
IPv4 parsing
UDP parsing
raw ITCH wire parsing

Therefore, "full end-to-end" currently means the complete trading and exchange
lifecycle from the normalized market-data event to the decoded OUCH Accepted
response.

Next Steps

Future benchmark work should include:

Run multiple repetitions and record median and variability.
Separate steady-state processing from setup/teardown.
Add true per-order round-trip latency measurement.
Report p50, p95, p99, p99.9, and maximum latency.
Extend the benchmark boundary backward to raw ITCH input.
Optionally benchmark the DPDK/NIC path separately.
Measure the Executed-response path in addition to Accepted responses.
Run benchmarks with CPU frequency scaling disabled and controlled CPU
affinity for more stable low-latency measurements.

