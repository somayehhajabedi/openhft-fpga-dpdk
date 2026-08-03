# P17 — Lock-Free Queue Performance Evaluation

## Objective

Evaluate the performance difference between a traditional
mutex-protected producer/consumer queue and a lock-free
Single Producer Single Consumer (SPSC) ring buffer.

Both implementations transfer identical `Order` objects
between one producer thread and one consumer thread.

---

# Test Environment

CPU

Intel Core i7-10510U

Cores

4 Physical / 8 Logical

Compiler

GCC

Benchmark Framework

Google Benchmark

Operating System

Ubuntu Linux

---

# Queue Implementations

## Mutex Queue

Synchronization

- std::mutex
- std::condition_variable
- std::queue<Order>

Characteristics

- Blocking synchronization
- Kernel-assisted wake-up
- Unbounded queue

---

## Lock-Free Queue

Synchronization

- Acquire/Release atomics
- Fixed-capacity SPSC ring buffer
- Busy-spin using `_mm_pause()`

Characteristics

- Wait-free producer
- Wait-free consumer
- Allocation-free hot path
- Cache-line aligned

---

# Google Benchmark Results

| Benchmark | Time | Throughput |
|-----------|------:|-----------:|
| Mutex Queue | 181 ms | 5.53 M Orders/sec |
| Lock-Free Queue | **32.0 ms** | **31.23 M Orders/sec** |

---

# Relative Performance

Lock-Free Queue achieved

- **5.65× higher throughput**
- **82% lower execution time**

compared to the mutex-based implementation.

---

# Hardware Performance Counters

## Mutex Queue

| Counter | Value |
|----------|-------------:|
| Cycles | 5,905,865,774 |
| Instructions | 4,305,971,319 |
| Cache References | 186,081,237 |
| Cache Misses | 5,802,115 |
| Branches | 1,051,726,197 |
| Branch Misses | 6,984,550 |

Iterations

4

Approximate per one million transferred orders

| Metric | Value |
|---------|---------:|
| Cycles | 1.476 B |
| Instructions | 1.076 B |
| Cache References | 46.5 M |
| Cache Misses | 1.45 M |
| Branches | 262.9 M |
| Branch Misses | 1.75 M |

---

## Lock-Free Queue

| Counter | Value |
|----------|-------------:|
| Cycles | 9,127,173,114 |
| Instructions | 1,494,023,959 |
| Cache References | 709,372,017 |
| Cache Misses | 613,074 |
| Branches | 151,301,888 |
| Branch Misses | 11,683,132 |

Iterations

22

Approximate per one million transferred orders

| Metric | Value |
|---------|---------:|
| Cycles | 415 M |
| Instructions | 67.9 M |
| Cache References | 32.2 M |
| Cache Misses | 27.9 K |
| Branches | 6.88 M |
| Branch Misses | 531 K |

---

# Analysis

Compared with the mutex-based implementation, the lock-free
queue demonstrated:

- ~72% fewer CPU cycles
- ~94% fewer executed instructions
- ~98% fewer cache misses
- ~97% fewer branch instructions

The reduced synchronization overhead allows the producer
and consumer to exchange messages entirely in user space
without relying on kernel-assisted blocking.

The mutex implementation spends a significant portion of
its execution time inside the operating system due to
mutex locking and condition variable wake-ups.

The lock-free implementation instead performs short
busy-spin retries using `_mm_pause()`, trading increased
CPU utilization for significantly lower latency.

---

# Current Limitations

The current comparison is intended as an initial baseline.

Differences between the two implementations include:

- Mutex queue is currently unbounded.
- Lock-free queue has a fixed capacity of 4095 elements.
- Lock-free queue uses busy spinning.
- Mutex queue blocks using condition variables.

Because the synchronization policies differ, the benchmark
should not be interpreted as a perfectly equivalent
comparison.

---

# Future Improvements

The following improvements are planned:

- Bounded mutex queue
- Backpressure comparison
- Queue occupancy metrics
- Producer stall measurements
- Consumer stall measurements
- Queue depth histogram
- Latency distribution (P50/P95/P99)
- CPU affinity comparison
- NUMA-aware execution
- Monitoring integration
- Prometheus metrics
- Grafana dashboards

---

# Conclusion

The initial benchmark demonstrates the significant benefit
of a lock-free SPSC queue for low-latency producer/consumer
communication.

Even in this preliminary evaluation, the lock-free
implementation provides more than five times the throughput
of the mutex-based design while dramatically reducing
CPU work and cache miss rates.

These results justify integrating the SPSC queue into the
future OpenHFT processing pipeline between the market data
receiver and the matching engine.