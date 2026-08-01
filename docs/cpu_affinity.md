CPU Affinity Optimization
Overview

This document evaluates the impact of CPU affinity on the matching engine profiling workload.

CPU affinity pins the execution thread to a dedicated logical CPU in order to reduce scheduler-induced migrations, improve cache locality, and reduce latency jitter

Motivation


Commands :

taskset -c 2 \
./build-profile/benchmarks/matching_engine_profile
Profiling workload is ready.
PID: 249882
Pairs per batch: 3000
Measured batches: 10000
Buy price levels: 16
Sell price: 100
Press Enter after attaching perf.

Orders processed: 60000000
Elapsed: 2.82445 seconds
Throughput: 2.12431e+07 orders/sec
Average latency: 47.0742 ns/order


./build-profile/benchmarks/matching_engine_profile
Profiling workload is ready.
PID: 249899
Pairs per batch: 3000
Measured batches: 10000
Buy price levels: 16
Sell price: 100
Press Enter after attaching perf.


Orders processed: 60000000
Elapsed: 3.69616 seconds
Throughput: 1.62331e+07 orders/sec
Average latency: 61.6026 ns/order

/////////////////////////////////////////////////

Without CPU affinity:

the Linux scheduler may migrate the thread
CPU caches may be invalidated
latency becomes less deterministic

Pinning the thread to a dedicated CPU minimizes these effects.

Benchmark Setup

Platform:

Ubuntu Linux
Intel CPU
RelWithDebInfo build

Workload:

Orders processed: 60,000,000
Buy price levels: 16
Sell price: 100
Measured batches: 10,000



Results


CPU Affinity Enabled:

Orders processed: 60000000

Elapsed:
2.82445 s

Throughput:
21.24 M orders/sec

Latency:
47.07 ns/order

/////////////////////////////////////////

Default Linux Scheduling
Orders processed: 60000000

Elapsed:
3.69616 s

Throughput:
16.23 M orders/sec

Latency:
61.60 ns/order

////////////////////////////////////////


Comparison
Metric	CPU Affinity	Default Scheduler
Throughput	21.24 M orders/s	16.23 M orders/s
Latency	47.07 ns	61.60 ns

Observed improvement (single run):

Throughput: ≈31% higher
Average latency: ≈24% lower


Notes

These measurements are based on a single run.

Final conclusions should be based on multiple repetitions to account for scheduler noise, CPU frequency scaling, and background activity.

Future Work
Repeat experiments across multiple CPU cores.
Measure latency variance and jitter.
Compare SMT sibling cores versus dedicated physical cores.
Evaluate interaction with NUMA-aware memory allocation.

///////////////////////////////////////////////

$ lscpu -e=CPU,CORE,SOCKET,NODE,ONLINE,MAXMHZ,MINMHZ

$ mpstat -P ALL 1 5


$lscpu -e=CPU,CORE,SOCKET,NODE,ONLINE,MAXMHZ,MINMHZ

CPU CORE SOCKET NODE ONLINE    MAXMHZ   MINMHZ
  0    0      0    0    yes 4900.0000 400.0000
  1    1      0    0    yes 4900.0000 400.0000
  2    2      0    0    yes 4900.0000 400.0000
  3    3      0    0    yes 4900.0000 400.0000
  4    0      0    0    yes 4900.0000 400.0000
  5    1      0    0    yes 4900.0000 400.0000
  6    2      0    0    yes 4900.0000 400.0000
  7    3      0    0    yes 4900.0000 400.0000


$ mpstat -P ALL 1 5

Linux 7.0.0-27-generic (shajabedi-ThinkPad-T14-Gen-1) 	07/31/2026 	_x86_64_	(8 CPU)

09:44:43 PM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
09:44:44 PM  all    1.51    0.00    0.25    0.00    0.00    0.00    0.00    0.00    0.00   98.24
09:44:44 PM    0    1.00    0.00    1.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:44 PM    1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:44 PM    2    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:44 PM    3    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:44 PM    4    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:44 PM    5    2.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:44 PM    6    9.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   91.00
09:44:44 PM    7    0.00    0.00    1.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00

09:44:44 PM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
09:44:45 PM  all    1.12    0.00    0.12    0.00    0.00    0.00    0.00    0.00    0.00   98.75
09:44:45 PM    0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:45 PM    1    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:45 PM    2    2.97    0.00    0.99    0.00    0.00    0.00    0.00    0.00    0.00   96.04
09:44:45 PM    3    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:45 PM    4    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:45 PM    5    0.99    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.01
09:44:45 PM    6    3.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   97.00
09:44:45 PM    7    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00

09:44:45 PM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
09:44:46 PM  all    0.62    0.00    0.38    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:46 PM    0    0.00    0.00    1.01    0.00    0.00    0.00    0.00    0.00    0.00   98.99
09:44:46 PM    1    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:46 PM    2    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:46 PM    3    1.98    0.00    0.99    0.00    0.00    0.00    0.00    0.00    0.00   97.03
09:44:46 PM    4    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:46 PM    5    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:46 PM    6    1.98    0.00    0.99    0.00    0.00    0.00    0.00    0.00    0.00   97.03
09:44:46 PM    7    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00

09:44:46 PM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
09:44:47 PM  all    0.63    0.00    0.00    0.13    0.00    0.00    0.00    0.00    0.00   99.25
09:44:47 PM    0    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:47 PM    1    0.00    0.00    0.00    1.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:47 PM    2    2.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:47 PM    3    1.01    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.99
09:44:47 PM    4    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:47 PM    5    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00
09:44:47 PM    6    2.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:47 PM    7    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00  100.00

09:44:47 PM  CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
09:44:48 PM  all    1.25    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.75
09:44:48 PM    0    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:48 PM    1    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:48 PM    2    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:48 PM    3    2.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:48 PM    4    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00
09:44:48 PM    5    0.99    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.01
09:44:48 PM    6    2.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   98.00
09:44:48 PM    7    1.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.00

Average:     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
Average:     all    1.03    0.00    0.15    0.03    0.00    0.00    0.00    0.00    0.00   98.80
Average:       0    0.40    0.00    0.40    0.00    0.00    0.00    0.00    0.00    0.00   99.20
Average:       1    0.40    0.00    0.00    0.20    0.00    0.00    0.00    0.00    0.00   99.40
Average:       2    1.40    0.00    0.20    0.00    0.00    0.00    0.00    0.00    0.00   98.40
Average:       3    1.20    0.00    0.20    0.00    0.00    0.00    0.00    0.00    0.00   98.59
Average:       4    0.20    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.80
Average:       5    0.80    0.00    0.00    0.00    0.00    0.00    0.00    0.00    0.00   99.20
Average:       6    3.59    0.00    0.20    0.00    0.00    0.00    0.00    0.00    0.00   96.21
Average:       7    0.20    0.00    0.20    0.00    0.00    0.00    0.00    0.00    0.00   99.60



$for cpu in 0 1 2 3 4 5 6 7; do
    echo "===== CPU $cpu ====="
    printf '\n' | \
        ./build-profile/benchmarks/matching_engine_profile \
        --cpu $cpu \
        | grep "Average latency"
done


===== CPU 0 =====
Average latency: 45.2506 ns/order
===== CPU 1 =====
Average latency: 45.8497 ns/order
===== CPU 2 =====
Average latency: 55.4517 ns/order
===== CPU 3 =====
Average latency: 45.1661 ns/order
===== CPU 4 =====
Average latency: 49.3999 ns/order
===== CPU 5 =====
Average latency: 45.3681 ns/order
===== CPU 6 =====
Average latency: 51.5792 ns/order
===== CPU 7 =====
Average latency: 62.2758 ns/order

CPU affinity was implemented successfully and made configurable through a command-line option (--cpu).

Measurements on the development machine showed that the selected logical CPU significantly affected latency. Some CPUs achieved ~45 ns/order, while others exceeded 60 ns/order.

This demonstrates that CPU affinity alone does not guarantee lower latency; selecting an appropriate CPU based on the system topology and workload is equally important.








