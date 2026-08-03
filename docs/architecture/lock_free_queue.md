P17 — Lock-Free SPSC Queue

Highlights

- Lock-Free SPSC Ring Buffer
- Acquire/Release Memory Ordering
- Runtime Queue Metrics
- High Water Mark Tracking
- 111 Unit Tests
- Google Benchmark
- perf Hardware Counter Analysis
- Bounded Mutex Comparison
- Architecture Documentation
- Benchmark Documentation




1. Motivation

2. Design

3. Memory Layout

4. Memory Ordering

5. Cache Behavior

6. False Sharing Prevention

7. Backpressure

8. Monitoring

9. Benchmarks

10. Future Work


cmake --build build -j$(nproc)

ctest --test-dir build --output-on-failure






cmake -S . -B build-profile \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build-profile -j$(nproc)

./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_min_time=1s



./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_min_time=1s
2026-08-03T09:50:56-04:00
Running ./build-profile/benchmarks/lockfree_queue_benchmark
Run on (8 X 4299.7 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.33, 0.38, 0.41
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------
BM_MutexQueue/real_time           181 ms        0.177 ms            8 items_per_second=5.53339M/s
BM_LockFreeQueue/real_time       32.0 ms        0.059 ms           45 items_per_second=31.2276M/s


//////////////////////////////////////////////

perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    ./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_filter=BM_MutexQueue
2026-08-03T09:53:12-04:00
Running ./build-profile/benchmarks/lockfree_queue_benchmark
Run on (8 X 4688.21 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.67, 1.06, 0.65
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations UserCounters...
----------------------------------------------------------------------------------
BM_MutexQueue/real_time        196 ms        0.263 ms            4 items_per_second=5.10171M/s

 Performance counter stats for './build-profile/benchmarks/lockfree_queue_benchmark --benchmark_filter=BM_MutexQueue':

     5,905,865,774      cycles                                                                  (67.15%)
     4,305,971,319      instructions                                                            (83.27%)
       186,081,237      cache-references                                                        (83.27%)
         5,802,115      cache-misses                                                            (83.23%)
     1,051,726,197      branches                                                                (83.34%)
         6,984,550      branch-misses                                                           (83.17%)

       0.969956577 seconds time elapsed

       0.915530000 seconds user
       0.914525000 seconds sys

///////////////////////////////////////////////////


$ perf stat \
    -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    ./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_filter=BM_LockFreeQueue
2026-08-03T09:53:20-04:00
Running ./build-profile/benchmarks/lockfree_queue_benchmark
Run on (8 X 4295.31 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.41, 1.06, 0.65
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------
BM_LockFreeQueue/real_time       36.6 ms        0.077 ms           22 items_per_second=27.2975M/s

 Performance counter stats for './build-profile/benchmarks/lockfree_queue_benchmark --benchmark_filter=BM_LockFreeQueue':

     9,127,173,114      cycles                                                                  (68.00%)
     1,494,023,959      instructions                                                            (84.26%)
       709,372,017      cache-references                                                        (83.89%)
           613,074      cache-misses                                                            (82.79%)
       151,301,888      branches                                                                (82.82%)
        11,683,132      branch-misses                                                           (83.46%)

       1.158152943 seconds time elapsed

       2.295737000 seconds user
       0.012992000 seconds sys


 //////////////////////////////////////////////////////


 # Lock-Free SPSC Queue

## Overview

The lock-free queue provides low-latency communication between exactly one
producer thread and one consumer thread.

It is implemented as a fixed-capacity ring buffer and is intended for
high-frequency message transfer between independent stages of the OpenHFT
pipeline.

Typical usage:

```text
Parser Thread
     |
     v
SPSC Ring Buffer
     |
     v
Matching Engine Thread
```

The queue avoids mutexes, condition variables, and dynamic allocation in the
hot path.

---

## Motivation

The original processing path executed parsing and order-book updates
synchronously.

```text
Receiver
   |
   v
Parser
   |
   v
ITCH Handler
   |
   v
Order Book
```

This design is simple, but a slow downstream stage can delay upstream packet
processing.

The SPSC queue introduces an asynchronous boundary between stages:

```text
Producer Thread
     |
     v
SPSC Queue
     |
     v
Consumer Thread
```

This allows each stage to progress independently until the queue reaches its
capacity.

---

## Why SPSC

SPSC means:

- Single Producer
- Single Consumer

This model matches the intended OpenHFT pipeline:

| Queue | Producer | Consumer |
|---|---|---|
| Parser to Matching | Parser thread | Matching thread |
| Matching to Logger | Matching thread | Logger thread |
| Matching to WAL | Matching thread | WAL writer thread |
| Matching to Monitoring | Matching thread | Monitoring thread |

An SPSC design is preferred over MPSC or MPMC because it requires fewer atomic
operations and has a simpler correctness model.

---

## Design Goals

The queue is designed with the following goals:

- Fixed capacity
- No dynamic allocation in the hot path
- FIFO ordering
- Wait-free `tryPush()` for the producer
- Wait-free `tryPop()` for the consumer
- Acquire/release synchronization
- Cache-line separation between producer and consumer state
- Explicit backpressure
- Lightweight runtime metrics
- Predictable latency

---

## Public API

```cpp
template<typename T, std::size_t Capacity>
class SPSCRingBuffer
{
public:
    bool tryPush(const T& value);
    bool tryPop(T& value);

    bool empty() const;
    bool full() const;

    std::size_t size() const;
    constexpr std::size_t capacity() const;

    const SPSCQueueMetrics& metrics() const noexcept;
};
```

`tryPush()` and `tryPop()` are non-blocking.

A failed call returns `false` and leaves the queue unchanged.

---

## Capacity Model

The implementation intentionally leaves one array slot unused.

This makes it possible to distinguish full and empty states using only the
producer and consumer indexes.

For example:

```text
Declared capacity: 4096
Usable capacity:   4095
```

The states are:

```text
Empty:
head == tail

Full:
increment(head) == tail
```

---

## Ring Buffer Layout

```text
+--------+--------+--------+--------+--------+
| Slot 0 | Slot 1 | Slot 2 |  ...   | Slot N |
+--------+--------+--------+--------+--------+
     ^                                  ^
     |                                  |
   tail                               head
```

The producer writes only to `head_`.

The consumer writes only to `tail_`.

Indexes wrap back to zero after reaching the end of the underlying array.

---

## Memory Ordering

The queue uses acquire/release ordering instead of sequential consistency.

### Producer

The producer:

1. Reads its own `head_` using relaxed ordering.
2. Reads `tail_` using acquire ordering.
3. Writes the object into the buffer.
4. Publishes the new `head_` using release ordering.

```cpp
buffer_[head] = value;

head_.store(
    nextHead,
    std::memory_order_release);
```

The release store guarantees that the buffer write becomes visible before the
new head position is observed by the consumer.

### Consumer

The consumer:

1. Reads its own `tail_` using relaxed ordering.
2. Reads `head_` using acquire ordering.
3. Reads the object from the buffer.
4. Publishes the new `tail_` using release ordering.

```cpp
value = buffer_[tail];

tail_.store(
    increment(tail),
    std::memory_order_release);
```

The acquire load guarantees that the consumer sees the producer's completed
buffer write after observing the updated head.

### Why Not `memory_order_seq_cst`

Sequential consistency provides a stronger global ordering than this algorithm
requires.

For an SPSC queue, acquire/release ordering is sufficient to establish the
required happens-before relationships with less synchronization overhead.

---

## Cache-Line Layout

The producer and consumer indexes are aligned separately:

```cpp
alignas(64)
std::atomic<std::size_t> head_{0};

alignas(64)
std::atomic<std::size_t> tail_{0};
```

This reduces false sharing.

Without separation, both cores may repeatedly invalidate the same cache line
even though the producer and consumer update different variables.

The exact hardware cache-line size is platform dependent, but 64 bytes is the
target size for the current x86-64 environment.

---

## Backpressure

The queue has fixed capacity, so the producer eventually receives `false` from
`tryPush()` if the consumer cannot keep up.

The queue itself does not select a waiting or overflow policy.

The caller may choose:

- Busy spin with `_mm_pause()`
- Yield
- Sleep
- Drop
- Retry with a bounded limit
- Fail-safe shutdown

Example busy-spin policy:

```cpp
while (!queue.tryPush(order))
{
    _mm_pause();
}
```

This separation keeps the queue primitive reusable and allows different
components to select different backpressure policies.

For example:

- Market data may permit controlled dropping in some environments.
- Order and execution events must normally never be dropped.
- WAL events must stall or trigger a failure policy if persistence cannot keep
  up.

---

## Runtime Metrics

The queue exposes lightweight counters:

```cpp
struct SPSCQueueMetrics
{
    std::atomic<std::uint64_t> pushCount{0};
    std::atomic<std::uint64_t> popCount{0};
    std::atomic<std::uint64_t> pushFailures{0};
    std::atomic<std::uint64_t> popFailures{0};
    std::atomic<std::size_t> highWaterMark{0};
};
```

These metrics provide:

- Number of successful pushes
- Number of successful pops
- Number of full-queue observations
- Number of empty-queue observations
- Maximum observed queue depth

Metric atomics use relaxed ordering because they do not participate in queue
correctness.

Only the producer updates `highWaterMark`, which is valid for this SPSC design.

---

## Monitoring Integration

A monitoring thread can periodically read queue metrics and export a snapshot.

Planned Prometheus metrics include:

```text
openhft_queue_depth
openhft_queue_capacity
openhft_queue_high_water_mark
openhft_queue_push_total
openhft_queue_pop_total
openhft_queue_push_failures_total
openhft_queue_pop_failures_total
openhft_queue_drops_total
openhft_queue_producer_stall_seconds_total
```

Monitoring must not perform formatting, logging, network I/O, or blocking work
inside `tryPush()` or `tryPop()`.

---

## Testing

The queue tests cover:

- Empty state after construction
- Push and pop
- FIFO ordering
- Full detection
- Empty detection
- Usable capacity
- Index wrap-around
- Push metrics
- Pop metrics
- Push failures
- Pop failures
- High-water mark
- Mixed operations

The full project test suite currently passes after the queue changes.

---

## Benchmark Methodology

The queue benchmark transfers one million real `Order` objects between:

- One producer thread
- One consumer thread

It compares:

```text
std::queue<Order>
+ std::mutex
+ std::condition_variable
```

against:

```text
SPSCRingBuffer<Order, 4096>
+ acquire/release atomics
+ busy-spin retry
```

Google Benchmark uses real time because the workload runs in worker threads.

Hardware counters are collected with Linux `perf`.

---

## Initial Benchmark Results

| Implementation | Time | Throughput |
|---|---:|---:|
| Mutex Queue | 181 ms | 5.53 M orders/s |
| SPSC Queue | 32.0 ms | 31.23 M orders/s |

The initial SPSC implementation achieved approximately:

- 5.65x higher throughput
- 82% lower elapsed time

A second perf run produced:

| Metric per one million orders | Mutex Queue | SPSC Queue |
|---|---:|---:|
| Time | 196 ms | 36.6 ms |
| Cycles | 1.476 B | 415 M |
| Instructions | 1.076 B | 67.9 M |
| Cache misses | 1.45 M | 27.9 K |
| Branch misses | 1.75 M | 531 K |

These results are an initial baseline, not the final fair comparison.

---

## Benchmark Limitations

The first benchmark compares queues with different waiting and capacity
policies:

- The mutex queue is unbounded.
- The SPSC queue is bounded.
- The mutex queue blocks using a condition variable.
- The SPSC queue busy-spins.
- Producer and consumer threads are not yet pinned.
- Hardware-counter groups were multiplexed.
- Google Benchmark was linked against a Debug build of the benchmark library.

A bounded mutex queue and controlled CPU-affinity tests are planned.

---

## Integration Plan

The first production integration point is:

```text
DPDK Receiver / Parser Thread
            |
            v
SPSC Queue<Order>
            |
            v
Matching Engine Thread
```

The matching engine remains single-writer and does not become internally
concurrent.

Future uses include:

```text
Matching Engine -> Logger
Matching Engine -> WAL
Matching Engine -> Monitoring
Strategy Engine -> Execution Gateway
```

Each connection should use a separate queue with exactly one producer and one
consumer.

---

## Failure Handling

The queue does not throw on normal full or empty conditions.

A failed push or pop returns `false`.

Higher-level components are responsible for deciding whether to:

- Retry
- Drop
- Stall
- Alert
- Stop the pipeline

This keeps failure policy out of the low-level data structure.

---

## Planned Improvements

- Metrics snapshot API
- Bounded mutex baseline
- Producer and consumer CPU affinity
- Backpressure benchmark
- Queue occupancy sampling
- p50/p95/p99 enqueue-to-dequeue latency
- Batch push and batch pop
- Configurable wait strategies
- Monitoring exporter
- Integration with the market-data pipeline
- Logging queue
- WAL queue
- Performance regression checks in CI

---

## Lessons Learned

- SPSC is simpler and faster than a more general multi-producer queue when the
  topology genuinely has one producer and one consumer.
- Fixed capacity makes memory usage predictable but requires explicit
  backpressure.
- Acquire/release ordering is sufficient for publishing and consuming slots.
- Cache-line separation matters for frequently modified producer and consumer
  state.
- Busy spinning improves latency but consumes CPU.
- Metrics must be lightweight and must not introduce synchronization into the
  queue algorithm.
- A fast benchmark is not enough; queue capacity, waiting policy, thread
  placement, and monitoring overhead must also be evaluated.

---

## Conclusion

The lock-free SPSC ring buffer provides a low-overhead communication primitive
for the threaded OpenHFT architecture.

It preserves the single-writer matching-engine model while decoupling producer
and consumer stages.

The queue is currently tested, benchmarked, instrumented with lightweight
metrics, and ready for controlled integration into the market-data pipeline.


./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true
2026-08-03T15:34:04-04:00
Running ./build-profile/benchmarks/lockfree_queue_benchmark
Run on (8 X 4664.54 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.65, 0.65, 0.56
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------
BM_MutexQueue/real_time_mean             186 ms        0.164 ms            5 items_per_second=5.39731M/s
BM_MutexQueue/real_time_median           186 ms        0.192 ms            5 items_per_second=5.38458M/s
BM_MutexQueue/real_time_stddev          8.97 ms        0.103 ms            5 items_per_second=265.171k/s
BM_MutexQueue/real_time_cv              4.83 %         63.03 %             5 items_per_second=4.91%
BM_LockFreeQueue/real_time_mean         44.0 ms        0.070 ms            5 items_per_second=23.3719M/s
BM_LockFreeQueue/real_time_median       44.6 ms        0.064 ms            5 items_per_second=22.4393M/s
BM_LockFreeQueue/real_time_stddev       7.85 ms        0.024 ms            5 items_per_second=4.74675M/s
BM_LockFreeQueue/real_time_cv          17.81 %         34.06 %             5 items_per_second=20.31%



## Repeated Benchmark Results

The benchmark was repeated five times to evaluate both average performance
and run-to-run variability.

| Implementation | Mean Time | Median Time | Mean Throughput | CV |
|---|---:|---:|---:|---:|
| Mutex Queue | 186 ms | 186 ms | 5.40 M orders/s | 4.83% |
| SPSC Queue | 44.0 ms | 44.6 ms | 23.37 M orders/s | 17.81% |

The lock-free SPSC queue achieved approximately 4.23x higher mean throughput
and reduced elapsed time by about 76.3%.

However, the SPSC benchmark showed higher run-to-run variability. The likely
causes include operating-system scheduling, CPU migration, shared-core
placement, dynamic CPU frequency, and the sensitivity of busy-spin loops to
thread placement.

The current results confirm a substantial throughput advantage, but they also
show that deterministic low-latency execution requires explicit producer and
consumer CPU affinity.

A future benchmark will pin the producer and consumer to selected physical
cores and compare multiple CPU pairings.


/////////////////////////////////////////////////////////////////////
## Fair Benchmark Comparison

./build-profile/benchmarks/lockfree_queue_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/shajabedi/Projects/openhft-fpga-dpdk/build-profile
[  2%] Built target ethernet_parser
[  5%] Built target orderbook
[  7%] Built target udp_parser
[ 10%] Built target replay
[ 12%] Built target fixed_hash_map_benchmark
[ 15%] Built target platform_utils
[ 26%] Built target itch_parser
[ 28%] Built target ipv4_parser
[ 30%] Built target huge_page_benchmark
[ 31%] Building CXX object benchmarks/CMakeFiles/lockfree_queue_benchmark.dir/lockfree_queue_benchmark.cpp.o
[ 32%] Built target allocator_benchmark
[ 34%] Built target false_sharing_benchmark
[ 40%] Built target matching_engine_core
[ 42%] Built target ethernet_test
[ 44%] Built target risk
[ 46%] Built target orderbook_benchmark
[ 48%] Built target order_pool_benchmark
[ 50%] Built target numa_benchmark
[ 52%] Built target bitmap_benchmark
[ 54%] Built target matching_engine_benchmark
[ 56%] Built target matching_engine_profile
[ 59%] Built target pcap_replay_reader_test
[ 63%] Built target ipv4_test
[ 64%] Built target gateway
[ 66%] Built target matching_engine_demo
[ 68%] Built target pipeline_benchmark
[ 70%] Building CXX object tests/CMakeFiles/parser_tests.dir/unit/common/spsc_queue_metrics_test.cpp.o
[ 70%] Building CXX object tests/CMakeFiles/parser_tests.dir/unit/common/spsc_ring_buffer_test.cpp.o
[ 71%] Linking CXX executable lockfree_queue_benchmark
[ 71%] Built target lockfree_queue_benchmark
[ 72%] Linking CXX executable parser_tests
[100%] Built target parser_tests
2026-08-03T16:15:16-04:00
Running ./build-profile/benchmarks/lockfree_queue_benchmark
Run on (8 X 2800.17 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 1.02, 0.71, 0.57
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------------------------------------------
Benchmark                                      Time             CPU   Iterations UserCounters...
------------------------------------------------------------------------------------------------
BM_MutexQueue/real_time_mean                 261 ms        0.111 ms            5 items_per_second=3.82977M/s
BM_MutexQueue/real_time_median               261 ms        0.099 ms            5 items_per_second=3.83073M/s
BM_MutexQueue/real_time_stddev              4.06 ms        0.042 ms            5 items_per_second=59.8273k/s
BM_MutexQueue/real_time_cv                  1.56 %         37.54 %             5 items_per_second=1.56%
BM_BoundedMutexQueue/real_time_mean          226 ms        0.073 ms            5 items_per_second=4.42324M/s
BM_BoundedMutexQueue/real_time_median        229 ms        0.074 ms            5 items_per_second=4.37212M/s
BM_BoundedMutexQueue/real_time_stddev       6.32 ms        0.004 ms            5 items_per_second=124.134k/s
BM_BoundedMutexQueue/real_time_cv           2.79 %          5.86 %             5 items_per_second=2.81%
BM_LockFreeQueue/real_time_mean             99.7 ms        0.061 ms            5 items_per_second=10.0506M/s
BM_LockFreeQueue/real_time_median           99.6 ms        0.061 ms            5 items_per_second=10.0403M/s
BM_LockFreeQueue/real_time_stddev           5.05 ms        0.005 ms            5 items_per_second=493.712k/s
BM_LockFreeQueue/real_time_cv               5.07 %          8.60 %             5 items_per_second=4.91%
shajabedi@shajabedi-ThinkPad-T14-Gen-1:~/Projects/openhft-fpga-dpdk$ 



## Fair Bounded-Queue Comparison

A bounded mutex-protected ring buffer was added to provide a fairer
comparison with the lock-free SPSC queue.

Both bounded implementations use:

- Fixed-capacity storage
- A usable capacity of 4095 orders
- No dynamic allocation during message transfer
- One producer and one consumer
- Identical `Order` payloads

| Implementation | Mean Time | Mean Throughput | CV |
|---|---:|---:|---:|
| Unbounded Mutex Queue | 261 ms | 3.83 M orders/s | 1.56% |
| Bounded Mutex Queue | 226 ms | 4.42 M orders/s | 2.79% |
| Lock-Free SPSC Queue | 99.7 ms | 10.05 M orders/s | 5.07% |

The bounded mutex queue improved throughput compared with the unbounded
`std::queue` implementation. However, the lock-free SPSC queue remained
approximately 2.27 times faster than the bounded mutex queue and reduced
elapsed time by about 55.9%.

This fairer comparison indicates that the performance advantage is not
caused only by fixed-capacity storage or the absence of dynamic allocation.
A substantial benefit remains from replacing mutex and condition-variable
coordination with acquire/release atomics and user-space busy spinning.

The lock-free implementation showed slightly higher run-to-run variability.
Future CPU-affinity experiments will evaluate whether pinning the producer
and consumer to selected physical cores reduces this jitter.

///////////////////////////////////////








