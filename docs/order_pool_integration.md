P11 - OrderPool Integration

Overview

This phase integrates OrderPool into the software order book pipeline to eliminate dynamic memory allocations from the hot path.

Previously, ITCHHandler owned all Order objects using:

std::vector<std::unique_ptr<Order>>

Each incoming Add Order message allocated memory using std::make_unique, while deleted orders relied on object destruction.

After this refactoring, OrderPool becomes the sole owner of all Order objects.


Motivation

Dynamic memory allocation (new / delete) introduces:

allocator overhead
cache misses
memory fragmentation
latency spikes

These characteristics are undesirable for low-latency trading systems.

Using a fixed-size object pool provides:

deterministic allocation
constant-time acquire/release
improved cache locality
zero heap allocations in the hot path
Architecture Before
ITCHHandler
      │
      ├── make_unique<Order>()
      │
      ▼
ArrayOrderBook
      │
      ▼
vector<unique_ptr<Order>>

Ownership of orders was coupled with the ITCH message handler.

Architecture After
                acquire()
                    │
                    ▼
             +--------------+
             |  OrderPool   |
             +--------------+
               ▲          │
               │          ▼
          release()   ArrayOrderBook
               ▲          │
               └──────────┘
               ITCHHandler

Ownership is centralized inside OrderPool.

Implemented Changes
1. ITCHHandler

Removed:

std::vector<std::unique_ptr<Order>> orders_;

Replaced with:

OrderPool orderPool_;
2. Add Order

Before:

auto order = std::make_unique<Order>();

After:

Order* order = orderPool_.acquire();
3. Delete Order

cancelOrder() now returns the removed Order*.

Deleted orders are recycled:

orderPool_.release(order);
4. Cancel Order

reduceOrder() now returns:

struct OrderUpdateResult
{
    bool success;
    Order* removed_order;
};

Partial cancel:

removed_order == nullptr

Full cancel:

removed_order != nullptr

The handler releases removed orders back to the pool.

5. Execute Order

Execution now follows the same ownership model as cancellation.

Orders completely filled are recycled into the pool.

6. Replace Order

No new allocation occurs.

The existing Order object is reused:

cancelOrder()

↓

update fields

↓

addOrder()

This preserves cache locality and avoids unnecessary allocation.

Benefits
No heap allocation in the hot path
Centralized ownership
Better cache locality
Deterministic memory management
Reduced allocator overhead
Cleaner separation of responsibilities
Current Status
Feature	Status
OrderPool implementation	
Add integration	
Delete integration	
Cancel integration	
Execute integration	
Replace integration	
Tests	 Passing
Benchmarks	Passing

./build-profile/benchmarks/order_pool_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true
2026-07-31T15:43:01-04:00
Running ./build-profile/benchmarks/order_pool_benchmark
Run on (8 X 4618.99 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.76, 0.97, 0.65
***WARNING*** Library was built as DEBUG. Timings may be affected.
--------------------------------------------------------------
Benchmark                    Time             CPU   Iterations
--------------------------------------------------------------
BM_NewDelete_mean         9.41 ns         9.39 ns            5
BM_NewDelete_median       9.40 ns         9.36 ns            5
BM_NewDelete_stddev      0.124 ns        0.122 ns            5
BM_NewDelete_cv           1.32 %          1.30 %             5
BM_OrderPool_mean         3.05 ns         3.05 ns            5
BM_OrderPool_median       3.01 ns         3.01 ns            5
BM_OrderPool_stddev      0.102 ns        0.098 ns            5
BM_OrderPool_cv           3.33 %          3.22 %             5



perf record \
    -o perf-order-pool.data \
    -- \
    ./build-profile/benchmarks/order_pool_benchmark \
        --benchmark_filter=BM_OrderPool \
        --benchmark_min_time=1s \
        --benchmark_repetitions=1
2026-07-31T15:43:32-04:00
Running ./build-profile/benchmarks/order_pool_benchmark
Run on (8 X 4404.35 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 2.32, 1.04, 0.68
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_OrderPool       2.92 ns         2.92 ns    478958627
[ perf record: Woken up 2 times to write data ]
[ perf record: Captured and wrote 0.288 MB perf-order-pool.data (6890 samples) ]


perf report     -i perf-order-pool.data     --stdio     --no-children     --sort overhead,symbol     --percent-limit 1
                                                                                                                                                          
    48.89%  [.] OrderPool::release(Order*)   
    27.90%  [.] BM_OrderPool(benchmark::State&) 
    23.04%  [.] OrderPool::acquire() 


## Perf Profiling

A flat `perf` profile was collected for the OrderPool benchmark.

| Symbol | CPU cycles |
|---|---:|
| `OrderPool::release(Order*)` | 48.89% |
| `BM_OrderPool(benchmark::State&)` | 27.90% |
| `OrderPool::acquire()` | 23.04% |

No samples were attributed to:

- `malloc`
- `free`
- `operator new`
- `operator delete`

This confirms that the OrderPool hot path operates without dynamic memory allocation.


perf record \
    -o perf-new-delete.data \
    -- \
    ./build-profile/benchmarks/order_pool_benchmark \
        --benchmark_filter=BM_NewDelete \
        --benchmark_min_time=1s \
        --benchmark_repetitions=1
2026-07-31T15:48:21-04:00
Running ./build-profile/benchmarks/order_pool_benchmark
Run on (8 X 3499.96 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.85, 0.90, 0.72
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_NewDelete       10.6 ns         10.6 ns    128044576
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.251 MB perf-new-delete.data (5936 samples) ]


perf report \
    -i perf-new-delete.data \
    --stdio \
    --no-children \
    --sort overhead,symbol \
    --percent-limit 1


37.31%  malloc
21.04%  cfree
10.77%  operator new
3.19%  operator delete
3.02%  sized operator delete


## Perf Profiling

A flat `perf` profile was collected for both allocation strategies.

### OrderPool

| Symbol | CPU cycles |
|---|---:|
| `OrderPool::release(Order*)` | 48.89% |
| `BM_OrderPool(benchmark::State&)` | 27.90% |
| `OrderPool::acquire()` | 23.04% |

No samples were attributed to:

- `malloc`
- `free`
- `operator new`
- `operator delete`

### New/Delete

| Symbol | CPU cycles |
|---|---:|
| `malloc` | 37.31% |
| `cfree` | 21.04% |
| `operator new(unsigned long)` | 10.77% |
| `operator delete(void*)` | 3.19% |
| `operator delete(void*, unsigned long)` | 3.02% |

The `new/delete` benchmark is dominated by allocator overhead, while the
OrderPool hot path performs only constant-time free-list operations.

This confirms that the OrderPool removes dynamic allocation from the critical
path and improves latency determinism.








