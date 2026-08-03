#include "thread_affinity.hpp"

#include <pthread.h>
#include <sched.h>

bool pinCurrentThreadToCpu(std::size_t cpuIndex)
{
    cpu_set_t cpuSet;

    CPU_ZERO(&cpuSet);
    CPU_SET(cpuIndex, &cpuSet);

    const int result =
        pthread_setaffinity_np(
            pthread_self(),
            sizeof(cpuSet),
            &cpuSet);

    return result == 0;
}