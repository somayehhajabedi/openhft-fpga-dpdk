

$ grep Huge /proc/meminfo 

grep Huge /proc/meminfo
AnonHugePages:         0 kB
ShmemHugePages:        0 kB
FileHugePages:     94208 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB


///////////////////////////

$ echo 128 | sudo tee /proc/sys/vm/nr_hugepages
$ grep Huge /proc/meminfo



$ perf stat \
    -e dTLB-load-misses,dTLB-loads,\
iTLB-load-misses,\
cache-misses,\
cache-references \
./build-profile/benchmarks/matching_engine_profile

CPU affinity disabled
Profiling workload is ready.
PID: 254575
Pairs per batch: 3000
Measured batches: 10000
Buy price levels: 16
Sell price: 100
Press Enter after attaching perf.

Orders processed: 60000000
Elapsed: 2.68881 seconds
Throughput: 2.23147e+07 orders/sec
Average latency: 44.8135 ns/order

 Performance counter stats for './build-profile/benchmarks/matching_engine_profile':

            25,572      dTLB-load-misses                                                        (79.98%)
     6,142,970,946      dTLB-loads                                                              (79.98%)
            20,047      iTLB-load-misses                                                        (80.02%)
           480,155      cache-misses                                                            (80.01%)
       539,957,600      cache-references                                                        (80.01%)

       3.756908828 seconds time elapsed

       2.693289000 seconds user
       0.003998000 seconds sys
////////////////////////////////////////////////////////////

$ perf stat \
    -e dTLB-load-misses,dTLB-loads,\
iTLB-load-misses,\
cache-misses,\
cache-references \
./build-profile/benchmarks/matching_engine_profile --cpu 3


CPU affinity enabled: CPU 3
Profiling workload is ready.
PID: 254591
Pairs per batch: 3000
Measured batches: 10000
Buy price levels: 16
Sell price: 100
Press Enter after attaching perf.

Orders processed: 60000000
Elapsed: 2.70653 seconds
Throughput: 2.21686e+07 orders/sec
Average latency: 45.1089 ns/order

 Performance counter stats for './build-profile/benchmarks/matching_engine_profile --cpu 3':

            22,216      dTLB-load-misses                                                        (79.99%)
     6,145,582,947      dTLB-loads                                                              (80.00%)
            16,995      iTLB-load-misses                                                        (79.97%)
         1,309,566      cache-misses                                                            (80.04%)
       568,214,713      cache-references                                                        (80.00%)

       3.913005115 seconds time elapsed

       2.711005000 seconds user
       0.004001000 seconds sys


 ////////////////////////////////////////////////////////////////

 ## Existing Matching Engine Baseline

| Metric | Value |
|---|---:|
| Average latency | 43.81 ns/order |
| Throughput | 22.82 M orders/s |
| dTLB loads | 6,138,933,029 |
| dTLB load misses | 34,654 |
| dTLB miss rate | 0.00056% |
| Cache references | 615,378,060 |
| Cache misses | 502,072 |
| Cache miss rate | 0.0816% |

The existing matching-engine workload already exhibits a very low dTLB miss
rate. Therefore, a dedicated large-memory benchmark is required to evaluate
HugeTLB behavior under meaningful TLB pressure.


./build-profile/benchmarks/huge_page_benchmark \
    --benchmark_min_time=1s
2026-08-01T10:49:19-04:00
Running ./build-profile/benchmarks/huge_page_benchmark
Run on (8 X 4299.95 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.62, 0.63, 0.50
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------------------------
Benchmark               Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------
BM_NormalPages      0.630 ms        0.630 ms         2262 bytes_per_second=397.031Gi/s
BM_HugePages        0.590 ms        0.589 ms         2268 bytes_per_second=424.368Gi/s


printf '\n' | perf stat \
    -e dTLB-loads,dTLB-load-misses \
    ./build-profile/benchmarks/huge_page_benchmark \
    --benchmark_filter=BM_NormalPages
2026-08-01T10:52:15-04:00
Running ./build-profile/benchmarks/huge_page_benchmark
Run on (8 X 4538.86 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.33, 0.55, 0.49
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------------------------
Benchmark               Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------
BM_NormalPages      0.610 ms        0.610 ms         1157 bytes_per_second=409.873Gi/s

 Performance counter stats for './build-profile/benchmarks/huge_page_benchmark --benchmark_filter=BM_NormalPages':

       418,992,797      dTLB-loads                                                            
        44,499,256      dTLB-load-misses                                                      

       1.108840221 seconds time elapsed

       0.848721000 seconds user
       0.259914000 seconds sys



printf '\n' | perf stat \
    -e dTLB-loads,dTLB-load-misses \
    ./build-profile/benchmarks/huge_page_benchmark \
    --benchmark_filter=BM_HugePages
2026-08-01T10:57:33-04:00
Running ./build-profile/benchmarks/huge_page_benchmark
Run on (8 X 4787.15 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.44, 0.39, 0.42
***WARNING*** Library was built as DEBUG. Timings may be affected.
-----------------------------------------------------------------------
Benchmark             Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------
BM_HugePages      0.570 ms        0.570 ms         1075 bytes_per_second=438.602Gi/s

 Performance counter stats for './build-profile/benchmarks/huge_page_benchmark --benchmark_filter=BM_HugePages':

        81,426,770      dTLB-loads                                                            
            15,345      dTLB-load-misses                                                      

       0.708078526 seconds time elapsed

       0.665730000 seconds user
       0.041983000 seconds sys



## HugeTLB Benchmark Results

A dedicated 256 MB memory benchmark was used to compare standard 4 KB pages
with explicit 2 MB HugeTLB pages.

The workload touched one byte every 4 KB, intentionally creating significant
data-TLB pressure.

| Metric | Normal Pages | HugeTLB Pages |
|---|---:|---:|
| Benchmark time | 0.610 ms | 0.570 ms |
| Throughput | 409.87 GiB/s | 438.60 GiB/s |
| dTLB loads | 418,992,797 | 81,426,770 |
| dTLB load misses | 44,499,256 | 15,345 |
| dTLB miss rate | 10.62% | 0.0188% |
| dTLB misses per iteration | 38,461 | 14.3 |

HugeTLB reduced dTLB misses per iteration by approximately 2,694x.

The benchmark also observed approximately 6.6% lower execution time and
7% higher reported throughput. However, the CPU frequency differed between
the two runs, so the timing improvement should be interpreted cautiously.

The hardware-counter results provide the strongest evidence: explicit 2 MB
HugeTLB pages almost completely removed the dTLB pressure created by the
4 KB-stride workload.

These results do not imply that Huge Pages improve every workload. The
matching-engine baseline already exhibited a very low dTLB miss rate, so its
benefit depends on the working-set size and memory-access pattern.
  





