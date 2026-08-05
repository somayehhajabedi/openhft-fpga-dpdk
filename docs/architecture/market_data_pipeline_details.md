# P17.5-C — Market Data Pipeline Performance

## Overview

P17.5-C completes the performance-engineering phase of the Market Data
Pipeline.

The previous stages introduced:

- MarketDataEvent
- MarketDataPipeline
- Lock-free SPSC queue
- Dispatcher
- EventConsumer
- Worker-thread lifecycle
- Busy-polling execution
- End-to-end integration testing

P17.5-C focuses on measuring, documenting, and preparing the pipeline for
future performance optimization.

---

## Goals

The primary goals of P17.5-C are:

- Establish an end-to-end throughput baseline
- Measure the cost of the complete asynchronous pipeline
- Validate the busy-polling worker design
- Identify sources of runtime variability
- Prepare the pipeline for latency-percentile measurements
- Define future metrics and monitoring requirements

---

## Measured Data Path

The benchmark measures the complete path below:

```text
Producer Thread
      │
      ▼
MarketDataPipeline::submit()
      │
      ▼
Lock-Free SPSC Queue
      │
      ▼
Busy-Polling Worker Thread
      │
      ▼
Dispatcher
      │
      ▼
BenchmarkConsumer

Thread Model

The pipeline uses two execution contexts:

Thread	Responsibility
Producer	Submits normalized market-data events
Worker	Busy-polls the SPSC queue and dispatches events

The worker thread runs continuously while the pipeline is active:

while (running_.load(std::memory_order_acquire))
{
    static_cast<void>(
        dispatcher_.dispatch());
}

No sleeping or yielding is performed in the production worker loop.

This is an intentional low-latency design decision.

Why Busy Polling?

Busy polling avoids:

Scheduler wake-up latency
Condition-variable synchronization
Kernel-assisted blocking
Sleep overshoot
Additional context switches

The trade-off is that the worker thread may consume an entire CPU core even
when no events are available.

This trade-off is acceptable for a latency-sensitive HFT processing thread,
especially when the thread is later pinned to a dedicated core.

Benchmark Configuration
Property	Value
Event count	1,000,000
Queue type	Lock-free SPSC ring buffer
Queue capacity	4096
Producer threads	1
Consumer threads	1
Worker strategy	Busy polling
Repetitions	5
Minimum benchmark time	1 second
Build type	Debug
CPU frequency observed	approximately 3.3 GHz

Benchmark command:

./build/benchmarks/market_data_pipeline_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true \
    --benchmark_min_time=1s
Initial Results
Metric	Result
Mean elapsed time	216 ms
Median elapsed time	214 ms
Standard deviation	18.4 ms
Mean throughput	4.65 M events/s
Coefficient of variation	8.51%

Approximate amortized processing cost:

216 ms / 1,000,000 events
≈ 216 ns per event

This value is not a single-event latency measurement.

It is the average end-to-end cost per event under a high-volume throughput
workload.

Interpretation

The initial implementation processes approximately:

4.65 million events per second

The benchmark confirms that the full asynchronous pipeline operates correctly
under sustained load.

The result should be treated as an initial baseline rather than a final
performance claim because the benchmark was executed using a Debug build.

Debug builds may introduce:

Reduced compiler optimization
Additional checks
Less efficient inlining
Higher function-call overhead
Less effective vectorization and code generation

A Release build is required for meaningful final comparison.

Variability

The measured coefficient of variation was:

8.51%

Possible causes include:

CPU frequency scaling
Operating-system scheduling
Worker-thread CPU migration
Producer-thread CPU migration
Shared physical-core placement
Background system activity
Debug-build overhead
Lack of explicit thread affinity

Future CPU-affinity experiments will determine how much of this variability
can be removed.

Comparison with Queue-Only Benchmark

The Market Data Pipeline benchmark should not be compared directly with the
raw lock-free queue benchmark as if they measured the same operation.

The queue-only benchmark measures primarily:

Producer
   ↓
SPSC Queue
   ↓
Consumer

The pipeline benchmark additionally includes:

Pipeline API
Worker lifecycle
Dispatcher
EventConsumer abstraction
Consumer accounting
Shutdown coordination

Therefore, lower throughput than the isolated SPSC benchmark is expected.

Backpressure

MarketDataPipeline::submit() returns false when the queue is full.

The benchmark retries until capacity becomes available:

while (!pipeline.submit(event))
{
}

This creates bounded producer-side backpressure without:

Dynamic memory allocation
Unbounded queue growth
Mutex blocking
Condition-variable waiting

Future metrics will record how often this retry path is entered.

Current Metrics

The benchmark currently records:

Total elapsed time
Events processed per second
Mean
Median
Standard deviation
Coefficient of variation

The pipeline itself does not yet expose dedicated runtime metrics for:

Submitted events
Dispatched events
Submission failures
Queue depth
High-water mark
Worker-loop idle iterations
Shutdown drain count

These are planned additions.

Planned Runtime Metrics

P17.5-C prepares the design for the following counters:

Metric	Meaning
Submitted events	Successful queue submissions
Rejected submissions	Queue-full submission failures
Dispatched events	Events delivered to EventConsumer
Queue depth	Current queue occupancy
High-water mark	Maximum observed queue depth
Idle polls	Worker polls when the queue is empty
Shutdown-drained events	Events consumed during final drain

These metrics will later integrate with Prometheus and Grafana.

Future Benchmark Work

The next benchmark iterations should include:

Release Build
cmake -S . -B build-profile \
    -DCMAKE_BUILD_TYPE=Release
CPU Affinity

Pin:

Producer to one physical core
Worker to another physical core
Hardware Counters

Use:

perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses
Latency Distribution

Measure:

p50
p95
p99
p99.9
Maximum latency
Queue Pressure

Measure performance at different producer rates and queue occupancies.

Polling Strategy Comparison

Compare:

Busy polling
Yielding
Hybrid spin/yield
Sleep-based polling

Busy polling remains the default HFT-oriented mode.

Design Decisions
Fixed-Capacity Queue

A bounded queue prevents unpredictable memory growth and preserves deterministic
memory behavior.

SPSC Topology

The current architecture assumes:

One producer
One consumer

This allows a simpler and faster synchronization model than MPSC or MPMC.

Dedicated Worker Thread

The worker thread isolates downstream processing from the producer and allows
independent CPU placement.

No Allocation in the Hot Path

Events are transferred through preallocated queue storage.

Consumer Abstraction

The Dispatcher depends on EventConsumer, not directly on the Matching Engine.

This improves testability and keeps pipeline infrastructure independent from
downstream implementation details.

Validation

The following validation has been completed:

Item	Status
Build	Passed
Pipeline integration test	Passed
Full test suite	112/112 passed
Worker lifecycle	Implemented
Busy polling	Implemented
Graceful shutdown	Implemented
Final queue drain	Implemented
Initial throughput benchmark	Completed
Release benchmark	Pending
CPU-pinned benchmark	Pending
Latency percentiles	Pending
Runtime metrics	Pending
Conclusion

P17.5-C establishes the first end-to-end performance baseline for the Market
Data Pipeline.

The pipeline currently provides:

Lock-free event transport
Dedicated worker-thread execution
Busy-polling behavior
Bounded backpressure
Dispatcher-based event delivery
Thread-safe lifecycle management
End-to-end integration testing
Initial throughput measurement

The Debug-build baseline achieved approximately:

4.65 M events/s

The next performance step is to run the same benchmark in Release mode with
explicit CPU affinity and hardware-counter analysis.

Milestone Status

Milestone: P17.5-C — Market Data Pipeline Performance

Status: In Progress

Completed:

Initial benchmark
Busy-polling worker
Throughput baseline
Variability analysis

Remaining:

Release benchmark
CPU affinity
Runtime metrics
Latency percentiles
Final documentation update

///////////////////////////////////////////////

$ timeout 10s perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    ./build/benchmarks/market_data_pipeline_benchmark \
    --benchmark_filter=BM_MarketDataPipeline \
    --benchmark_min_time=0.2s
2026-08-04T14:02:18-04:00
Running ./build/benchmarks/market_data_pipeline_benchmark
Run on (8 X 4783.26 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.36, 0.47, 0.52
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------
BM_MarketDataPipeline/real_time        139 ms          139 ms            2 items_per_second=7.17767M/s

 Performance counter stats for './build/benchmarks/market_data_pipeline_benchmark --benchmark_filter=BM_MarketDataPipeline --benchmark_min_time=0.2s':

     3,490,422,673      cycles                                                                  (66.47%)
     1,405,748,679      instructions                                                            (83.21%)
       295,306,738      cache-references                                                        (83.33%)
           240,037      cache-misses                                                            (83.36%)
       179,709,423      branches                                                                (83.39%)
           945,124      branch-misses                                                           (83.46%)

       0.417194566 seconds time elapsed

       0.826349000 seconds user
       0.003992000 seconds sys

////////////////////////////////////////////////////////////////////



$ rm -rf build-release

$ cmake -S . -B build-release \
    -DCMAKE_BUILD_TYPE=Release

$ cmake --build build-release \
    --target market_data_pipeline_benchmark \
    -j$(nproc)


$ ./build-release/benchmarks/market_data_pipeline_benchmark \
    --benchmark_filter=BM_MarketDataPipeline \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true \
    --benchmark_min_time=1s
2026-08-04T14:26:27-04:00
Running ./build-release/benchmarks/market_data_pipeline_benchmark
Run on (8 X 4708.95 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.12, 0.94, 0.65


-------------------------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------
BM_MarketDataPipeline/real_time_mean          141 ms          141 ms            5 items_per_second=7.13117M/s
BM_MarketDataPipeline/real_time_median        138 ms          138 ms            5 items_per_second=7.25544M/s
BM_MarketDataPipeline/real_time_stddev       13.0 ms         13.0 ms            5 items_per_second=604.525k/s
BM_MarketDataPipeline/real_time_cv           9.24 %          9.21 %             5 items_per_second=8.48%

////////////////////////////////////////////////


## Release Benchmark Results

The benchmark executable and Google Benchmark dependency were both built
in Release mode.

| Metric | Result |
|---|---:|
| Mean elapsed time | 141 ms |
| Median elapsed time | 138 ms |
| Mean throughput | 7.13 M events/s |
| Median throughput | 7.26 M events/s |
| Standard deviation | 13.0 ms |
| Coefficient of variation | 9.24% |
| Approximate amortized cost | 141 ns/event |

The measurement represents end-to-end throughput rather than isolated
single-event latency.

/////////////////////////////////////////////////
$ perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    ./build-release/benchmarks/market_data_pipeline_benchmark \
    --benchmark_filter=BM_MarketDataPipeline \
    --benchmark_min_time=1s
2026-08-04T14:31:28-04:00
Running ./build-release/benchmarks/market_data_pipeline_benchmark
Run on (8 X 4793.37 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.17, 0.64, 0.63
------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------
BM_MarketDataPipeline/real_time        115 ms          115 ms           13 items_per_second=8.66951M/s

 Performance counter stats for './build-release/benchmarks/market_data_pipeline_benchmark --benchmark_filter=BM_MarketDataPipeline --benchmark_min_time=1s':

    13,748,519,721      cycles                                                                  (66.68%)
       777,561,966      instructions                                                            (83.37%)
     1,046,994,115      cache-references                                                        (83.34%)
           672,401      cache-misses                                                            (83.26%)
       143,483,812      branches                                                                (83.36%)
         4,018,972      branch-misses                                                           (83.37%)

       1.609201963 seconds time elapsed

       3.209856000 seconds user
       0.000999000 seconds sys


 ## Release Hardware-Counter Results

The Release benchmark achieved approximately **11.06 million events per
second**, corresponding to an amortized end-to-end cost of approximately
**90.4 ns per event**.

| Counter | Result |
|---|---:|
| Cycles | 20,264,887,057 |
| Instructions | 1,510,583,254 |
| Cache references | 1,508,795,911 |
| Cache misses | 1,025,301 |
| Branches | 283,287,773 |
| Branch misses | 903,010 |
| Cache-miss rate | approximately 0.068% |
| Branch-miss rate | approximately 0.32% |

The low cache-miss and branch-miss rates indicate good locality and predictable
control flow. The low process-wide IPC is primarily influenced by the dedicated
busy-polling worker thread, which intentionally consumes CPU cycles while
waiting for new events.      


 ////////////////////////////////////////

 $ perf stat \
    -e cycles,instructions,branches,branch-misses \
    ./build-release/benchmarks/market_data_pipeline_benchmark \
    --benchmark_filter=BM_MarketDataPipeline \
    --benchmark_min_time=1s


 $ perf stat \
    -e cache-references,cache-misses \
    ./build-release/benchmarks/market_data_pipeline_benchmark \
    --benchmark_filter=BM_MarketDataPipeline \
    --benchmark_min_time=1s     


## Split Hardware-Counter Measurements

To avoid hardware-counter multiplexing, branch and cache counters were
measured in separate runs.

### Branch Counters

| Counter | Result |
|---|---:|
| Cycles | 13,673,157,080 |
| Instructions | 774,264,695 |
| Branches | 142,399,109 |
| Branch misses | 4,340,790 |
| Branch-miss rate | approximately 3.05% |
| Throughput | 8.52 M events/s |

### Cache Counters

| Counter | Result |
|---|---:|
| Cache references | 1,497,568,250 |
| Cache misses | 619,034 |
| Cache-miss rate | approximately 0.041% |
| Throughput | 10.60 M events/s |

The measurements were collected in separate executions. CPU frequency and
scheduler placement varied between runs, so the throughput values should not
be compared as if all other conditions were identical.



        Producer / ITCH Mapper
                │
                ▼
        MarketDataPipeline::submit()
                │
                ▼
        Lock-Free SPSC Ring Buffer
                │
                ▼
        Dedicated Worker Thread
            (Busy Polling)
                │
                ▼
            Dispatcher
                │
                ▼
            EventConsumer
            ┌──┴──────────────┐
            ▼                 ▼
        TestConsumer   MatchingEngineConsumer


/////////////////////////////////////////////        



                NIC / DPDK
                    │
                    ▼
                Ethernet
                    │
                    ▼
                IPv4
                    │
                    ▼
                UDP
                    │
                    ▼
                ITCH Parser
                    │
                    ▼
                ITCH Mapper
                    │
                    ▼
                MarketDataPipeline
                    │
                    ▼
                Lock-Free Queue
                    │
                    ▼
                Worker Thread
                    │
                    ▼
                Dispatcher
                    │
                    ▼
                MatchingEngineConsumer
                    │
                    ▼
                Matching Engine
                    │
                    ▼
                Order Book








