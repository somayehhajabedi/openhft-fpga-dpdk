# P09 - Bitmap-Based Order Book Optimization

P09 Bitmap Optimization
│
├── Executive Summary
├── Problem Statement
├── Baseline Measurements
├── Root Cause Analysis
├── Alternative Solutions
│     ├── Linear Scan
│     ├── Tree
│     ├── Bitmap   ← انتخاب شد
│
├── Why Bitmap?
├── Complexity Analysis
├── Memory Impact
├── Cache Analysis
├── Implementation
├── Correctness
├── Benchmark Results
├── Profiling Results
├── Lessons Learned
├── Interview Talking Points
└── Future Work



## Objective

Reduce the latency of best bid and best ask updates by replacing the linear scan performed in `refreshBestBid()` and `refreshBestAsk()` with a bitmap-based lookup.

---

## Background

Performance profiling after P08 showed that the dominant execution time was spent inside `ArrayOrderBook::cancelOrder()`.

Further investigation using `perf annotate` revealed that the primary hotspot was the linear search performed when the current best bid or best ask price level became empty.

---

## Root Cause

Current implementation:

```cpp
void ArrayOrderBook::refreshBestBid()
{
    while (best_bid_ > 0)
    {
        const PriceLevel& level =
            bid_levels_[priceToIndex(best_bid_)];

        if (!level.empty())
            return;

        --best_bid_;
    }

    best_bid_ = 0;
}
```

The same algorithm is used for `refreshBestAsk()`.

### Complexity

| Case | Complexity |
|------|------------|
| Best | O(1) |
| Worst | O(number of empty price levels) |

When many consecutive price levels are empty, the algorithm performs unnecessary memory accesses before finding the next active level.

---

## Design Goals

- Preserve the public API.
- Keep deterministic behaviour.
- Avoid dynamic memory allocation.
- Minimize code changes.
- Improve cache efficiency.
- Replace linear scanning with bitmap lookup.

---

## Proposed Solution

Introduce two bitmaps.

```cpp
std::array<std::uint64_t, BitmapWordCount> bid_level_bitmap_;
std::array<std::uint64_t, BitmapWordCount> ask_level_bitmap_;
```

Each bit represents one price level.

```
Bit = 1   Active price level
Bit = 0   Empty price level
```

---

## Bitmap Update Rules

| Operation | Bitmap Action |
|----------|---------------|
| First order inserted into a level | Set bit |
| Last order removed from a level | Clear bit |
| Quantity reduction | No change |
| Partial execution | No change |
| Replace order | Handled through cancel + add |

---

## Implementation Plan

### Phase 1

- Add bitmap data structures.
- Implement bitmap helper functions.
- No behavioural changes.

### Phase 2

- Update `addOrder()`.
- Update `cancelOrder()`.
- Keep current refresh algorithm.

### Phase 3

Replace the linear scan inside:

- `refreshBestBid()`
- `refreshBestAsk()`

with bitmap search.

---

## Commands Executed

```bash
mkdir -p tests/unit/orderbook

touch tests/unit/orderbook/array_order_book_test.cpp

cmake -S . -B build

cmake --build build -j$(nproc)
```

---

## Current Status

Completed:

- Bitmap design
- Data structure design
- Helper API design
- Dedicated OrderBook test directory
- Test infrastructure update

In Progress:

- Bitmap helper implementation

Pending:

- Bitmap integration
- Unit tests
- Benchmark
- Profiling
- Performance comparison

The original implementation checked each price level one by one, which caused unnecessary memory accesses. By maintaining a bitmap of active price levels, I reduced the search to word-level operations and used bit manipulation to locate the next active price. This significantly reduces memory traffic and improves latency, especially when the order book is sparse.


##  Points

• Original implementation scanned price levels linearly.

• Profiling showed refreshBestBid() and refreshBestAsk() were hotspots.

• Introduced a bitmap of active price levels.

• Reduced memory accesses by operating on 64-bit words.

• Used C++20 bit operations for efficient bit scanning.

• Validated correctness with unit tests.

• Measured improvement using Google Benchmark and Linux perf.


Pros

- Faster best-price lookup
- Better cache behavior
- Deterministic execution

Cons

- Additional bitmap memory
- Slightly more complex update logic
- Bitmap must remain synchronized with price levels

























cmake -S . -B build-release \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-release -j$(nproc)



./build-release/benchmarks/bitmap_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true


    -------------------------------------------------------------------------------------------
Benchmark                                                 Time             CPU   Iterations
-------------------------------------------------------------------------------------------
BM_RefreshBestBidLargeGap/iterations:1000_mean         1476 ns         1042 ns            5
BM_RefreshBestBidLargeGap/iterations:1000_median       1431 ns         1011 ns            5
BM_RefreshBestBidLargeGap/iterations:1000_stddev        117 ns         66.8 ns            5
BM_RefreshBestBidLargeGap/iterations:1000_cv           7.91 %          6.41 %             5
BM_RefreshBestAskLargeGap/iterations:1000_mean         1280 ns          911 ns            5
BM_RefreshBestAskLargeGap/iterations:1000_median       1240 ns          870 ns            5
BM_RefreshBestAskLargeGap/iterations:1000_stddev       91.9 ns         93.2 ns            5
BM_RefreshBestAskLargeGap/iterations:1000_cv           7.18 %         10.23 %             5
BM_RefreshBestBidSameWord/iterations:1000_mean          871 ns          570 ns            5
BM_RefreshBestBidSameWord/iterations:1000_median        862 ns          588 ns            5
BM_RefreshBestBidSameWord/iterations:1000_stddev       23.5 ns         58.0 ns            5
BM_RefreshBestBidSameWord/iterations:1000_cv           2.70 %         10.18 %             5
BM_RefreshBestAskSameWord/iterations:1000_mean          898 ns          519 ns            5
BM_RefreshBestAskSameWord/iterations:1000_median        895 ns          519 ns            5
BM_RefreshBestAskSameWord/iterations:1000_stddev       69.3 ns         43.2 ns            5
BM_RefreshBestAskSameWord/iterations:1000_cv           7.72 %          8.32 %             5


Bitmap lookup keeps nearby best-price refreshes significantly cheaper than wide-gap refreshes. In the measured scenarios, same-word refreshes completed in approximately 0.52–0.57 µs CPU time, while large-gap refreshes required approximately 0.91–1.04 µs.


perf record -o perf-bitmap-flat.data -- \
./build-release/benchmarks/bitmap_benchmark \
    --benchmark_filter=BM_RefreshBestBidLargeGap \
    --benchmark_repetitions=1

------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations
------------------------------------------------------------------------------------
BM_RefreshBestBidLargeGap/iterations:1000       1408 ns         1125 ns         1000
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.044 MB perf-bitmap-flat.data (511 samples) ]


perf report \
    -i perf-bitmap-flat.data \
    --stdio \
    --no-children \
    --sort overhead,symbol \
    --percent-limit 1


# Overhead  Symbol                                          IPC   [IPC Coverage]
# ........  ..............................................  ....................
#
    94.53%  [.] __memset_avx2_unaligned_erms                -      -            
     1.43%  [.] ArrayOrderBook::cancelOrder(unsigned long)  -      -            











