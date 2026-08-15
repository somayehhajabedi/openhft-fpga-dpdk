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



/////////////////////////////////


## IRQ Affinity Experiment

### Objective

After CPU affinity and CPU isolation, rare latency spikes in the
tens-of-microseconds range were still observed.

The next experiment investigated whether hardware interrupts,
particularly Wi-Fi interrupts, were responsible for these extreme
tail-latency outliers.

### Initial IRQ Distribution

Inspection of `/proc/interrupts` showed that Wi-Fi interrupts were
still executing on the latency-sensitive CPUs.

Relevant examples included:

```text
IRQ 171 -> iwlwifi:default_queue
IRQ 174 -> iwlwifi:queue_3
IRQ 175 -> iwlwifi:queue_4
```

CPU 2 and CPU 3 are used by the latency-sensitive pipeline threads.

Despite booting with:

```text
isolcpus=2,3,6,7
```

Wi-Fi interrupts were still observed on CPU 2 and CPU 3.

This demonstrates an important distinction:

> CPU scheduler isolation does not automatically imply IRQ isolation.

### IRQ Affinity Configuration

The system's `irqbalance` service was verified to be inactive before
manually changing IRQ affinity.

The relevant Wi-Fi IRQs were moved to the housekeeping CPUs:

```text
0,1,4,5
```

using:

```bash
echo 0,1,4,5 | sudo tee /proc/irq/171/smp_affinity_list
echo 0,1,4,5 | sudo tee /proc/irq/174/smp_affinity_list
echo 0,1,4,5 | sudo tee /proc/irq/175/smp_affinity_list
```

The resulting affinity configuration was verified as:

```text
IRQ 171: 0-1,4-5
IRQ 174: 0-1,4-5
IRQ 175: 0-1,4-5
```

This removed these Wi-Fi IRQs from the latency-sensitive CPU set:

```text
2,3,6,7
```

---

### Benchmark Results

The uncontended latency benchmark was executed five times after
applying IRQ affinity.

| Run | p50 | p95 | p99 | p99.9 | Max |
|---|---:|---:|---:|---:|---:|
| 1 | 160 ns | 175 ns | 183 ns | 207 ns | 42.958 us |
| 2 | 151 ns | 166 ns | 172 ns | 191 ns | 46.644 us |
| 3 | 152 ns | 172 ns | 180 ns | 202 ns | 16.997 us |
| 4 | 161 ns | 174 ns | 182 ns | 207 ns | 38.859 us |
| 5 | 169 ns | 184 ns | 193 ns | 249 ns | 56.225 us |

The normal latency distribution remained approximately:

```text
p50    = 151-169 ns
p99    = 172-193 ns
p99.9  = 191-249 ns
```

However, extreme maximum latency remained highly variable:

```text
max = 17.0-56.2 us
```

---

## Comparison With CPU Isolation Baseline

Before manually moving the Wi-Fi IRQs, five runs after CPU isolation
produced:

| Metric | CPU Isolation | CPU Isolation + IRQ Affinity |
|---|---:|---:|
| p50 | 148-163 ns | 151-169 ns |
| p99 | 173-194 ns | 172-193 ns |
| p99.9 | 194-214 ns | 191-249 ns |
| Max | 37.5-43.4 us | 17.0-56.2 us |

The central latency distribution remained almost unchanged.

IRQ affinity therefore did not produce a consistent reduction in
extreme tail latency.

One run reached a maximum latency of only approximately 17 us, while
another reached approximately 56 us.

The variation indicates that Wi-Fi IRQ placement alone does not
explain the recurring latency spikes.

---

## Findings So Far

The investigation has progressively tested several possible sources
of latency variation:

```text
Market Data Pipeline
        |
        v
CPU affinity
        |
        v
Scheduler analysis with perf sched
        |
        v
CPU topology analysis
        |
        v
Wi-Fi disable experiment
        |
        v
CPU isolation
        |
        v
Wi-Fi IRQ affinity
        |
        v
Extreme tail-latency spikes still remain
```

The uncontended pipeline consistently achieves approximately
150-200 ns typical latency.

The p99 and p99.9 latency are also generally sub-microsecond.

However, rare outliers in the tens-of-microseconds range remain
reproducible.

The experiments so far indicate that the extreme outliers cannot be
explained solely by:

- normal Linux scheduler placement
- Wi-Fi activity
- CPU affinity
- scheduler CPU isolation
- Wi-Fi IRQ placement

---

## Important Observation

The benchmark currently measures each event using
`std::chrono::steady_clock::now()`.

Therefore, before applying additional system-level tuning, the
measurement methodology itself should also be investigated.

Potential remaining sources include:

- other hardware interrupts
- kernel threads
- softirq activity
- System Management Interrupts (SMIs)
- SMT sibling interference
- CPU frequency and power-state transitions
- timer and kernel housekeeping activity
- page faults
- clock-reading overhead
- benchmark instrumentation
- preemption occurring between the start and end timestamps

At this stage, applying additional optimizations without identifying
the source of the outlier would risk optimizing based on speculation.

---

## Next Step: Correlate the Spike With System Activity

The next phase should focus on identifying what happens during the
rare 20-60 us latency events.

Instead of modifying additional system parameters immediately, the
goal will be to correlate latency spikes with observable system
activity.

The investigation should include:

1. tracing scheduler activity around latency spikes
2. inspecting remaining IRQ activity on the isolated CPUs
3. investigating softirq and kernel-thread activity
4. validating the latency measurement methodology
5. evaluating the cost and behavior of `steady_clock::now()`
6. investigating SMT and CPU power-management effects if necessary

The objective is to move from:

```text
"We observe a ~40 us latency spike."
```

to:

```text
"A ~40 us latency spike occurred because of X."
```

Only after identifying that cause should further low-latency tuning
be applied.


////////////////////////////////////////






