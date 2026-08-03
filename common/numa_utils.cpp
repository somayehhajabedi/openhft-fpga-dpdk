#include "numa_utils.hpp"

#include <numa.h>
#include <sched.h>

namespace numa_utils
{

bool isAvailable()
{
    return numa_available() >= 0;
}

int nodeCount()
{
    if (!isAvailable())
    {
        return 0;
    }

    return numa_max_node() + 1;
}

int currentCpuNode()
{
    if (!isAvailable())
    {
        return -1;
    }

    const int cpu = sched_getcpu();

    if (cpu < 0)
    {
        return -1;
    }

    return numa_node_of_cpu(cpu);
}

bool isValidNode(int node)
{
    return node >= 0 &&
           node < nodeCount();
}

void* allocateOnNode(
    std::size_t size,
    int node)
{
    if (size == 0 ||
        !isValidNode(node))
    {
        return nullptr;
    }

    return numa_alloc_onnode(
        size,
        node);
}

void freeMemory(
    void* memory,
    std::size_t size)
{
    if (memory == nullptr ||
        size == 0)
    {
        return;
    }

    numa_free(
        memory,
        size);
}

} // namespace numa_utils

