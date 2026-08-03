
/*
 * Huge Page Benchmark
 *
 * Purpose
 * -------
 * This benchmark compares memory access behavior when using:
 *
 *   1. Normal anonymous memory backed by standard 4 KB pages.
 *   2. Explicit HugeTLB memory backed by 2 MB huge pages.
 *
 * The goal is to evaluate the effect of page size on:
 *
 *   - dTLB pressure
 *   - dTLB load misses
 *   - memory access latency
 *   - effective memory throughput
 *
 * Benchmark Design
 * ----------------
 * Both benchmarks allocate a large buffer of the same size and execute the
 * same access pattern.
 *
 * The buffer is traversed with a 4 KB stride:
 *
 *     buffer[0]
 *     buffer[4096]
 *     buffer[8192]
 *     ...
 *
 * This access pattern touches one byte in each normal memory page and is
 * intentionally designed to place pressure on the data Translation Lookaside
 * Buffer (dTLB).
 *
 * With a 256 MB buffer:
 *
 *   - Standard 4 KB pages require approximately 65,536 page translations.
 *   - 2 MB huge pages require only 128 page translations.
 *
 * This provides a controlled workload for measuring how larger pages reduce
 * the number of virtual-to-physical address translations.
 *
 * Normal Page Benchmark
 * ---------------------
 * BM_NormalPages uses std::vector<std::uint8_t>.
 *
 * The vector is backed by the regular process heap and normally uses standard
 * 4 KB pages, unless the operating system transparently promotes some pages.
 *
 * Huge Page Benchmark
 * -------------------
 * BM_HugePages allocates memory using:
 *
 *     mmap(..., MAP_HUGETLB, ...)
 *
 * This explicitly requests memory from the Linux HugeTLB pool.
 *
 * The system must have enough huge pages reserved before running the
 * benchmark. For example, reserving 128 pages of 2 MB provides 256 MB:
 *
 *     echo 128 | sudo tee /proc/sys/vm/nr_hugepages
 *
 * The current HugeTLB state can be checked with:
 *
 *     grep Huge /proc/meminfo
 *
 * Measurement
 * -----------
 * Google Benchmark reports execution time and processed bytes.
 *
 * Linux perf can be used to compare TLB activity:
 *
 *     perf stat \
 *         -e dTLB-loads,dTLB-load-misses \
 *         ./huge_page_benchmark \
 *         --benchmark_filter=BM_NormalPages
 *
 *     perf stat \
 *         -e dTLB-loads,dTLB-load-misses \
 *         ./huge_page_benchmark \
 *         --benchmark_filter=BM_HugePages
 *
 * Important Notes
 * ---------------
 * Huge pages do not automatically improve every workload.
 *
 * They are most useful when:
 *
 *   - the working set spans many regular pages
 *   - TLB misses are significant
 *   - memory access patterns touch a large address range
 *
 * The matching-engine workload already has a very low dTLB miss rate, so this
 * dedicated benchmark intentionally creates stronger TLB pressure to make the
 * effect of page size measurable.
 *
 * This benchmark is designed to study page translation behavior, not to claim
 * that HugeTLB always improves application performance.
 */

#include <benchmark/benchmark.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <vector>

namespace
{

constexpr std::size_t BufferSize =
    256ULL * 1024ULL * 1024ULL;

constexpr std::size_t NormalPageStride = 4096;

class HugePageBuffer
{
public:
    explicit HugePageBuffer(std::size_t size)
        : size_(size)
    {
        data_ = static_cast<std::uint8_t*>(
            mmap(
                nullptr,
                size_,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE |
                    MAP_ANONYMOUS |
                    MAP_HUGETLB,
                -1,
                0));

        if (data_ == MAP_FAILED)
        {
            data_ = nullptr;
        }
    }

    ~HugePageBuffer()
    {
        if (data_ != nullptr)
        {
            munmap(data_, size_);
        }
    }

    HugePageBuffer(const HugePageBuffer&) = delete;
    HugePageBuffer& operator=(const HugePageBuffer&) = delete;

    [[nodiscard]] bool valid() const
    {
        return data_ != nullptr;
    }

    [[nodiscard]] std::uint8_t* data()
    {
        return data_;
    }

private:
    std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};


// Touch one byte per 4 KB region to create page-level memory accesses.
// On normal memory, each access usually targets a different 4 KB page.
// On HugeTLB memory, many of these accesses share the same 2 MB translation.
void touchBuffer(
    std::uint8_t* buffer,
    std::size_t size)
{
    for (std::size_t offset = 0;
         offset < size;
         offset += NormalPageStride)
    {
        ++buffer[offset];
    }

    benchmark::DoNotOptimize(buffer);
    benchmark::ClobberMemory();
}

void BM_NormalPages(benchmark::State& state)
{
    std::vector<std::uint8_t> buffer(BufferSize);

    for (auto _ : state)
    {
        touchBuffer(buffer.data(), buffer.size());
    }

    state.SetBytesProcessed(
        static_cast<std::int64_t>(
            state.iterations() * BufferSize));
}

void BM_HugePages(benchmark::State& state)
{
    HugePageBuffer buffer(BufferSize);

    if (!buffer.valid())
    {
        state.SkipWithError(std::strerror(errno));
        return;
    }

    for (auto _ : state)
    {
        touchBuffer(buffer.data(), BufferSize);
    }

    state.SetBytesProcessed(
        static_cast<std::int64_t>(
            state.iterations() * BufferSize));
}

} // namespace

BENCHMARK(BM_NormalPages)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_HugePages)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();