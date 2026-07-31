#include <benchmark/benchmark.h>

#include "order.hpp"

static void BM_NewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        Order* order = new Order;

        benchmark::DoNotOptimize(order);

        delete order;
    }
}

BENCHMARK(BM_NewDelete);

#include "order_pool.hpp"

static void BM_OrderPool(benchmark::State& state)
{
    OrderPool pool(1024);

    for (auto _ : state)
    {
        Order* order = pool.acquire();

        benchmark::DoNotOptimize(order);

        pool.release(order);
    }
}

BENCHMARK(BM_OrderPool);


