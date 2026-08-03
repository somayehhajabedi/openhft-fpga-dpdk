/*
 * NUMA Allocation Benchmark
 *
 * Purpose
 * -------
 * This benchmark compares standard heap-backed memory with memory explicitly
 * allocated on a NUMA node using libnuma.
 *
 * On the current development machine, only one NUMA node is available.
 * Therefore, this benchmark measures local-node allocation behavior only.
 *
 * A true local-versus-remote NUMA comparison requires a system with at least
 * two NUMA nodes.
 */

#include <benchmark/benchmark.h>

#include "common/numa_utils.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{

constexpr std::size_t BufferSize =
    256ULL * 1024ULL * 1024ULL;

constexpr std::size_t Stride = 4096;

void touchBuffer(
    std::uint8_t* buffer,
    std::size_t size)
{
    for (std::size_t offset = 0;
         offset < size;
         offset += Stride)
    {
        ++buffer[offset];
    }

    benchmark::DoNotOptimize(buffer);
    benchmark::ClobberMemory();
}

void BM_StandardMemory(benchmark::State& state)
{
    std::vector<std::uint8_t> buffer(BufferSize);

    for (auto _ : state)
    {
        touchBuffer(
            buffer.data(),
            buffer.size());
    }

    state.SetBytesProcessed(
        static_cast<std::int64_t>(
            state.iterations() * BufferSize));
}

void BM_NumaNode0Memory(benchmark::State& state)
{
    void* rawMemory =
        numa_utils::allocateOnNode(
            BufferSize,
            0);

    if (rawMemory == nullptr)
    {
        state.SkipWithError(
            "Failed to allocate memory on NUMA node 0");
        return;
    }

    auto* buffer =
        static_cast<std::uint8_t*>(rawMemory);

    for (auto _ : state)
    {
        touchBuffer(
            buffer,
            BufferSize);
    }

    numa_utils::freeMemory(
        rawMemory,
        BufferSize);

    state.SetBytesProcessed(
        static_cast<std::int64_t>(
            state.iterations() * BufferSize));
}

} // namespace

BENCHMARK(BM_StandardMemory)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_NumaNode0Memory)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();

