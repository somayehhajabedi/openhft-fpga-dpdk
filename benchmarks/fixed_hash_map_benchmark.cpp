#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "common/fixed_hash_map.hpp"

namespace
{
constexpr std::size_t Capacity = 4096;
constexpr std::size_t ItemCount = 2048;
}

static void BM_FixedHashMapInsert(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        FixedHashMap<std::uint64_t, std::uint64_t, Capacity> map;
        state.ResumeTiming();

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            benchmark::DoNotOptimize(map.insert(i, i));
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(ItemCount));
}

static void BM_UnorderedMapInsert(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        std::unordered_map<std::uint64_t, std::uint64_t> map;
        map.reserve(Capacity);
        state.ResumeTiming();

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            benchmark::DoNotOptimize(map.emplace(i, i));
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(ItemCount));
}

static void BM_FixedHashMapFind(benchmark::State& state)
{
    FixedHashMap<std::uint64_t, std::uint64_t, Capacity> map;

    for (std::uint64_t i = 0; i < ItemCount; ++i)
    {
        map.insert(i, i);
    }

    std::uint64_t key = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(map.find(key));

        ++key;
        if (key == ItemCount)
        {
            key = 0;
        }
    }
}

static void BM_UnorderedMapFind(benchmark::State& state)
{
    std::unordered_map<std::uint64_t, std::uint64_t> map;
    map.reserve(Capacity);

    for (std::uint64_t i = 0; i < ItemCount; ++i)
    {
        map.emplace(i, i);
    }

    std::uint64_t key = 0;

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(map.find(key));

        ++key;
        if (key == ItemCount)
        {
            key = 0;
        }
    }
}

static void BM_FixedHashMapErase(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        FixedHashMap<std::uint64_t, std::uint64_t, Capacity> map;

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            map.insert(i, i);
        }

        state.ResumeTiming();

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            benchmark::DoNotOptimize(map.erase(i));
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(ItemCount));
}

static void BM_UnorderedMapErase(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        std::unordered_map<std::uint64_t, std::uint64_t> map;
        map.reserve(Capacity);

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            map.emplace(i, i);
        }

        state.ResumeTiming();

        for (std::uint64_t i = 0; i < ItemCount; ++i)
        {
            benchmark::DoNotOptimize(map.erase(i));
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(ItemCount));
}

BENCHMARK(BM_FixedHashMapInsert);
BENCHMARK(BM_UnorderedMapInsert);

BENCHMARK(BM_FixedHashMapFind);
BENCHMARK(BM_UnorderedMapFind);

BENCHMARK(BM_FixedHashMapErase);
BENCHMARK(BM_UnorderedMapErase);

BENCHMARK_MAIN();
