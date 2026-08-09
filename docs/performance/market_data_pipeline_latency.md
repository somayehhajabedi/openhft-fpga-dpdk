OpenHFT-FPGA-DPDK
Latency Measurement Report

Market Data Pipeline • p50 / p95 / p99 / p99.9 • Saturated vs. Uncontended

Prepared for low-latency architecture review and interview discussion

1. Goal

Measure end-to-end software-pipeline latency from the producer's submission point to EventConsumer::consume(), while separating normal/uncontended latency from latency caused by queue buildup under saturation.

2. Measured Path

Producer timestamp (t0)

MarketDataPipeline::submit()

SPSC ring-buffer transport

Dedicated worker thread / busy polling

Dispatcher

EventConsumer::consume() timestamp (t1)

Per-event latency is defined as:  latency = t1 - t0

3. Benchmark Design

3.1 Preallocated measurement storage

Timestamp and latency arrays are fixed-size and allocated before the measured path. This avoids vector growth or other dynamic allocation contaminating tail latency.

std::array<Clock::time_point, MessageCount> submitTimes;
std::array<std::uint64_t, MessageCount> latenciesNs;

3.2 Event-to-sample mapping

The benchmark uses orderId as a sequence number: orderId 1 maps to index 0, orderId 2 to index 1, and so on. The consumer uses that index to retrieve the corresponding submission timestamp.

3.3 Timestamp ordering

The submission timestamp is written before the event is successfully published to the SPSC queue. Writing it after submit() would create a race: the consumer could consume the event before the producer stores its timestamp. The queue's release/acquire publication establishes the required visibility for data written before publication.

3.4 Percentiles

After all events have been consumed, samples are sorted outside the measured hot path. The benchmark reports p50, p95, p99, p99.9, and maximum latency. Sorting is excluded from the timed section.

4. Results

Workload

Throughput

p50

p95

p99

p99.9

Max

Saturated

7.16 M events/s

586.335 µs

724.250 µs

728.657 µs

729.285 µs

732.190 µs

Uncontended

3.24 M events/s*

188 ns

218 ns

231 ns

249 ns

44.441 µs

*The uncontended benchmark intentionally waits for each event to be consumed before submitting the next event. Its items/s figure is therefore not a pipeline-throughput metric.

5. Interpretation

5.1 Uncontended latency

With no meaningful queue backlog, the measured path has a p50 of 188 ns and p99 of 231 ns. The main distribution is tight: p99 is only 43 ns above p50. A rare maximum of 44.441 µs exists despite p99.9 being 249 ns, indicating occasional large outliers that should be investigated separately.

5.2 Saturated latency

When the producer submits as quickly as possible, queue residency dominates end-to-end latency. The pipeline sustains about 7.16 million events/s in this run, but p99 rises to about 729 µs because events wait behind previously queued work.

5.3 Why the saturated result makes sense

At roughly 7.16 million events/s, the system advances at about 140 ns per event on average. A queue with roughly four thousand usable slots can therefore accumulate hundreds of microseconds of waiting time when near full. This is consistent with the observed saturated latency and shows that queueing delay, rather than only processing cost, can dominate end-to-end latency.

6. What These Benchmarks Do — and Do Not — Prove

The saturated test is an overload/queueing-latency measurement, not the baseline processing latency.

The uncontended test is a latency measurement, not a maximum-throughput test.

Clock::now(), atomic counters, benchmark instrumentation, OS scheduling, interrupts, and CPU migration can affect measurements.

The 231 ns p99 should therefore be described as the measured benchmark result, not as a universal production guarantee.

Shared CI runners should be used for smoke/regression checks, not authoritative low-latency numbers.

7. Next Performance Steps

Timestamp baseline: Measure back-to-back Clock::now() overhead so instrumentation cost is understood.

Outlier investigation: Investigate the gap between p99.9 = 249 ns and max = 44.441 µs using perf and scheduler/system counters.

CPU affinity: Pin producer and consumer threads and repeat measurements.

System noise: Track context switches, CPU migrations, page faults, interrupts, cache misses, and branch misses.

Warm-up: Add an explicit warm-up phase before collecting latency samples.

Repeated runs: Run multiple trials and compare percentile stability.

Regression checks: Store baseline percentiles and detect meaningful p99/p99.9 regressions.

Observability: Later export aggregated metrics to Prometheus/Grafana outside the hot path.

8. Interview Summary

Suggested explanation:

I separated saturated and uncontended latency because queueing delay can completely dominate end-to-end latency. In the uncontended benchmark, the market-data pipeline measured about 188 ns at p50 and 231 ns at p99. Under saturation, throughput was about 7.16 million events per second, while p99 increased to roughly 729 µs because events accumulated in the SPSC queue. I therefore treat processing latency, queueing latency, and throughput as separate measurements. I would next baseline timestamp overhead and use perf and system-level counters to investigate rare tail-latency outliers.

9. Key Takeaways

Average latency alone is insufficient for a low-latency system; tail percentiles matter.

p99 measurement and p99 diagnosis are different tasks.

Queue capacity can absorb bursts, but a deep queue can hide overload by converting it into latency.

Performance instrumentation must be designed so it does not itself create allocations or synchronization noise in the hot path.

Prometheus is appropriate later for aggregated observability; precise microbenchmark percentile measurement should remain local to the benchmark/histogram path.

OpenHFT-FPGA-DPDK • Market Data Pipeline Latency






perf stat \
  -e cycles,instructions,cache-references,cache-misses,branches,branch-misses,context-switches,cpu-migrations,page-faults \
  ./build-release/benchmarks/market_data_pipeline_latency_benchmark
2026-08-09T17:46:04-04:00
Running ./build-release/benchmarks/market_data_pipeline_latency_benchmark
Run on (8 X 4300.94 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.42, 0.48, 0.48
------------------------------------------------------------------------------------------------------------
Benchmark                                                  Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------------------------
BM_MarketDataPipelineLatency/real_time              10014774 ns      9993994 ns           76 items_per_second=9.98525M/s max_ns=467.708k p50_ns=407.365k p95_ns=450.22k p999_ns=467.439k p99_ns=466.177k
BM_MarketDataPipelineUncontendedLatency/real_time   24235452 ns     24218100 ns           30 items_per_second=4.12619M/s max_ns=44.85k p50_ns=155 p95_ns=170 p999_ns=193 p99_ns=177

 Performance counter stats for './build-release/benchmarks/market_data_pipeline_latency_benchmark':

    17,202,252,468      cycles                                                                  (66.93%)
    11,769,901,809      instructions                                                            (83.48%)
       811,333,377      cache-references                                                        (83.65%)
         4,566,235      cache-misses                                                            (83.61%)
     2,891,142,710      branches                                                                (83.58%)
        59,301,281      branch-misses                                                           (83.60%)
               284      context-switches                                                      
                 4      cpu-migrations                                                        
               613      page-faults                                                           

       2.290634620 seconds time elapsed

       4.105403000 seconds user
       0.014035000 seconds sys


