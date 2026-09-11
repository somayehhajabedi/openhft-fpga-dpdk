# OUCH TLS Round-Trip Latency Benchmark

## Purpose

This benchmark measures the steady-state application-level round-trip
latency of the OUCH order-entry path over TLS.

The measured path is:

Client
→ OUCH EnterOrder encode
→ TLS send
→ TCP loopback
→ ExchangeTcpServer
→ ExchangeOuchHandler
→ MatchingEngine
→ OUCH Accepted encode
→ TLS send
→ TCP loopback
→ client receive
→ OuchResponseDispatcher

The TLS handshake is intentionally excluded from the timed region.

The purpose of this benchmark is to establish a baseline for future
latency optimizations such as CPU affinity, busy polling, socket tuning,
reduced context switching, and transport-level optimizations.

---

## Benchmark Architecture

The benchmark runs the client and exchange server inside the same
process using separate threads.

The communication path still uses a real TCP socket over the loopback
interface and a real TLS connection.

```text
Benchmark Process
│
├── Client Thread
│   │
│   ├── OuchEncoder
│   ├── TlsOuchTransport
│   │       │
│   │       │ TLS 1.3 / TCP loopback
│   │       ▼
│   ├── receive()
│   └── OuchResponseDispatcher
│
└── Exchange Thread
    │
    └── ExchangeTcpServer
            │
            ├── epoll
            ├── TLS
            ├── ExchangeOuchHandler
            └── MatchingEngine

The ExchangeTcpServer event loop is driven by a dedicated
std::thread.

epoll performs I/O multiplexing; it does not create the server thread.

TLS Configuration

The benchmark negotiated:

TLS version: TLSv1.3
TLS cipher: TLS_AES_256_GCM_SHA384

The TLS connection is established once before the benchmark begins.

TLS handshake latency is therefore not included in the reported
per-order latency.

Order Flow

Each benchmark operation sends an OUCH EnterOrder message and waits
for the corresponding OUCH Accepted response.

The measured interval is approximately:

start
  ↓
OUCH encode
  ↓
TLS send
  ↓
Exchange processing
  ↓
OUCH Accepted
  ↓
TLS receive
  ↓
OUCH response dispatch
  ↓
stop

The benchmark therefore measures application-level order-entry
round-trip latency rather than raw network latency.

Order Book Management

The matching engine has finite order storage.

Sending only resting Buy orders eventually fills the order storage and
causes the benchmark to fail.

During initial testing this occurred at:

userRefNum = 4097

after approximately 4096 live orders had accumulated.

To prevent order accumulation, the benchmark alternates orders:

Buy  25 @ 101
Sell 25 @ 101
Buy  25 @ 101
Sell 25 @ 101
...

Each Sell can match the preceding Buy, preventing continuous growth of
the live order population.

Warmup

Before measurement, the benchmark performs:

1,000 round trips

These iterations are excluded from the latency statistics.

Warmup reduces startup effects related to:

TLS internal state
socket buffers
instruction cache
data cache
branch predictors
matching-engine initialization
operating-system startup effects
Measurement

Measured samples:

100,000

Timing uses:

std::chrono::steady_clock

Each sample measures one complete OUCH request/response round trip.

The samples are sorted after measurement to calculate latency
percentiles.

Baseline Results

Release build baseline:

Metric	Result
Samples	100,000
Minimum	16.015 µs
Mean	27.507 µs
p50	25.077 µs
p95	37.381 µs
p99	44.838 µs
p99.9	97.058 µs
Maximum	1.691 ms
Throughput	36.319K round-trips/sec

Raw benchmark output:

OUCH TLS Round-Trip Benchmark
-----------------------------
Samples:    100000
Min:        16015 ns
Mean:       27506.6 ns
p50:        25077 ns
p95:        37381 ns
p99:        44838 ns
p99.9:      97058 ns
Max:        1690678 ns
Throughput: 36319.3 round-trips/sec
Interpretation

The median application-level TLS round-trip latency is approximately:

25 µs

99% of measured round trips complete within approximately:

45 µs

99.9% complete within approximately:

97 µs

The maximum observed latency was approximately:

1.69 ms

The maximum should not be interpreted as normal steady-state latency.

Large isolated outliers can be caused by operating-system scheduling,
context switches, interrupts, page activity, or other system noise.

For low-latency analysis, the latency distribution and especially
p99/p99.9 are more informative than a single maximum observation.

Important Limitations

This benchmark is not a physical network latency measurement.

Both client and exchange run on the same machine and communicate using
the loopback interface.

Therefore the results do not include:

physical NIC latency
switch latency
network propagation
remote exchange latency
cross-machine clock effects

The benchmark does include:

OUCH encoding
TLS encryption/decryption
TCP socket processing
kernel networking
thread scheduling
epoll-based exchange processing
OUCH decoding
matching-engine processing
Accepted encoding
response decoding and dispatch
Baseline Status

This result should be treated as the pre-optimization TLS round-trip
baseline.

Future optimization results should be compared against this baseline,
especially:

p50   = 25.077 µs
p99   = 44.838 µs
p99.9 = 97.058 µs

The benchmark should be rerun several times when evaluating changes to
avoid drawing conclusions from a single noisy run.

Planned Experiments

Potential follow-up experiments include:

Pin client and exchange threads to dedicated CPU cores.
Replace timeout-based polling with busy polling.
Tune TCP socket options.
Measure plain TCP versus TLS.
Measure exchange-core latency separately from transport latency.
Measure TLS encryption/decryption overhead.
Investigate p99 and p99.9 scheduler noise.
Run client and exchange on separate physical machines.
Compare loopback TCP/TLS with kernel-bypass networking.
Track benchmark results over time for latency regressions.


