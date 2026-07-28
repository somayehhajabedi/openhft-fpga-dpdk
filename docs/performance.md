# Performance Baseline

## Matching Engine Insert Benchmark

Build:

```bash
rm -rf build

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENHFT_BUILD_TESTS=ON

cmake --build build -j$(nproc)


./build/benchmarks/matching_engine_benchmark \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true



Baseline result:

Mean CPU:        65,292 ns per 1024 orders
Median CPU:      63,903 ns per 1024 orders
Mean latency:    ~63.8 ns/order
Median latency:  ~62.4 ns/order
Throughput:      ~15.7 million orders/second
CV:              3.46%


Environment notes:
CPU scaling enabled
Google Benchmark library reported as DEBUG

```bash
git add docs/performance.md
git commit -m "Document matching engine performance baseline"
git push



./build/benchmarks/matching_engine_benchmark \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  | tee benchmark_results/matching_engine_baseline.txt

///////////////////////////////////////////////////////////////////

# P03 — Pre-size Order Index

## Objective

Reduce allocation overhead caused by `std::unordered_map` growth.

## Background

Heaptrack showed that `ArrayOrderBook::addOrder()` dominated allocation calls.

## Changes

Added a constructor to `ArrayOrderBook`:

```cpp
ArrayOrderBook::ArrayOrderBook(std::size_t order_capacity)
{
    order_index_.max_load_factor(0.7f);
    order_index_.reserve(order_capacity);
}
```

## Build

```bash
cmake -S . -B build-release \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENHFT_BUILD_TESTS=ON

cmake --build build-release -j$(nproc)
```

## Validation

```bash
ctest --test-dir build-release --output-on-failure
```

Result:

```
67/67 tests passed
```

## Benchmark

### Command

```bash
./build-release/benchmarks/matching_engine_benchmark \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true
```

### Result

```
Mean throughput : 16.76M orders/s
Mean latency    : 61,297 ns/batch
≈59.9 ns/order
CV              : 5.52%
```

## Heap Analysis

### Record

```bash
heaptrack \
    ./build-release/benchmarks/matching_engine_benchmark \
    --benchmark_min_time=3s
```

### Analyze

```bash
latest=$(ls -t heaptrack.matching_engine_benchmark.*.zst | head -1)

heaptrack_print "$latest" | head -120
```

### Key Findings

```
13,249,536 allocation calls

ArrayOrderBook::addOrder()
```

```
std::_Hashtable<>::_M_rehash(...)
```

performed during

```
ArrayOrderBook::ArrayOrderBook()
```

## Analysis

`reserve()` removes repeated bucket growth by allocating the bucket table up front.

However, `std::unordered_map` still performs one heap allocation for each inserted node.

## Conclusion

This optimization is correct and should be kept.

The remaining hot-path allocations originate from the node-based implementation of `std::unordered_map`.

The next optimization will replace the order index with an allocation-free data structure.


///////////////////////////////////////////////////////////////////////////////////

## P04 — Fixed-Capacity Hash Map

### 1. Objective

Replace the allocation-heavy `std::unordered_map` used for order lookup with a fixed-capacity, allocation-free hash map suitable for low-latency workloads.

The goals of this phase were:

- eliminate runtime node allocations;
- improve cache locality;
- reduce insertion, lookup, and erase latency;
- provide deterministic memory usage;
- support collision handling through open addressing;
- support deletion through tombstones;
- validate correctness before integrating the structure into `ArrayOrderBook`.

---

### 2. Background

Previous profiling of `ArrayOrderBook::addOrder()` identified `std::unordered_map` as a major source of runtime allocations.

Heaptrack reported approximately:

```text
12.2 million allocations
```

originating from order insertion and lookup activity associated with the node-based hash map.

During P03, the existing `std::unordered_map` was optimized using:

```cpp
reserve(...)
max_load_factor(0.7f)
```

This removed runtime rehashing and improved matching-engine throughput from approximately:

```text
15.1 million orders/second
```

to:

```text
16.76 million orders/second
```

However, reserving capacity did not eliminate per-node allocations because `std::unordered_map` still dynamically allocates individual nodes.

To remove this remaining allocation overhead, P04 introduced a custom fixed-capacity hash map based on contiguous storage and linear probing.

---

### 3. Code Changes

A new header-only container was implemented:

```text
common/fixed_hash_map.hpp
```

The structure uses:

- compile-time fixed capacity;
- `std::array` for contiguous storage;
- open addressing;
- linear probing;
- no runtime memory allocation;
- three entry states:
  - `Empty`
  - `Occupied`
  - `Deleted`

The following public operations were implemented:

```cpp
insert(...)
find(...)
erase(...)
contains(...)
size()
capacity()
empty()
```

#### Collision handling

When two keys map to the same initial slot, the map searches subsequent slots using linear probing until it finds:

- the requested key;
- an empty slot;
- or the end of the probe sequence.

#### Tombstone handling

Deleted entries are marked as:

```cpp
EntryState::Deleted
```

rather than immediately becoming empty.

This is necessary because marking a deleted slot as empty could prematurely terminate a later lookup and make entries located farther along the probe chain unreachable.

#### Tombstone correctness fix

The initial implementation reused the first deleted slot immediately during insertion.

That behavior introduced a subtle correctness issue.

Consider the following sequence:

```cpp
map.insert(1, 10);
map.insert(5, 20);  // collision with key 1

map.erase(1);       // first slot becomes Deleted
map.insert(5, 30);  // key 5 already exists later in the probe chain
```

If insertion stops at the first deleted slot, key `5` is inserted a second time instead of updating the existing entry.

This can produce:

- duplicate keys;
- incorrect lookup behavior;
- an incorrect `size_` value.

The insertion algorithm was corrected to:

1. remember the first deleted slot;
2. continue probing after the tombstone;
3. search for an existing matching key;
4. update the existing value if the key is found;
5. otherwise insert into the first remembered deleted slot;
6. use an empty slot when no earlier tombstone exists.

This preserves both correctness and tombstone reuse.

---

### 4. Files Modified

The following files were added or modified:

```text
common/fixed_hash_map.hpp
tests/unit/common/fixed_hash_map_test.cpp
tests/CMakeLists.txt
benchmarks/fixed_hash_map_benchmark.cpp
benchmarks/CMakeLists.txt
```

#### New implementation

```text
common/fixed_hash_map.hpp
```

Contains the fixed-capacity hash map implementation.

#### New unit tests

```text
tests/unit/common/fixed_hash_map_test.cpp
```

Contains correctness and regression tests for insertion, lookup, collision handling, deletion, tombstones, and full-capacity behavior.

#### Test integration

```text
tests/CMakeLists.txt
```

Updated to compile the FixedHashMap tests as part of the existing test executable.

#### New benchmark

```text
benchmarks/fixed_hash_map_benchmark.cpp
```

Compares `FixedHashMap` against `std::unordered_map` for:

- insertion;
- lookup;
- erase.

#### Benchmark integration

```text
benchmarks/CMakeLists.txt
```

Added the new executable target:

```text
fixed_hash_map_benchmark
```

---

### 5. Commands Executed

#### Configure and build the development tree

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

#### Run the complete test suite

```bash
ctest --test-dir build --output-on-failure
```

#### Run the initial benchmark

```bash
./build/benchmarks/fixed_hash_map_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_report_aggregates_only=true
```

#### Configure the Release build

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release
```

#### Build the Release tree

```bash
cmake --build build-release -j$(nproc)
```

#### Verify Release configuration

```bash
grep CMAKE_BUILD_TYPE build-release/CMakeCache.txt
```

Output:

```text
CMAKE_BUILD_TYPE:STRING=Release
```

#### Verify compiler flags

```bash
cat \
  build-release/benchmarks/CMakeFiles/fixed_hash_map_benchmark.dir/flags.make
```

Output:

```text
CXX_FLAGS = -O3 -DNDEBUG -std=c++20
```

#### Run and save the Release benchmark

```bash
mkdir -p benchmark_results

./build-release/benchmarks/fixed_hash_map_benchmark \
  --benchmark_repetitions=10 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=benchmark_results/p04_fixed_hash_map_release.json \
  --benchmark_out_format=json
```

The raw benchmark result was saved to:

```text
benchmark_results/p04_fixed_hash_map_release.json
```

---

### 6. Validation

The complete test suite passed:

```text
100% tests passed
0 tests failed out of 76
```

Total test time:

```text
0.25 seconds
```

The FixedHashMap test suite included the following tests:

```text
FixedHashMapTest.InsertAndFind
FixedHashMapTest.Contains
FixedHashMapTest.UpdateExistingKey
FixedHashMapTest.Erase
FixedHashMapTest.Collision
FixedHashMapTest.ReuseDeletedSlot
FixedHashMapTest.FullTable
FixedHashMapTest.UpdatesExistingKeyBeyondDeletedSlot
FixedHashMapTest.EraseMissingKeyReturnsFalse
```

#### Insert and lookup

Verified that an inserted key can be found and that the stored value is returned correctly.

#### Contains

Verified correct behavior for existing and missing keys.

#### Existing-key update

Verified that reinserting an existing key updates its value without increasing the map size.

#### Erase

Verified that an existing key can be removed and can no longer be found.

#### Collision handling

Verified that multiple keys with the same initial bucket can coexist and remain accessible.

#### Deleted-slot reuse

Verified that a tombstone can be reused by a later insertion.

#### Full-table behavior

Verified that insertion fails correctly when the map reaches its fixed capacity.

#### Existing key beyond deleted slot

A regression test was added for the tombstone duplicate-key issue:

```text
FixedHashMapTest.UpdatesExistingKeyBeyondDeletedSlot
```

The test verifies that the map:

- continues probing after a deleted slot;
- finds the existing key;
- updates the existing value;
- does not create a duplicate;
- does not incorrectly increase the map size.

#### Erasing a missing key

Verified that erasing a nonexistent key returns failure without modifying the map.

---

### 7. Benchmark

The benchmark compared:

```text
FixedHashMap
```

against:

```text
std::unordered_map
```

#### Benchmark configuration

```text
FixedHashMap capacity: 4096
Number of inserted items: 2048
Approximate load factor: 50%
Benchmark repetitions: 10
Build type: Release
Compiler optimization: -O3
Assertions disabled: -DNDEBUG
C++ standard: C++20
```

System information reported by Google Benchmark:

```text
CPU threads: 8
Reported CPU frequency: 4900 MHz
L1 Data cache: 32 KiB x4
L1 Instruction cache: 32 KiB x4
L2 Unified cache: 256 KiB x4
L3 Unified cache: 8192 KiB x1
```

Google Benchmark reported that CPU frequency scaling was enabled:

```text
CPU scaling is enabled
```

This means some timing noise may remain in the measurements.

Google Benchmark also reported:

```text
Library was built as DEBUG
```

The project benchmark executable itself was confirmed to be compiled in Release mode using:

```text
-O3 -DNDEBUG -std=c++20
```

Therefore, the debug warning refers to the installed Google Benchmark library rather than the project code.

#### Release benchmark results

| Operation | FixedHashMap CPU mean | std::unordered_map CPU mean | Improvement |
|---|---:|---:|---:|
| Insert, 2048 items | 4,125 ns | 95,895 ns | 23.2x |
| Find | 1.68 ns | 6.55 ns | 3.9x |
| Erase, 2048 items | 2,441 ns | 59,924 ns | 24.5x |

#### Per-item latency

| Operation | FixedHashMap | std::unordered_map |
|---|---:|---:|
| Insert | 2.01 ns/item | 46.82 ns/item |
| Erase | 1.19 ns/item | 29.26 ns/item |

The per-item values were calculated as follows:

```text
FixedHashMap insert:
4125 ns / 2048 items = 2.01 ns/item

std::unordered_map insert:
95895 ns / 2048 items = 46.82 ns/item

FixedHashMap erase:
2441 ns / 2048 items = 1.19 ns/item

std::unordered_map erase:
59924 ns / 2048 items = 29.26 ns/item
```

#### Throughput

| Operation | FixedHashMap | std::unordered_map |
|---|---:|---:|
| Insert | 496.51 million items/s | 21.38 million items/s |
| Erase | 841.66 million items/s | 34.40 million items/s |

#### Runtime variability

| Operation | FixedHashMap CV | std::unordered_map CV |
|---|---:|---:|
| Insert | 1.23% | 3.27% |
| Find | 0.75% | 4.29% |
| Erase | 5.98% | 9.12% |

The FixedHashMap showed lower variability for all three measured operations.

This is especially relevant in low-latency systems, where predictable execution time can be as important as average throughput.

---

### 8. Profiling / Heaptrack

No separate Heaptrack session was required for the standalone FixedHashMap microbenchmark because the implementation uses fixed internal storage:

```cpp
std::array<Entry, Capacity>
```

The container performs no dynamic allocation during:

- insertion;
- lookup;
- erase;
- collision probing;
- tombstone reuse.

The motivation for this phase came from the earlier Heaptrack analysis of the full order-book benchmark.

That profiling session identified approximately:

```text
12.2 million allocations
```

associated with the `std::unordered_map` used by `ArrayOrderBook`.

P03 removed runtime rehashing through preallocation but retained node allocation.

P04 provides the allocation-free replacement required to eliminate that remaining source of heap activity.

A new Heaptrack run will be performed after the FixedHashMap is integrated into `ArrayOrderBook`.

The post-integration profiling goals are:

- verify that unordered-map node allocations disappear;
- compare total allocation count with the P03 result;
- identify any remaining allocation sources;
- measure whether the matching-engine hotspot changes;
- confirm that allocation removal improves end-to-end throughput.

---

### 9. Results

The FixedHashMap implementation passed all correctness tests and significantly outperformed `std::unordered_map`.

Key results:

- all 76 project tests passed;
- insertion was approximately 23.2 times faster;
- lookup was approximately 3.9 times faster;
- erase was approximately 24.5 times faster;
- insertion used approximately 2.01 ns per item;
- erase used approximately 1.19 ns per item;
- fixed storage eliminated runtime container allocation;
- contiguous storage improved cache locality;
- insertion and lookup showed low runtime variability;
- memory capacity became deterministic and known at compile time.

The implementation also correctly handled:

- collisions;
- full-table behavior;
- existing-key updates;
- erased slots;
- tombstone reuse;
- missing-key deletion;
- existing keys located beyond deleted slots.

---

### 10. Analysis

The performance difference between the two containers comes primarily from their memory layouts and allocation strategies.

#### std::unordered_map characteristics

`std::unordered_map` is generally implemented as a node-based container.

Its costs may include:

- one dynamic allocation per node;
- allocator metadata;
- pointer chasing;
- non-contiguous nodes;
- lower cache locality;
- additional indirection;
- bucket-array access;
- possible rehashing unless capacity is reserved.

P03 removed rehashing from the benchmarked workload, but node allocation and pointer traversal remained.

#### FixedHashMap characteristics

The custom FixedHashMap uses:

- one contiguous `std::array`;
- no node allocation;
- direct indexed access;
- linear probing;
- compile-time capacity;
- predictable memory ownership;
- compact entry traversal.

These characteristics are well suited to the order lookup path of a low-latency matching engine.

#### Effect of the tombstone correctness fix

Before the tombstone insertion logic was corrected, the FixedHashMap insertion benchmark measured approximately:

```text
1972 ns for 2048 insertions
```

After the correctness fix, the Release result measured:

```text
4125 ns for 2048 insertions
```

The corrected insertion path is approximately 2.1 times slower than the original implementation.

The additional cost occurs because insertion can no longer stop immediately at the first deleted slot.

It must continue probing to determine whether the key already exists later in the cluster.

This added work is necessary to prevent duplicate keys and incorrect size tracking.

Despite the additional probing, the corrected implementation remained approximately:

```text
23.2 times faster
```

than `std::unordered_map` for insertion.

This is an acceptable and necessary trade-off.

A faster implementation that violates map semantics cannot be used in the order book.

#### Predictability

The FixedHashMap coefficient of variation was lower than `std::unordered_map` for insertion, lookup, and erase.

For example:

```text
FixedHashMap find CV: 0.75%
std::unordered_map find CV: 4.29%
```

This suggests that the fixed-storage implementation provides not only lower average latency but also more stable execution time.

For HFT and matching-engine workloads, reduced latency variation is valuable because it helps reduce jitter and improves tail-latency predictability.

#### Capacity trade-off

The container has fixed capacity.

This provides:

- deterministic memory consumption;
- no resizing;
- no rehashing;
- no runtime allocation.

However, it also means the maximum number of entries must be selected in advance.

When integrated into `ArrayOrderBook`, the capacity must be chosen based on:

- expected maximum active orders;
- memory footprint;
- desired maximum load factor;
- collision behavior;
- failure handling when capacity is exhausted.

The order book must explicitly handle failed insertion rather than assuming unlimited growth.

#### Benchmark limitations

The current benchmark provides strong microbenchmark evidence, but it does not yet prove the same improvement at the complete matching-engine level.

The final impact will depend on:

- the percentage of total runtime spent in order lookup;
- order allocation costs outside the map;
- matching logic;
- price-level operations;
- cache effects in the full order book;
- workload distribution;
- insertion, cancellation, replacement, and execution ratios.

Therefore, the next phase must measure the full `ArrayOrderBook` and matching-engine workloads after integration.

---

### 11. Conclusion

P04 successfully produced a correct, allocation-free fixed-capacity hash map for low-latency order lookup.

The implementation provides:

- deterministic memory usage;
- fixed compile-time capacity;
- contiguous storage;
- no runtime allocation;
- collision handling;
- tombstone deletion;
- correct deleted-slot reuse;
- correct existing-key updates;
- substantially lower latency than `std::unordered_map`;
- lower runtime variability in the tested operations.

The benchmark demonstrated the following approximate improvements:

```text
Insert: 23.2x
Find:    3.9x
Erase:  24.5x
```

All project tests passed:

```text
76/76
```

The FixedHashMap is ready to be integrated into `ArrayOrderBook`.

---

### 12. Next Step

The next phase will replace the order lookup container inside `ArrayOrderBook`.

Current structure:

```cpp
std::unordered_map<OrderId, Order*> orders_;
```

Planned replacement:

```cpp
FixedHashMap<OrderId, Order*, Capacity> orders_;
```

The integration phase will include:

1. selecting an appropriate fixed capacity;
2. replacing `std::unordered_map` lookup operations;
3. handling insertion failure when capacity is exhausted;
4. updating lookup and erase call sites;
5. running all unit and integration tests;
6. rerunning the order-book benchmark;
7. rerunning the matching-engine benchmark;
8. comparing throughput against the P03 result;
9. rerunning `perf`;
10. rerunning Heaptrack;
11. verifying that unordered-map node allocations are eliminated;
12. documenting the end-to-end performance impact.

The primary comparison point will be the P03 matching-engine result:

```text
Approximately 16.76 million orders/second
Approximately 59.9 ns/order
```

The next phase will determine whether the strong standalone FixedHashMap results translate into measurable full-system improvement.

//////////////////////////////////////////////////////////////////////////////////

# P05 – Integrate FixedHashMap into ArrayOrderBook

## Objective

Replace the allocation-heavy `std::unordered_map` used by `ArrayOrderBook`
with the custom allocation-free `FixedHashMap` implemented during P04,
then validate correctness, benchmark performance, and profile the
matching engine.

---

## Background

P04 demonstrated that `FixedHashMap` significantly outperformed
`std::unordered_map` in isolated microbenchmarks while completely
eliminating per-node heap allocations.

The next step was integrating the container into the production
matching engine to evaluate its impact on real application performance.

---

## Code Changes

The order lookup container inside `ArrayOrderBook` was replaced.

Previous implementation:

```cpp
std::unordered_map<OrderId, Order*> order_index_;
```

New implementation:

```cpp
FixedHashMap<
    OrderId,
    Order*,
    DefaultOrderCapacity> order_index_;
```

The order book implementation was updated to use the
`FixedHashMap` interface.

Major API changes included:

- replacing `operator[]` with `insert()`;
- replacing iterator-based lookup with pointer-based lookup;
- replacing iterator erase with key erase;
- removing `reserve()` and `max_load_factor()` since the hash map
  has fixed compile-time capacity.

---

## Files Modified

```
common/fixed_hash_map.hpp
orderbook/software/array_order_book.hpp
orderbook/software/array_order_book.cpp
tests/
benchmarks/
```

---

## Commands Executed

Build

```bash
cmake --build build -j$(nproc)
```

Validation

```bash
ctest --test-dir build --output-on-failure
```

Release Benchmark

```bash
./build-release/benchmarks/matching_engine_benchmark \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=true \
    --benchmark_out=benchmark_results/p05_matching_engine_fixed_hash_map_release.json \
    --benchmark_out_format=json
```

Profiling

```bash
perf record -e cycles:u \
    ./build-release/benchmarks/matching_engine_benchmark
```

---

## Validation

All regression tests passed.

```
76 / 76 tests passed
0 failures
```

The FixedHashMap integration preserved the existing behaviour of the
matching engine and order book.

---

## Benchmark

Release benchmark results:

| Metric | Value |
|---------|------:|
| Mean throughput | **15.62 M orders/sec** |
| Median throughput | **15.51 M orders/sec** |
| Coefficient of variation | **11.02%** |

A subsequent profiling run reported approximately:

```
18.15 M orders/sec
```

However, due to CPU frequency scaling and benchmark variability,
the release benchmark above is treated as the official measurement.

---

## Profiling

User-space profiling was performed using:

```bash
perf record -e cycles:u
```

Major samples:

```
67.38%  __memset_avx2_unaligned_erms
10.93%  ArrayOrderBook::addOrder()
```

Notably,

```
FixedHashMap::insert()
FixedHashMap::find()
```

did **not** appear among the dominant hotspots.

---

## Analysis

The integration successfully removed `std::unordered_map`
from the order lookup path.

Although isolated microbenchmarks showed large improvements,
the end-to-end matching engine benchmark did not demonstrate
a measurable throughput improvement.

Profiling initially suggested that most CPU time was spent
inside `memset`.

Further investigation revealed that this observation resulted
from profiling the complete benchmark execution.

Google Benchmark excludes benchmark setup from its timing by
calling `PauseTiming()` and `ResumeTiming()`, but `perf`
samples the entire process, including object construction,
memory initialization, and benchmark setup.

Therefore the large `memset` hotspot does **not** necessarily
represent the hot path executed while processing orders.

The profiling session nevertheless confirmed an important result:

The custom hash map is no longer a dominant runtime cost.

---

## Results

✓ Successfully replaced `std::unordered_map` inside the order book.

✓ Eliminated dynamic hash table growth from the hot lookup path.

✓ Passed all correctness tests.

✓ Successfully integrated the custom container into the production
matching engine.

✓ Established a reproducible benchmarking and profiling workflow.

---

## Conclusion

P05 completed the transition from a standard library hash table
to a custom fixed-capacity hash map suitable for low-latency
applications.

The integration was successful and maintained correctness.

Performance measurements indicate that the primary bottleneck is
no longer order lookup.

Future optimization efforts should focus on profiling workloads
that isolate the true matching-engine hot path while excluding
benchmark setup overhead.

---

## Next Step

P06

Develop a dedicated profiling benchmark that:

- constructs the matching engine only once;
- prepares benchmark data outside the measured region;
- profiles only the steady-state order processing path;
- identifies the next true runtime bottleneck before additional
optimizations are implemented.


////////////////////////////////////////////////////////////////////////////////////


# P06 – Investigate and Optimize Matching Engine Performance

## Objective

Investigate the unexpected performance regression observed after the
`FixedHashMap` integration completed in P05.

Develop a dedicated profiling benchmark that isolates steady-state
order processing, identify the true runtime bottleneck, implement
the required optimization, and validate the improvement through
benchmarking and CPU profiling.


---

## Background

P05 successfully integrated `FixedHashMap` into the production
matching engine.

The migration eliminated dynamic hash table allocations from the
order lookup path while preserving the correctness of the order
book implementation.

However, release benchmarks produced an unexpected result.

Instead of improving throughput, the matching engine became
significantly slower than the previous implementation.

The regression was large enough that it could not be explained
by normal benchmark variation.

This suggested that a new runtime bottleneck had been introduced
during the integration.

The next step was to isolate the hot execution path, identify the
source of the regression, and restore the expected performance.

---

## Regression

A dedicated release benchmark was executed after completing the
`FixedHashMap` integration.

The initial expectation was a measurable throughput improvement.

Instead, the benchmark revealed a significant performance regression.

Observed throughput dropped to approximately:

| Metric | Value |
|---------|------:|
| Throughput | **2.7 M orders/sec** |

The regression was substantially larger than normal benchmark
variation.

This indicated that the slowdown originated from the application
itself rather than measurement noise.

At this stage, the implementation was functionally correct.

All regression tests continued to pass.

The problem was therefore limited to runtime performance.

Additional optimizations were intentionally postponed until the
root cause of the regression could be identified.


---

---

## Benchmark Design

The existing Google Benchmark measured both benchmark setup and
steady-state order processing.

This made CPU profiling difficult because benchmark preparation
introduced additional execution time that was unrelated to the
matching engine.

To isolate the hot execution path, a dedicated profiling benchmark
was developed.

The new benchmark constructs the matching engine only once.

Benchmark data is prepared outside the measured region.

Each iteration measures only steady-state order processing.

This produces a workload that is significantly more suitable for
CPU profiling using Linux `perf`.

The benchmark also generates a deterministic order flow, allowing
profiling results to remain reproducible across multiple runs.


---

## Implementation Changes

A dedicated profiling benchmark was developed for the matching engine.

Unlike the previous benchmark, the matching engine is constructed
only once before benchmarking begins.

Benchmark data is generated outside the measured region.

This prevents setup operations from affecting the measured
execution time.

The workload was redesigned to produce deterministic order flow.

Buy orders are distributed across multiple price levels.

Sell orders are submitted at a fixed execution price.

Each benchmark iteration completely empties the order book.

This guarantees identical initial conditions for every iteration.

The benchmark was also updated to support CPU profiling with
Linux `perf`.

Finally, the implementation was adjusted to eliminate the
performance regression identified during profiling.


---

## Files Modified

```
benchmarks/matching_engine_profile.cpp

common/fixed_hash_map.hpp

benchmark_results/

docs/performance.md
```

---

## Commands Executed

Build

```bash
cmake --build build -j$(nproc)
```

AddressSanitizer

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DOPENHFT_ENABLE_ASAN=ON

cmake --build build-asan -j$(nproc)

./build-asan/benchmarks/matching_engine_profile
```

Release Benchmark

```bash
./build-release/benchmarks/matching_engine_profile
```

CPU Profiling

```bash
perf record -e cycles:u \
    ./build-release/benchmarks/matching_engine_profile

perf report
```


---

## Validation

The dedicated profiling benchmark was first executed with
AddressSanitizer enabled.

The initial run immediately reported a memory error.

Investigation showed that benchmark iterations reused `Order`
objects while stale pointers remained inside the order book.

As a result, previously removed orders could be referenced after
their internal state had been reset.

The benchmark workload was redesigned to completely empty the
order book after every batch.

This guarantees that each iteration starts from a clean state.

After the workload redesign, the benchmark completed without any
AddressSanitizer errors.

Memory correctness was therefore validated before continuing with
performance analysis.

---

## Benchmark

The dedicated profiling benchmark was executed after the workload
had been redesigned and validated with AddressSanitizer.

The benchmark measures only steady-state order processing.

Object construction and benchmark preparation are excluded from
the measured region.

The initial benchmark produced the following result.

| Metric | Value |
|---------|------:|
| Throughput | **2.7 M orders/sec** |

The measured throughput was significantly lower than the previous
implementation.

The regression confirmed that the slowdown remained after fixing
the benchmark workload.

The next step was identifying where CPU time was being spent.


---

## Profiling

CPU profiling was performed using Linux `perf`.

The dedicated benchmark isolates steady-state order processing,
allowing the collected samples to accurately represent the hot
execution path.

Initial profiling reported the following hotspot.

```
ArrayOrderBook::addOrder()
```

Approximately 96% of all CPU samples were attributed to this
function.

This result immediately narrowed the investigation to the order
insertion path.

However, `ArrayOrderBook::addOrder()` performs several operations.

The profiling result identified where time was spent, but not
why the slowdown occurred.

A deeper investigation of the insertion path was therefore
required.


---

## Root Cause Analysis

The profiling results suggested that most CPU time was spent inside
`ArrayOrderBook::addOrder()`.

A detailed code review showed that the function itself performs only
a small amount of work.

Its primary responsibility is forwarding new orders to the order
lookup container.

This observation suggested that the actual bottleneck might exist
inside the hash table implementation rather than inside the order
book itself.

The insertion path of `FixedHashMap` was therefore investigated.

The implementation uses open addressing with linear probing.

Deleted entries are represented using tombstones to preserve probe
chains during lookups.

While this strategy avoids expensive table reorganization after each
erase operation, it introduces another risk.

As the benchmark repeatedly inserts and removes orders, tombstones
gradually accumulate inside the table.

Although the number of active orders remains nearly constant, the
number of occupied buckets continuously increases.

As a result, every insertion must probe more buckets before finding
an available slot.

The average probe length therefore increases over time.

The hash table gradually becomes slower even though its logical size
does not grow.

This behaviour explains why `ArrayOrderBook::addOrder()` appeared as
the dominant hotspot during CPU profiling.

The slowdown originated from progressively longer probe chains inside
`FixedHashMap::insert()`.


### Root Cause Summary

The performance regression was **not** caused by the matching engine.

It was **not** caused by the benchmark.

It was **not** caused by AddressSanitizer.

The regression originated from tombstone accumulation inside
`FixedHashMap`, which progressively increased the average probe
length during insertion.


---

## Optimization

The profiling results indicated that insertion performance degraded
as tombstones accumulated inside the hash table.

The optimization focused on preventing probe chains from growing
during long benchmark runs.

The tombstone handling logic inside `FixedHashMap` was revised.

Unused buckets are now reclaimed before probe chains become
excessively long.

This prevents deleted entries from accumulating indefinitely.

No changes were required to the matching engine.

No changes were required to the order book implementation.

The optimization was completely contained within the hash table.

After the implementation was updated, the benchmark was executed
again using the same workload and profiling configuration.

---

## Final Validation

The optimized implementation was first validated using
AddressSanitizer.

The benchmark completed successfully without reporting memory
errors.

Regression tests continued to pass after the optimization.

Repeated benchmark runs produced consistent throughput results.

The optimization therefore improved performance while preserving
correctness.

---

## Results

The optimization completely eliminated the observed performance
regression.

Final benchmark results are summarized below.

| Metric | Value |
|---------|------:|
| Orders processed | **60,000,000** |
| Elapsed time | **0.97 s** |
| Throughput | **61.7 M orders/sec** |
| Average latency | **16.2 ns/order** |

Compared to the initial profiling benchmark:

| Metric | Before | After |
|---------|-------:|------:|
| Throughput | 2.7 M orders/sec | **61.7 M orders/sec** |

CPU profiling also changed significantly.

`ArrayOrderBook::addOrder()` was no longer the dominant hotspot.

The optimization successfully restored the expected insertion
performance of the hash table.

### Performance Summary

| Phase | Throughput |
|------|-----------:|
| P05 Release Benchmark | 15.6 M orders/sec |
| Initial Profiling Benchmark | 2.7 M orders/sec |
| After Optimization | **61.7 M orders/sec** |


## Lessons Learned

This investigation reinforced several important engineering
principles.

Performance optimizations should always be validated with
measurements.

Microbenchmark improvements do not necessarily translate into
application-level performance gains.

Profiling should isolate the steady-state execution path whenever
possible.

Correctness must be verified before interpreting performance
results.

Small implementation details, such as tombstone accumulation,
can have a significant impact on long-running workloads.

Finally, evidence should always drive optimization decisions.
Assumptions alone are insufficient.

---

## Conclusion

This investigation identified the root cause of the unexpected
performance regression observed after integrating `FixedHashMap`.

Profiling showed that insertion performance gradually degraded as
tombstones accumulated during long-running workloads.

The tombstone handling logic was optimized to prevent excessive
probe chain growth.

After the optimization, throughput increased from approximately
2.7 million orders per second to over 61 million orders per
second.

The optimized implementation was validated using AddressSanitizer,
unit tests, and repeated benchmark runs.

The matching engine now achieves stable and predictable
performance under sustained workloads.

---

## Next Step

The next phase will focus on profiling and optimizing the complete
market data processing pipeline.

This includes measuring end-to-end latency from packet reception
to order book updates and identifying additional optimization
opportunities across the parsing and processing stages.

///////////////////////////////////////////////////////////////////////////////


# P07 – P07 – End-to-End ITCH Processing Pipeline Benchmark


## Objective

Evaluate the performance of the complete software market data
processing pipeline.

Measure end-to-end latency and throughput from ITCH message
processing to order book updates.

Establish a reproducible performance baseline for future
optimizations.

Validate the correctness of the integrated pipeline before
extending the system with additional components.


## Background

Previous phases focused on individual components of the market
data pipeline.

Ethernet, IPv4, UDP, ITCH parsing, and the matching engine were
implemented and validated independently.

P06 investigated and optimized the performance of the matching
engine.

The next step is to evaluate the complete software pipeline as a
single integrated system.

This phase establishes a baseline before introducing networking
overhead through DPDK.
## Success Criteria

- The complete pipeline processes market data correctly.

- End-to-end latency is measured.

- Throughput is measured.

- The benchmark is deterministic and reproducible.

- Performance results are documented.

- Profiling identifies the next optimization target.

## Design

The benchmark executes the complete software processing path.

ITCH messages are replayed from a deterministic data source.

Each message is decoded, translated into an internal event,
processed by the matching engine, and applied to the order book.

Latency and throughput are measured for the entire processing
pipeline.

The benchmark excludes DPDK and packet parsing in order to
measure the application processing cost independently from
networking overhead.


## Design Decision

The benchmark intentionally includes replay file reading.

Although file I/O introduces additional overhead, the benchmark aims to measure the complete software replay pipeline rather than isolated processing stages.

Future benchmark phases will separate file I/O from in-memory processing to evaluate the impact of storage independently.

## Processing Pipeline

P07 evaluates the complete software processing path used by the
current implementation.

The measured pipeline is shown below.

ITCH Replay Reader
    ↓
ItchReplayDispatcher
    ↓
ITCHHandler
    ↓
ITCH Parser
    ↓
ITCH Mapper
    ↓
ArrayOrderBook

The replay reader provides deterministic ITCH messages.

`ItchReplayDispatcher` routes each message according to its ITCH
message type.

`ITCHHandler` coordinates the processing of each message.

The corresponding parser converts the binary payload into a wire
message structure.

The mapper transforms the parsed message into the internal data
model.

Finally, the translated message is applied to the order book.

The benchmark measures the performance of this complete software
processing path.





Replayed ITCH messages are forwarded to
`ItchReplayDispatcher::dispatch()`.

The dispatcher reads the message type from the first byte of the
ITCH message.

It then routes the message to the corresponding method on
`ITCHHandler`.

The dispatcher stores a non-owning reference to `ITCHHandler`.

No order book or matching logic is implemented inside the
dispatcher.


ItchReplayReader
        │
        ▼
ItchReplayDispatcher::dispatch()
        │
        ├── 'A' → ITCHHandler::onAddOrder()
        ├── 'X' → ITCHHandler::onOrderCancel()
        ├── 'D' → ITCHHandler::onOrderDelete()
        ├── 'E' → ITCHHandler::onOrderExecuted()
        └── 'U' → ITCHHandler::onOrderReplace()




ITCH Processing Pipeline

                    ITCH Replay Reader
                            │
                            ▼
                  ItchReplayDispatcher
                            │
                            ▼
                      ITCHHandler
                            │
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
  ITCH Parser         ITCH Mapper        Order Lifecycle
                            │
                            ▼
                    ArrayOrderBook





## Dispatcher Integration

The benchmark uses `ItchReplayDispatcher::dispatch()` as the entry point into the message-processing path.

The dispatcher validates the message pointer and length.

It reads the ITCH message type from the first byte.

Supported message types are routed as follows:

| Message Type | Handler Method |
|---|---|
| `A` | `ITCHHandler::onAddOrder()` |
| `X` | `ITCHHandler::onOrderCancel()` |
| `D` | `ITCHHandler::onOrderDelete()` |
| `E` | `ITCHHandler::onOrderExecuted()` |
| `U` | `ITCHHandler::onOrderReplace()` |

Unsupported message types are rejected.

The benchmark will call the dispatcher rather than invoking handler methods directly.

This preserves the production processing path and includes dispatch overhead in the end-to-end measurement.

## Commands Executed

```bash
cat market_data/replay/itch_replay_dispatcher.hpp
cat market_data/replay/itch_replay_dispatcher.cpp



For each iteration : 

Create OrderBook
        │
Create Handler
        │
Create Dispatcher
        │
Open Replay File
        │
Create Message Buffer
        │
Start Timer (Google Benchmark)
        │
Read
Dispatch
Read
Dispatch
...
Until EOF
        │
End Iteration


## Replay Dataset Discovery

No valid ITCH replay dataset was found in the repository.

The discovered `.bin` files were CMake compiler-detection artifacts.

They do not contain market data and cannot be used by the pipeline benchmark.

A benchmark-specific replay fixture will therefore be created.

The fixture will use the binary format expected by `ItchReplayReader`.

Each record contains a two-byte network-order message length followed by the ITCH message payload.

Existing parser and integration tests will be inspected before generating the dataset.

This avoids duplicating or guessing the ITCH wire format.

## Commands Executed

```bash
find . -iname "*.bin" -o -iname "*.itch" -o -iname "*.dat" -o -iname "*.pcap"


Commands: 

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target pipeline_benchmark -j$(nproc)


./build/benchmarks/pipeline_benchmark



## Initial Benchmark Result

The end-to-end replay benchmark built and executed successfully.

The benchmark processed **5,000 ITCH messages** per iteration.

The benchmark was executed **five times** with the CPU scaling governor configured to **performance**.

The measured mean execution time was **21.76 milliseconds** per iteration.

The measured median execution time was **21.64 milliseconds** per iteration.

The reported mean throughput was approximately **229,937 messages per second**.

The reported median throughput was approximately **231,168 messages per second**.

The coefficient of variation (CV) was approximately **2.08%**, indicating that the benchmark results were reasonably stable across repeated executions.

The measured execution time corresponds to an average processing cost of approximately **4.35 microseconds per message**.

The measurement includes replay file reading, message dispatch, ITCH parsing, message mapping, handler execution, order book updates, and per-iteration object construction.

The result therefore represents an **end-to-end replay pipeline baseline** rather than a pure message-processing latency measurement.

Google Benchmark still reported that the benchmark library was built in **Debug** mode, which may introduce additional measurement overhead.

A setup-only baseline will be added in the next stage to estimate the cost of per-iteration object construction separately from replay processing.

## Commands Executed

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target pipeline_benchmark -j$(nproc)

cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

./build/benchmarks/pipeline_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true


Benchmark: pipelineReplay

Mean Time:        21,757,878 ns
Median Time:      21,637,759 ns

Mean Throughput:  229.937k messages/s
Median Throughput:231.168k messages/s

Standard Deviation: 451,582 ns
Coefficient of Variation (CV): 2.08%

Repetitions: 5



Step 1 — Build with debug symbols:
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build \
    --target pipeline_benchmark \
    -j$(nproc)

Step 2 — Record CPU profile:

rm -f pipeline-simple.data

perf record \
    -o pipeline-simple.data \
    -e cpu-clock \
    -F 199 \
    -- \
    ./build/benchmarks/pipeline_benchmark \
    --benchmark_min_time=3s



-------------------------------------------------------------------------
Benchmark               Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------
pipelineReplay   27916207 ns     27909721 ns          144 items_per_second=179.149k/s
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.080 MB pipeline-simple.data (1445 samples) ]


Step 3 — Report:

perf report \
    -i pipeline-simple.data \
    --stdio \
    --no-children \
    --sort overhead,comm,dso,symbol \
    --percent-limit 1

# Total Lost Samples: 0
#
# Samples: 1K of event 'cpu-clock'
# Event count (approx.): 7261305625
#
# Overhead  Command          Shared Object        Symbol                                                                                       IPC   [IPC Coverage]
# ........  ...............  ...................  ...........................................................................................  ....................
#
    96.54%  pipeline_benchm  pipeline_benchmark   [.] ArrayOrderBook::cancelOrder(unsigned long)                                               -      -            




perf annotate \
    -i pipeline-simple.data \
    --stdio \
    'ArrayOrderBook::cancelOrder(unsigned long)' \
    > cancel_annotate.txt

grep -E '^[[:space:]]*[0-9]+\.[0-9]+[[:space:]]*:' \
    cancel_annotate.txt \
    | sort -nr \
    | head -n 20


objdump -CdS build/benchmarks/pipeline_benchmark \
> pipeline_objdump.txt


Step 4 — Hot Functions:

Hotspot 1 — Tombstone Cleanup

The hottest instructions correspond to

clearDeletedEntries();

for (Entry& entry : entries_)
{
    if (entry.state == EntryState::Deleted)
        entry.state = EntryState::Empty;
}


Representative assembly:

cmpb   $0x2,(%rdx)
jne
movb   $0x0,(%rdx)
add    $0x18,%rdx
cmp    %rdx,%rax
jne


Observation

Whenever the order book becomes empty:

size_ == 0

the implementation scans the entire fixed hash table to convert tombstones back to empty entries.

Since the hash table contains 4096 entries, this operation performs a complete linear traversal.


Hotspot 2 — Best Bid Refresh:


The second hotspot corresponds to

while (best_bid_ > 0)
{
    if (!bid_levels_[best_bid_].empty())
        break;

    --best_bid_;
}

Representative assembly:

shl    $0x5,%rax
cmpq   $0x0,...
sub    $0x1,%rcx
jne


Observation

After removing the current best bid, the implementation walks downward one price level
at a time until it finds the next non-empty level.

Root Cause

The replay benchmark repeatedly empties the order book.

As a result, every replay iteration frequently performs:

cancelOrder()
    ↓
clearDeletedEntries()
        ↓
scan all hash-table entries

refreshBestBid()
        ↓
scan price levels

These two linear scans dominate the total execution time of the replay pipeline.




Conclusion

The replay pipeline itself is not the primary bottleneck.

Neither:

Replay reader
Dispatcher
ITCH parser
Mapper

consumes significant CPU time.

Instead, nearly all sampled CPU time is spent inside two linear scans within the software order book implementation.


Next Phase (P08)

Only after documenting these findings should optimization begin.

Candidate optimization topics include:

Eliminating the full tombstone cleanup scan.
Improving best bid / best ask maintenance.
Measuring the impact of each optimization independently.
Comparing latency and throughput before and after each change.

///////////////////////////////////////////////////////////////////////////////////////////////

P08 – FixedHashMap Optimization
Objective

Optimize the order cancellation path by removing unnecessary work in FixedHashMap::erase().

The previous profiling session identified ArrayOrderBook::cancelOrder() as the dominant CPU hotspot. Further investigation showed that FixedHashMap::erase() performed a full scan of the hash table whenever the container became empty.


Root Cause

The implementation contained the following code:

if (size_ == 0)
{
    clearDeletedEntries();
}

clearDeletedEntries() performs a linear scan over every bucket:

for (Entry& entry : entries_)
{
    if (entry.state == EntryState::Deleted)
        entry.state = EntryState::Empty;
}

Although correct, this cleanup is expensive and is executed every time the last element is erased.

The insert() implementation already reuses deleted buckets during probing, making this cleanup unnecessary for correctness.


Optimization

Removed the cleanup call from FixedHashMap::erase().

Before:

entry.state = EntryState::Deleted;
--size_;

if (size_ == 0)
{
    clearDeletedEntries();
}

After:

entry.state = EntryState::Deleted;
--size_;

return true;


Validation

A new regression test was added:

FixedHashMapTest.ReusesDeletedEntriesAfterBecomingEmpty

The test verifies that:

the table becomes empty,
deleted entries remain,
insertion still succeeds,
deleted slots are correctly reused.



Test Results

All FixedHashMap unit tests passed.

[==========] Running 10 tests
[  PASSED  ] 10 tests.

No regressions were observed.

Benchmark
Commands Executed
cmake --build build --target pipeline_benchmark -j$(nproc)

./build/benchmarks/pipeline_benchmark
Before Optimization
Metric	Value
Pipeline latency	21.76 ms
Throughput	229.9k messages/sec
After Optimization
Metric	Value
Pipeline latency	13.17 ms
Throughput	379.7k messages/sec
Performance Improvement
Metric	Improvement
Latency	39.4% lower
Throughput	65.1% higher
Overall Speedup	1.65×
Profiling
Commands Executed
perf record \
    -e cpu-clock \
    -g \
    -o pipeline-after-optimization.data \
    -- ./build/benchmarks/pipeline_benchmark

perf report \
    -i pipeline-after-optimization.data
Profiling Result

Even after the optimization, the primary hotspot remains:

94.28%
ArrayOrderBook::cancelOrder(unsigned long)

The previous bottleneck inside FixedHashMap::erase() has been removed, but cancelOrder() is still responsible for most of the remaining CPU time.

The next optimization step is to profile cancelOrder() at the source-line level using perf annotate to identify the new dominant hotspot.

Conclusion

The optimization successfully removed unnecessary work from the hash table erase path without affecting correctness.

Regression testing confirmed that deleted slots continue to be reused correctly.

Benchmark results showed a substantial improvement:

39.4% lower latency
65.1% higher throughput
1.65× overall speedup

The order cancellation path remains the dominant performance hotspot and will be the focus of the next optimization phase.


////////////////////////////////////////////////////////////////////////
P08


perf annotate \
    -i pipeline-after-optimization.data \
    --stdio \
    'ArrayOrderBook::cancelOrder(unsigned long)'

    0.00 :   49a0:        movq    %rcx, %rax
   39.28 :   49a3:        shlq    $0x5, %rax
    0.00 :   49a7:        cmpq    $0x0, 0x8(%r8,%rax)
   20.90 :   49ad:        jne     0x48a4 <ArrayOrderBook::cancelOrder(unsigned long)+0xa4>
   39.53 :   49b3:        subq    $0x1, %rcx
    0.03 :   49b7:        movq    %rcx, 0x61a800(%r8)
    0.00 :   49be:        jne     0x49a0 <ArrayOrderBook::cancelOrder(unsigned long)+0x1a0>
    0.00 :   49c0:        movq    $0x0, 0x61a800(%r8)
    0.00 :   49cb:        jmp     0x48a4 <ArrayOrderBook::cancelOrder(unsigned long)+0xa4>

Bottleneck : 

while (best_bid_ > 0)
{
    if (!bid_levels_[best_bid_].empty())
        break;

    --best_bid_;
}





























