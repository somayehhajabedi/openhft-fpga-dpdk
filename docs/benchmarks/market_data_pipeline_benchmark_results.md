# Market Data Pipeline Benchmark Results

## Overview

This benchmark measures the end-to-end throughput of the Market Data Pipeline.

The benchmark includes the complete processing path:

```
Producer
    │
    ▼
MarketDataPipeline::submit()
    │
    ▼
Lock-Free SPSC Queue
    │
    ▼
Worker Thread (Busy Polling)
    │
    ▼
Dispatcher
    │
    ▼
EventConsumer
```

Unlike queue-only benchmarks, this benchmark measures the complete software
pipeline including producer submission, queue transport, dispatcher execution,
and consumer processing.

---

# Benchmark Configuration

| Property | Value |
|----------|-------|
| Queue Type | Lock-Free SPSC Ring Buffer |
| Queue Capacity | 4096 |
| Producer Threads | 1 |
| Consumer Threads | 1 |
| Polling Strategy | Busy Polling |
| Events | 1,000,000 |
| Build Type | Debug |

---

# Results

| Metric | Value |
|---------|------:|
| Mean Time | 216 ms |
| Median Time | 214 ms |
| Throughput | 4.65 M events/s |
| Coefficient of Variation | 8.51 % |

---

# Interpretation

The benchmark demonstrates that the initial Market Data Pipeline is capable of
processing approximately **4.65 million events per second** on the current
development machine.

The reported throughput represents the complete end-to-end software pipeline,
including:

- Event submission
- Lock-free queue transfer
- Worker-thread scheduling
- Dispatcher execution
- Consumer processing

It does **not** represent raw queue throughput.

---

# Notes

The benchmark was executed using a **Debug** build.

Debug builds disable many compiler optimizations and therefore should not be
used for performance evaluation.

A Release benchmark will be added later for comparison.

---

# Future Work

Planned benchmark improvements include:

- Release build measurements
- CPU affinity
- NUMA-aware execution
- Latency histogram (P50/P95/P99)
- Queue depth statistics
- Cache miss analysis
- Hardware performance counters
- Multi-producer pipeline evaluation

---

# Conclusion

This benchmark establishes the first end-to-end performance baseline for the
Market Data Pipeline.

Future optimization work will compare against this baseline to quantify
performance improvements introduced by CPU affinity, NUMA awareness, cache
optimization, and dispatcher enhancements.