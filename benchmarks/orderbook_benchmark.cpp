#include <benchmark/benchmark.h>

#include "array_order_book.hpp"
#include "order.hpp"

static Order makeOrder(
    OrderId id,
    Side side = Side::Buy,
    Price price = 1000,
    Quantity quantity = 100)
{
    return Order{
        .id = id,
        .account_id = 100,
        .side = side,
        .price = price,
        .quantity = quantity
    };
}

static void BM_AddOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;
        Order order = makeOrder(1);

        state.ResumeTiming();

        book.addOrder(&order);

        benchmark::DoNotOptimize(book.bestBid());
    }
}

static void BM_CancelOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;
        Order order = makeOrder(1);

        book.addOrder(&order);

        state.ResumeTiming();

        bool result = book.cancelOrder(order.id);

        benchmark::DoNotOptimize(result);
    }
}
static void BM_ReplaceOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;
        Order order = makeOrder(1);

        book.addOrder(&order);

        state.ResumeTiming();

        bool result = book.replaceOrder(
            order.id,
            2,
            200,
            1100
        );

        benchmark::DoNotOptimize(result);
    }
}

static void BM_ExecuteOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();

        ArrayOrderBook book;
        Order order = makeOrder(1);

        book.addOrder(&order);

        state.ResumeTiming();

        bool result = book.executeOrder(
            order.id,
            50
        );

        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_AddOrder)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_CancelOrder)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ReplaceOrder)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ExecuteOrder)
    ->Unit(benchmark::kNanosecond);
