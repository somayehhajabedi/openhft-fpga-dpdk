# Order Pool

## Motivation

The initial implementation allocated every incoming order dynamically:

```cpp
auto order = std::make_unique<Order>();
```

While correct, dynamic allocation (`new`/`delete`) introduces allocator overhead,
additional latency, and latency jitter in the hot path.

Since a low-latency matching engine processes millions of orders, eliminating
dynamic allocation from the critical path improves determinism and overall
performance.

---

## Design

The OrderPool preallocates a fixed number of `Order` objects during
initialization.

```text
            Constructor

      +----------------------+
      |   std::vector<Order> |
      +----------------------+

      +----------------------+
      | free_list_           |
      | Order*               |
      +----------------------+
```

No additional memory allocation occurs after construction.

---

## API

```cpp
class OrderPool
{
public:
    explicit OrderPool(std::size_t capacity);

    Order* acquire();

    void release(Order* order);

    std::size_t capacity() const;

    std::size_t available() const;
};
```

---

## Acquire

```cpp
Order* acquire();
```

Behavior:

- Returns one available `Order`
- Uses LIFO allocation
- Returns `nullptr` when exhausted
- Performs no heap allocation

Complexity:

```
O(1)
```

---

## Release

```cpp
void release(Order* order);
```

Behavior:

- Returns an order back to the pool
- Performs no deallocation
- Executes in constant time

Complexity:

```
O(1)
```

---

## Why LIFO?

The free list is implemented as a stack.

```text
Acquire

pop_back()

Release

push_back()
```

The most recently released object has the highest probability of still being
present in the CPU cache.

Reusing that object improves cache locality and reduces memory access latency.

---

## Responsibility

The OrderPool owns memory only.

It does **not**:

- initialize an order
- reset an order
- validate an order
- insert into the order book

The consumer is responsible for constructing a valid `Order`.

---

## Unit Testing

The following tests were implemented:

- Pool initialization
- Acquire decreases available objects
- Pool exhaustion
- Release returns object
- LIFO behavior

All tests passed.

```
90 / 90 tests passed
```

---

## Benchmark

Environment

- Intel CPU
- Google Benchmark

Results

| Benchmark | Time |
|-----------|------:|
| BM_NewDelete | 8.49 ns |
| BM_OrderPool | 2.36 ns |

Approximate speedup

```
3.6x
```

Notes

The benchmark was executed with the Benchmark library built in Debug mode.

Although absolute timings may change in Release mode, both implementations were
measured under identical conditions, making the comparison valid.

---

## Current Status

Completed

- OrderPool implementation
- Unit tests
- Benchmark
- Integration into build system

Next

- Integrate OrderPool into ITCHHandler
- Remove dynamic allocation from the hot path
- Benchmark end-to-end pipeline latency
