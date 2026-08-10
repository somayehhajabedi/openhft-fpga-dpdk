# Market Data Pipeline Tail-Latency Investigation

## Objective

Investigate rare tail-latency spikes in the market-data pipeline.

The uncontended benchmark normally shows sub-microsecond latency,
but occasional latency spikes of tens of microseconds are observed.

## Baseline

Typical uncontended latency observed across multiple runs:

| Metric | Typical Range |
|---|---:|
| p50 | 150–210 ns |
| p95 | 170–230 ns |
| p99 | 180–250 ns |
| p99.9 | 200–300 ns |
| max | 34–45 µs |

The large difference between p99.9 and maximum latency indicates
rare system-level outliers.

## CPU Affinity

The producer and consumer threads were pinned to separate logical CPUs:

- Producer: CPU 2
- Consumer: CPU 3

CPU topology:

```text
CPU 0 / CPU 4 -> Physical Core 0
CPU 1 / CPU 5 -> Physical Core 1
CPU 2 / CPU 6 -> Physical Core 2
CPU 3 / CPU 7 -> Physical Core 3

However, their SMT siblings (CPU 6 and CPU 7) remain available to
other system workloads.

Scheduler Analysis

perf sched was used to inspect scheduler activity:

sudo perf sched record \
    ./build-release/benchmarks/market_data_pipeline_latency_benchmark

sudo perf sched timehist --cpu 2,3





Unrelated activity was observed on the latency-sensitive CPUs,
including:

WebRTC worker threads
terminal processes
kernel workers
other scheduler activity

Some observed execution intervals were in the tens-of-microseconds
range, comparable to the benchmark's maximum latency spikes.

Interrupt Analysis

/proc/interrupts showed hardware interrupts assigned to CPUs 2 and 3.

These included:

NVMe interrupts
Wi-Fi interrupts

Therefore, CPU affinity alone does not provide an isolated execution
environment.

Wi-Fi Experiment

Wi-Fi was temporarily disabled to determine whether Wi-Fi interrupts
were the primary source of the tail-latency spikes.

Across repeated runs with Wi-Fi disabled, uncontended latency remained
approximately:

p50    ~150-210 ns
p99    ~180-250 ns
p99.9  ~200-300 ns
max    ~34-43 us

Rare tens-of-microseconds spikes therefore remained reproducible.

Finding

Wi-Fi activity may contribute to system noise, but disabling Wi-Fi
alone does not eliminate the tail-latency spikes.

The current evidence points toward broader OS interference including:

scheduler activity
kernel workers
hardware interrupts
SMT sibling contention

rather than the SPSC queue itself.

Key Lesson

CPU affinity is not CPU isolation.

Affinity controls where a thread may execute, but does not prevent
other tasks or interrupts from executing on the same physical CPU
resources.

For deterministic low-latency workloads, CPU isolation, IRQ affinity,
and SMT behavior must also be considered.

Next Experiment

Introduce controlled CPU isolation for the producer and consumer
physical cores.

Then repeat the same benchmark and compare:

p50
p95
p99
p99.9
max latency
context switches
CPU migrations

The main question is whether CPU isolation reduces the recurring
~34-45 us maximum latency spikes.



market_data_pipeline_latency.md
    → Benchmark design and latency measurements

market_data_pipeline_tail_latency.md
    → Tail-latency spike investigation
    → perf sched analysis
    → CPU affinity
    → IRQ analysis
    → Wi-Fi experiment
    → CPU isolation

///////////////////////////



## CPU Isolation Experiment

Linux was configured to isolate the logical CPUs used by the
latency-sensitive pipeline threads.

The kernel was booted with:

```text
isolcpus=2,3,6,7

The CPU topology was:

Physical Core 0: CPU 0, CPU 4
Physical Core 1: CPU 1, CPU 5
Physical Core 2: CPU 2, CPU 6
Physical Core 3: CPU 3, CPU 7

The benchmark configuration used:

Producer: CPU 2
Consumer: CPU 3
CPU 6 and CPU 7 are the SMT siblings of the selected cores.
Results

Five uncontended latency runs after CPU isolation produced:

Run	p50	p95	p99	p99.9	Max
1	159 ns	172 ns	179 ns	198 ns	39.6 us
2	163 ns	177 ns	194 ns	214 ns	43.4 us
3	148 ns	166 ns	173 ns	198 ns	37.5 us
4	152 ns	173 ns	181 ns	194 ns	38.0 us
5	153 ns	171 ns	187 ns	201 ns	37.6 us

The normal latency distribution was stable:

p50    = 148-163 ns
p99    = 173-194 ns
p99.9  = 194-214 ns

However, rare maximum-latency outliers remained:

max = 37.5-43.4 us
Finding

CPU isolation improved the stability of the normal latency
distribution, but it did not eliminate the rare tens-of-microseconds
latency spikes.

This demonstrates that CPU affinity and scheduler isolation alone are
not sufficient to guarantee deterministic tail latency.

The remaining outliers may involve other sources of system noise,
including:

hardware interrupts
kernel activity
SMT sibling effects
CPU power and frequency management
benchmark instrumentation overhead
Next Step

The next investigation will focus on IRQ activity and IRQ affinity on
the latency-sensitive CPUs.

The objective is to determine whether hardware interrupts correlate
with the remaining approximately 40 us maximum-latency outliers.

progression :

```text
~200 ns normal latency
        ↓
~40 us unexplained outlier
        ↓
perf sched
        ↓
CPU affinity
        ↓
IRQ / Wi-Fi investigation
        ↓
CPU isolation
        ↓
~40 us still remains
        ↓
IRQ affinity / deeper tracing  ← next



