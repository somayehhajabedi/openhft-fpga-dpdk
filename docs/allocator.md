cmake -S . -B build-profile \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build-profile -j$(nproc)


./build-profile/benchmarks/allocator_benchmark \
    --benchmark_min_time=1s
2026-08-01T12:37:58-04:00
Running ./build-profile/benchmarks/allocator_benchmark
Run on (8 X 4797.14 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.54, 0.38, 0.30
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_NewDelete       10.1 ns         10.1 ns    100000000



./build-profile/benchmarks/allocator_benchmark \
    --benchmark_min_time=1s
[  9%] Built target udp_parser
[  9%] Built target ipv4_parser
[ 12%] Built target replay
[ 20%] Built target itch_parser
[ 21%] Built target ethernet_parser
[ 24%] Built target orderbook
[ 26%] Built target fixed_hash_map_benchmark
[ 28%] Built target platform_utils
[ 30%] Built target false_sharing_benchmark
[ 34%] Built target ethernet_test
[ 38%] Built target pcap_replay_reader_test
[ 40%] Built target risk
[ 41%] Built target ipv4_test
[ 37%] Building CXX object benchmarks/CMakeFiles/allocator_benchmark.dir/allocator_benchmark.cpp.o
[ 43%] Built target huge_page_benchmark
[ 50%] Built target matching_engine_core
[ 54%] Built target orderbook_benchmark
[ 54%] Built target bitmap_benchmark
[ 56%] Built target order_pool_benchmark
[ 59%] Built target gateway
[ 63%] Built target matching_engine_profile
[ 63%] Built target matching_engine_benchmark
[ 65%] Built target matching_engine_demo
[ 68%] Built target pipeline_benchmark
[ 98%] Built target parser_tests
[100%] Linking CXX executable allocator_benchmark
[100%] Built target allocator_benchmark
2026-08-01T13:18:10-04:00
Running ./build-profile/benchmarks/allocator_benchmark
Run on (8 X 4440.27 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.46, 0.29, 0.16
***WARNING*** Library was built as DEBUG. Timings may be affected.
----------------------------------------------------------
Benchmark                Time             CPU   Iterations
----------------------------------------------------------
BM_NewDelete          10.8 ns         10.8 ns    100000000
BM_PMRMonotonic       4.26 ns         4.26 ns    325636803




./build-profile/benchmarks/allocator_benchmark \
    --benchmark_min_time=1s
2026-08-01T13:42:24-04:00
Running ./build-profile/benchmarks/allocator_benchmark
Run on (8 X 4614.12 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.90, 0.38, 0.22
***WARNING*** Library was built as DEBUG. Timings may be affected.
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_NewDelete            10.2 ns         10.2 ns    100000000
BM_PMRMonotonic         3.90 ns         3.90 ns    354740665
BM_PMRBatch             2827 ns         2826 ns       495337 items_per_second=353.853M/s
BM_NewDeleteBatch      34300 ns        34289 ns        40900 items_per_second=29.1636M/s


## Benchmark Results

| Benchmark | Time | Throughput |
|---|---:|---:|
| `BM_NewDelete` | 10.2 ns | — |
| `BM_PMRMonotonic` | 3.90 ns | — |
| `BM_NewDeleteBatch` | 34.3 us | 29.16 M items/s |
| `BM_PMRBatch` | 2.83 us | 353.85 M items/s |

For single-object allocation, `std::pmr::monotonic_buffer_resource`
was approximately 2.6x faster than `new/delete`.

For batches of 1,000 objects, PMR was approximately 12.1x faster and
delivered about 12.1x higher throughput.

The larger batch improvement comes from performing many low-cost bump-pointer
allocations followed by one bulk release, rather than invoking the general
heap allocator and deallocator for every object.

This makes monotonic PMR resources suitable for short-lived batches,
temporary parsing objects, scratch storage, and event-processing phases.

They are less suitable for independently freed, long-lived objects; the
existing OrderPool remains a better fit for that ownership pattern.


//////////////////////////////////////////////////////////

Current replay path already reuses std::vector capacity.
Replacing it with std::pmr does not provide a meaningful benefit.






