
#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dispatcher/event_dispatcher.hpp"
#include "orderbook/software/matching_engine.hpp"
#include "orderbook/software/order.hpp"

static void BM_MatchingEngineInsert(benchmark::State& state)
{
    constexpr std::size_t batch_size = 1024;

    for (auto _ : state)
    {
        state.PauseTiming();

        EventDispatcher dispatcher;
        MatchingEngine engine(dispatcher);

        std::vector<Order> orders;
        orders.reserve(batch_size);

        for (std::size_t i = 0; i < batch_size; ++i)
        {
            orders.push_back(Order{
                static_cast<OrderId>(i + 1),
                static_cast<AccountId>(1001),
                Side::Buy,
                static_cast<Price>(100),
                static_cast<Quantity>(100)
            });
        }

        state.ResumeTiming();

        for (Order& order : orders)
        {
            benchmark::DoNotOptimize(order);
            engine.process(&order);
        }

        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(batch_size));
}

BENCHMARK(BM_MatchingEngineInsert);

BENCHMARK_MAIN();
