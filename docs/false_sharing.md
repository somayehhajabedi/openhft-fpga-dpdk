cmake -S . -B build-profile \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build-profile -j$(nproc)


./build-profile/benchmarks/false_sharing_benchmark \
    --benchmark_min_time=1s
2026-08-01T11:55:25-04:00
Running ./build-profile/benchmarks/false_sharing_benchmark
Run on (8 X 4564.88 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.41, 0.38, 0.35
***WARNING*** Library was built as DEBUG. Timings may be affected.


------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_FalseSharing          422 ms        0.055 ms          100
BM_PaddedCounters       55.7 ms        0.051 ms          100

$ perf stat \
    -e cache-references,cache-misses,cycles,instructions \
    ./build-profile/benchmarks/false_sharing_benchmark \
    --benchmark_filter=BM_FalseSharing
2026-08-01T11:57:46-04:00
Running ./build-profile/benchmarks/false_sharing_benchmark
Run on (8 X 4345.22 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.50, 0.56, 0.43
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------
Benchmark                Time             CPU   Iterations
----------------------------------------------------------
BM_FalseSharing        339 ms        0.069 ms           10

 Performance counter stats for './build-profile/benchmarks/false_sharing_benchmark --benchmark_filter=BM_FalseSharing':

     1,470,705,352      cache-references                                                      
           509,162      cache-misses                                                          
    33,099,309,883      cycles                                                                
       946,789,191      instructions                                                          

       3.734460555 seconds time elapsed

       7.450380000 seconds user
       0.002000000 seconds sys



$ perf stat \
    -e cache-references,cache-misses,cycles,instructions \
    ./build-profile/benchmarks/false_sharing_benchmark \
    --benchmark_filter=BM_PaddedCounters
2026-08-01T11:58:56-04:00
Running ./build-profile/benchmarks/false_sharing_benchmark
Run on (8 X 4419.3 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.76, 0.62, 0.47
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_PaddedCounters       48.8 ms        0.067 ms          100

 Performance counter stats for './build-profile/benchmarks/false_sharing_benchmark --benchmark_filter=BM_PaddedCounters':

        10,831,718      cache-references                                                      
         1,707,012      cache-misses                                                          
    40,516,014,654      cycles                                                                
     8,990,521,558      instructions                                                          

       5.347289867 seconds time elapsed

      10.558380000 seconds user
       0.009998000 seconds sys


       ## Performance Results

The benchmark compared two atomic counters sharing one cache line with two
counters aligned to separate 64-byte cache lines.

Because Google Benchmark selected different iteration counts, hardware-counter
values were normalized per benchmark iteration.

| Metric | False Sharing | Padded Counters |
|---|---:|---:|
| Real time | 339 ms | 48.8 ms |
| Cycles per iteration | 3.31 billion | 405 million |
| Instructions per iteration | 94.7 million | 89.9 million |
| IPC | 0.029 | 0.222 |
| Cache references per iteration | 147.1 million | 108 thousand |
| Cache misses per iteration | 50.9 thousand | 17.1 thousand |

Separating the counters onto independent cache lines produced approximately
a 6.95x speedup and reduced cycles per iteration by about 87.8%.

The instruction counts remained relatively similar, indicating that the
performance loss was not caused by additional application work. It was caused
primarily by cache-coherency traffic and repeated ownership transfer of the
shared cache line between CPU cores.

This experiment demonstrates why frequently written per-thread state should
not share cache lines in low-latency concurrent systems.

