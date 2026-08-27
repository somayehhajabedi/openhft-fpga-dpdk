# Full End-to-End Trading Flow Benchmark

This document describes the full application-level end-to-end benchmark for the
OpenHFT-FPGA-DPDK trading pipeline.

The benchmark is implemented in:

`benchmarks/full_flow_benchmark.cpp`

Two complementary benchmarks are provided:

- `fullEndToEndFlow`: batched end-to-end throughput benchmark
- `fullEndToEndLatency`: steady-state per-order round-trip latency benchmark

## Measured Flow

The measured application path is:

```text
MarketDataEvent
    ↓
MarketDataPipeline (SPSC)
    ↓
MarketDataBookConsumer
    ↓
Local ArrayOrderBook
    ↓
SimpleThresholdStrategy
    ↓
StrategyEngine
    ↓
OrderIntent
    ↓
Gateway
    ↓
RiskManager
    ↓
OuchExecutionSink
    ↓
OUCH EnterOrder encoding
    ↓
TcpOuchTransport
    ↓
Linux TCP loopback
    ↓
ExchangeTcpServer
    ↓
epoll
    ↓
ExchangeOuchHandler
    ↓
OUCH EnterOrder decoding
    ↓
MatchingEngine
    ↓
Exchange ArrayOrderBook
    ↓
OUCH Accepted encoding
    ↓
TCP response
    ↓
TcpOuchTransport::receive()
    ↓
OuchResponseDispatcher
    ↓
Accepted decoding
```

These benchmarks therefore exercise both the trading-system side and the
exchange-simulator side of the current architecture.

## Benchmark Workload

Each benchmark setup uses:

- 500 market-data events
- Event type: AddOrder
- Side: Sell
- Price: 10000
- Strategy threshold: 10000
- Strategy quantity: 1
- Account: 1001

Every input event is intentionally constructed to trigger
`SimpleThresholdStrategy`.

Therefore, a complete 500-event workload is expected to generate:

- 500 OrderIntent objects
- 500 successful risk checks
- 500 OUCH EnterOrder messages
- 500 exchange-side orders
- 500 OUCH Accepted responses
- 500 successfully decoded Accepted messages

The benchmark validates that all expected orders complete the application-level
round trip.

---

## Batched End-to-End Throughput Benchmark

### Benchmark

`fullEndToEndFlow`

This benchmark submits a batch of 500 market-data events, waits for the
market-data pipeline to process them, and then receives and decodes the
corresponding 500 OUCH Accepted responses.

### Command

```bash
./build-release/benchmarks/full_flow_benchmark \
  --benchmark_filter=fullEndToEndFlow \
  --benchmark_min_time=1s \
  --benchmark_repetitions=1
```

### Initial Result

Environment reported by Google Benchmark:

```text
CPU:            8 × 4900 MHz
L1 Data:        32 KiB × 4
L1 Instruction: 32 KiB × 4
L2 Unified:     256 KiB × 4
L3 Unified:     8192 KiB
CPU scaling:    enabled
```

Observed result:

```text
Benchmark                           Time             CPU   Iterations UserCounters
fullEndToEndFlow/real_time   65211409 ns     24460190 ns           23 items_per_second=7.66737k/s
```

The measured wall-clock result was approximately:

```text
65.21 ms per 500-event benchmark iteration
7.67K events/orders per second
```

The corresponding amortized wall-clock cost is approximately:

```text
65.21 ms / 500
≈ 130.4 µs per event
```

### Important Interpretation

The approximately `130.4 µs/event` value is **not** a direct single-order
latency measurement.

The benchmark submits a batch of 500 market-data events, waits for processing,
and then receives and decodes 500 Accepted responses.

The benchmark iteration also contains substantial setup and teardown work,
including:

- creation of trading-system components
- creation of exchange components
- TCP server startup
- TCP connection establishment
- exchange polling thread creation
- market-data pipeline thread startup
- thread shutdown and join
- socket shutdown

Therefore, this result should primarily be interpreted as an end-to-end system
throughput benchmark with an amortized per-event cost.

---

## Comparison With In-Process Business Flow

An earlier benchmark measured the business flow without the real TCP exchange
round trip:

```text
MarketDataEvent
    ↓
SPSC
    ↓
Local Order Book
    ↓
Strategy
    ↓
Gateway
    ↓
RiskManager
    ↓
OUCH encoding
    ↓
In-memory benchmark transport
```

Observed median:

```text
315518 ns / 500 events
≈ 631 ns/event
```

Observed throughput:

```text
≈ 1.60 million events/second
```

The full TCP/exchange throughput benchmark produced approximately:

```text
≈ 7.67 thousand events/second
≈ 130.4 µs/event amortized
```

These measurements are not directly equivalent latency measurements.

The difference includes Linux TCP loopback, kernel networking, epoll
processing, additional thread scheduling, exchange-side OUCH decoding,
matching-engine processing, Accepted encoding, TCP response delivery, and
client-side response decoding.

---

## Stack Overflow Issue Found During Benchmark Development

The first version of the full end-to-end benchmark crashed with `SIGSEGV`.

The benchmark originally created both:

```cpp
ArrayOrderBook marketBook;
MatchingEngine matchingEngine(exchangeDispatcher);
```

on the benchmark thread's stack.

`ArrayOrderBook` contains two large fixed-size price-level arrays:

```cpp
std::array<PriceLevel, 100000> bid_levels_;
std::array<PriceLevel, 100000> ask_levels_;
```

`MatchingEngine` also contains its own `ArrayOrderBook`.

The system stack limit was:

```text
8192 KiB
```

The benchmark therefore placed two large order-book instances on an 8 MiB
stack, causing stack overflow and `SIGSEGV`.

The benchmark harness was corrected by allocating these large benchmark
objects on the heap:

```cpp
auto matchingEngine =
    std::make_unique<MatchingEngine>(
        exchangeDispatcher);

auto marketBook =
    std::make_unique<ArrayOrderBook>();
```

No production order-book or matching-engine architecture was changed.

After this change, the end-to-end benchmark completed successfully.

---

## Steady-State End-to-End Latency Benchmark

### Benchmark

`fullEndToEndLatency`

This benchmark measures individual application-level round-trip latency after
system initialization.

Unlike the batched throughput benchmark, system initialization and teardown
are excluded from the per-order latency samples.

The exchange server, matching engine, TCP connection, market-data pipeline,
strategy, gateway, and risk manager are initialized before latency samples are
collected.

### Measurement Method

For each latency sample, exactly one market-data event is submitted and the
benchmark waits for the corresponding OUCH Accepted response before submitting
the next event.

The workload therefore behaves as:

```text
submit event 1 → receive Accepted 1
submit event 2 → receive Accepted 2
submit event 3 → receive Accepted 3
...
```

This intentionally avoids creating a queue backlog and provides an
uncontended sequential request/response measurement.

The latency interval begins immediately before:

```cpp
pipeline.submit(event)
```

and ends after the corresponding OUCH Accepted message has been received and
successfully decoded.

Latency samples are collected using:

```cpp
std::chrono::steady_clock
```

Each Google Benchmark iteration processes 500 orders. This preserves the
production RiskManager configuration instead of increasing its position limits
for benchmarking purposes.

Latency samples are retained across all Google Benchmark iterations within a
single repetition. Percentiles are calculated once from the complete aggregated
sample set after the repetition finishes.

Percentiles use the nearest-rank definition:

```text
rank  = ceil(p × N)
index = rank - 1
```

Sample collection uses fixed-size per-iteration storage so latency recording
does not allocate memory inside an individual measured round trip.

Sorting, percentile calculation, setup, teardown, and aggregation are excluded
from the individual latency intervals.

### Single-Repetition Validation

Command:

```bash
./build-release/benchmarks/full_flow_benchmark \
  --benchmark_filter=fullEndToEndLatency \
  --benchmark_min_time=1s \
  --benchmark_repetitions=1
```

Observed result:

```text
fullEndToEndLatency/real_time    9554765 ns   2910211 ns   123
items_per_second=52.3299k/s
max_ns=3.15801M
p50_ns=16.579k
p95_ns=21.478k
p99_ns=28.421k
p999_ns=53.015k
samples=61.5k
```

This repetition aggregated:

```text
123 iterations × 500 samples = 61,500 latency samples
```

The measured percentiles were:

| Metric | Latency |
|---|---:|
| p50 | 16.579 µs |
| p95 | 21.478 µs |
| p99 | 28.421 µs |
| p99.9 | 53.015 µs |
| max | 3.158 ms |

---

## Five-Repetition Stability Run

To evaluate measurement stability, the latency benchmark was run five times:

```bash
./build-release/benchmarks/full_flow_benchmark \
  --benchmark_filter=fullEndToEndLatency \
  --benchmark_min_time=1s \
  --benchmark_repetitions=5
```

Each repetition executed 124 Google Benchmark iterations and therefore
aggregated:

```text
124 × 500 = 62,000 latency samples
```

Across all five independent repetitions, approximately 310,000 round trips
were measured.

The individual repetition results were:

| Run | p50 | p95 | p99 | p99.9 | Max |
|---:|---:|---:|---:|---:|---:|
| 1 | 16.739 µs | 21.966 µs | 29.370 µs | 85.982 µs | 3.052 ms |
| 2 | 17.146 µs | 21.950 µs | 29.246 µs | 84.009 µs | 2.696 ms |
| 3 | 16.048 µs | 20.373 µs | 27.294 µs | 50.170 µs | 3.162 ms |
| 4 | 16.225 µs | 20.659 µs | 27.673 µs | 52.897 µs | 3.382 ms |
| 5 | 16.484 µs | 21.628 µs | 28.199 µs | 51.502 µs | 2.972 ms |

Google Benchmark reported the following median across the five repetitions:

| Metric | Median |
|---|---:|
| p50 | 16.484 µs |
| p95 | 21.628 µs |
| p99 | 28.199 µs |
| p99.9 | 52.897 µs |
| max | 3.052 ms |
| throughput | 53.653K items/s |

The coefficient of variation reported across the five repetitions was:

| Metric | CV |
|---|---:|
| p50 | 2.62% |
| p95 | 3.51% |
| p99 | 3.27% |
| p99.9 | 28.30% |
| max | 8.26% |
| throughput | 2.29% |

The p50 and p99 results are relatively stable across the five repetitions.

The p99 values ranged from:

```text
27.294 µs to 29.370 µs
```

The p99.9 result shows substantially more variation, which is expected for a
far-tail metric and should be interpreted more cautiously.

The five repetitions are independent distributions. The reported median
percentiles are the medians of the five per-repetition percentile values; they
are not percentiles calculated from one combined 310,000-sample distribution.

### Baseline Result

For the current benchmark environment, the median repetition is used as the
baseline:

```text
p50    ≈ 16.484 µs
p95    ≈ 21.628 µs
p99    ≈ 28.199 µs
p99.9  ≈ 52.897 µs
max    ≈ 3.052 ms

throughput ≈ 53.653K items/second
samples     = 62,000 per repetition
```

A concise description of the current result is:

> Across five benchmark repetitions of approximately 62K samples each, the
> median run measured approximately 16.5 µs p50 and 28.2 µs p99
> application-level end-to-end latency over Linux TCP loopback.

---

## What the Latency Measurement Includes

The measured latency includes:

- market-data pipeline processing
- SPSC queue handoff
- local order-book update
- strategy execution
- gateway processing
- risk checking
- OUCH EnterOrder serialization
- Linux TCP loopback
- epoll-based exchange processing
- OUCH EnterOrder decoding
- matching-engine processing
- exchange order-book processing
- OUCH Accepted encoding
- TCP response delivery
- client receive processing
- OUCH Accepted decoding

The result therefore represents an **application-level trading round trip**,
not the latency of an individual component such as the matching engine.

---

## Throughput vs. Latency

The two benchmarks answer different performance questions.

### `fullEndToEndFlow`

Measures the batched overall workload and includes substantial lifecycle,
setup, teardown, and batching overhead.

Initial result:

```text
65.2 ms / 500 events
≈ 130.4 µs amortized per event
≈ 7.67K events/second
```

This should **not** be interpreted as single-order latency.

### `fullEndToEndLatency`

Measures individual sequential request/response round trips after system
initialization.

Current five-repetition median baseline:

```text
p50    ≈ 16.5 µs
p95    ≈ 21.6 µs
p99    ≈ 28.2 µs
p99.9  ≈ 52.9 µs
```

This is the appropriate benchmark when discussing current steady-state
application-level end-to-end latency.

The reported `items_per_second` value for this benchmark corresponds to the
serialized latency workload. It is not a saturated-throughput measurement
because the benchmark deliberately waits for each response before submitting
the next request.

---

## Current Benchmark Boundary

The benchmarks currently begin at `MarketDataEvent`.

They do **not** include:

- physical NIC receive
- DPDK RX burst
- Ethernet parsing
- IPv4 parsing
- UDP parsing
- raw ITCH wire parsing
- external network latency

Therefore, "full end-to-end" currently means the trading and exchange lifecycle
from the normalized market-data event to the decoded OUCH Accepted response.

---

## Measurement Caveats

CPU frequency scaling was enabled during the measurements.

Google Benchmark reported:

```text
WARNING: CPU scaling is enabled, the benchmark real time measurements may be
noisy and will incur extra overhead.
```

The results should therefore be treated as a performance baseline for the
current benchmark environment rather than a production latency guarantee.

The benchmark uses Linux TCP loopback. This exercises the Linux kernel TCP
stack and the epoll-based exchange server but does not measure physical network
hardware.

The benchmark currently has no explicit warm-up phase, so early samples may
contain cold-start effects such as cache warming, page faults, or initial
scheduler behavior.

Each repetition contains approximately 62,000 samples, which provides a much
stronger p99 measurement than the original 500-sample implementation.

The p99.9 and maximum values remain more sensitive to operating-system noise
and rare scheduling or networking outliers. In the five-repetition run,
p99.9 had a substantially higher coefficient of variation than p50 or p99.

The maximum observed latency was approximately 2.7-3.4 ms across the five
runs. Because the benchmark runs on a general-purpose Linux environment with
CPU scaling enabled and uses multiple threads plus the kernel TCP stack, these
isolated maximum values should not be interpreted as representative
steady-state latency without additional profiling.

Two `steady_clock::now()` calls are part of each latency observation, so clock
measurement overhead is included in the reported values.

---

## Next Steps

Future benchmark work should include:

1. Disable CPU frequency scaling where practical.
2. Pin trading and exchange threads to dedicated CPU cores.
3. Add an explicit warm-up phase before collecting latency samples.
4. Measure `steady_clock::now()` overhead separately.
5. Investigate p99.9 and maximum outliers using `perf` and scheduler counters.
6. Check TCP configuration, including the effect of `TCP_NODELAY`.
7. Extend the measurement boundary backward to raw ITCH input.
8. Benchmark the DPDK/NIC receive path separately.
9. Measure the Executed-response path in addition to Accepted responses.
10. Establish repeatable performance-regression baselines for CI.