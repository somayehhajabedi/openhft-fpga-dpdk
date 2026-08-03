#pragma once

#include <cstddef>

namespace numa_utils
{

[[nodiscard]] bool isAvailable();

[[nodiscard]] int nodeCount();

[[nodiscard]] int currentCpuNode();

[[nodiscard]] bool isValidNode(int node);

[[nodiscard]] void* allocateOnNode(
    std::size_t size,
    int node);

void freeMemory(
    void* memory,
    std::size_t size);

} // namespace numa_utils

