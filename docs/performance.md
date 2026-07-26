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



