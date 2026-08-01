/*
 * Memory Allocator Benchmark
 *
 * Purpose
 * -------
 * This benchmark compares different allocation strategies used in
 * low-latency systems.
 *
 * The benchmark measures:
 *
 *   - Standard new/delete
 *   - STL container allocations
 *   - Object pool allocation
 *   - std::pmr memory resources
 *
 * The goal is to evaluate allocation overhead, memory locality,
 * and allocation scalability.
 *
 * Frequent dynamic allocations are expensive in latency-sensitive
 * applications because they may introduce allocator contention,
 * fragmentation, cache misses, and unpredictable execution time.
 *
 * Modern C++ provides std::pmr (Polymorphic Memory Resources)
 * to reduce allocation overhead by reusing preallocated memory.
 *
 * This benchmark studies the performance characteristics of
 * different allocation strategies under identical workloads.
 */

 #include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <array>
#include <memory_resource>

namespace
{

struct TestObject
{
    std::uint64_t id = 0;
    std::uint64_t value = 0;
    std::uint64_t timestamp = 0;
};

void BM_NewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* object =
            new TestObject{
                1,
                100,
                123456789
            };

        benchmark::DoNotOptimize(object);

        delete object;
    }
}

} // namespace

BENCHMARK(BM_NewDelete);

void BM_PMRMonotonic(benchmark::State& state)
{
    alignas(std::max_align_t)
    std::array<std::byte, 1024 * 1024> buffer{};

    std::pmr::monotonic_buffer_resource resource(
        buffer.data(),
        buffer.size());

    std::pmr::polymorphic_allocator<TestObject> allocator(
        &resource);

    for (auto _ : state)
    {
        TestObject* object =
            allocator.allocate(1);

        std::construct_at(
            object,
            TestObject{
                1,
                100,
                123456789
            });

        benchmark::DoNotOptimize(object);

        std::destroy_at(object);

        resource.release();
    }
}

BENCHMARK(BM_PMRMonotonic);


void BM_PMRBatch(benchmark::State& state)
{
    constexpr std::size_t ObjectCount = 1000;

    alignas(std::max_align_t)
    std::array<std::byte, 1024 * 1024> buffer{};

    std::pmr::monotonic_buffer_resource resource(
        buffer.data(),
        buffer.size());

    std::pmr::polymorphic_allocator<TestObject> allocator(
        &resource);

    for (auto _ : state)
    {
        TestObject* objects[ObjectCount];

        for (std::size_t i = 0; i < ObjectCount; ++i)
        {
            objects[i] = allocator.allocate(1);

            std::construct_at(
                objects[i],
                TestObject{
                    i,
                    i * 10,
                    i * 100
                });
        }

        benchmark::DoNotOptimize(objects);

        resource.release();
    }

    state.SetItemsProcessed(
        state.iterations() * ObjectCount);
}


BENCHMARK(BM_PMRBatch);

void BM_NewDeleteBatch(benchmark::State& state)
{
    constexpr std::size_t ObjectCount = 1000;

    for (auto _ : state)
    {
        TestObject* objects[ObjectCount];

        for (std::size_t i = 0; i < ObjectCount; ++i)
        {
            objects[i] = new TestObject{
                i,
                i * 10,
                i * 100
            };
        }

        benchmark::DoNotOptimize(objects);

        for (std::size_t i = 0; i < ObjectCount; ++i)
        {
            delete objects[i];
        }
    }

    state.SetItemsProcessed(
        state.iterations() * ObjectCount);
}

BENCHMARK(BM_NewDeleteBatch);

BENCHMARK_MAIN();



